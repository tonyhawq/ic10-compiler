#include "CodeGenerator.h"
#include "Compiler.h"

std::unique_ptr<StackVariable> StackEnvironment::resolve(const std::string& name)
{
	const auto& val = this->variables.find(name);
	if (val != this->variables.end())
	{
		printf("resolved %s\n", name.c_str());
		return std::make_unique<StackVariable>(this->variables.at(name));
	}
	if (this->parent)
	{
		printf("While resolving %s added %i because of frame size\n", name.c_str(), this->m_frame_size);
		std::unique_ptr<StackVariable> var = this->parent->resolve(name);
		var->offset += this->m_frame_size;
		return var;
	}
	return nullptr;
}

StackEnvironment::StackEnvironment()
	:m_frame_size(0), m_function_name(nullptr), parent(nullptr), child(nullptr)
{}

StackEnvironment::StackEnvironment(StackEnvironment* parent)
	:m_frame_size(0), m_function_name(nullptr), parent(parent), child(nullptr)
{}

StackEnvironment::~StackEnvironment()
{
	if (child)
	{
		delete child;
	}
	if (parent)
	{
		parent->child = nullptr;
	}
}

bool StackEnvironment::is_in_function() const
{
	if (this->m_function_name)
	{
		return true;
	}
	if (!this->parent)
	{
		return false;
	}
	return this->parent->is_in_function();
}

const std::string& StackEnvironment::function_name() const
{
	if (this->m_function_name)
	{
		return *this->m_function_name;
	}
	if (!this->parent)
	{
		throw std::runtime_error("Attempted to get name of function while not inside function.");
	}
	return this->parent->function_name();
}

void StackEnvironment::define(const std::string& name, int size)
{
	for (auto& var : this->variables)
	{
		var.second.offset += size;
	}
	this->variables.emplace(name, StackVariable{ name, 0, size });
	this->m_frame_size += size;
}

void StackEnvironment::set_frame_size(int value)
{
	this->m_frame_size = value;
}

int StackEnvironment::frame_size() const
{
	return this->m_frame_size;
}

StackEnvironment* StackEnvironment::spawn()
{
	StackEnvironment* env = new StackEnvironment(this);
	this->child = env;
	return env;
}

StackEnvironment* StackEnvironment::spawn_in_function(const std::string& name)
{
	if (this->child)
	{
		throw std::runtime_error("Attempted to spawn while having a child.");
	}
	StackEnvironment* child = new StackEnvironment(this);
	this->child = child;
	child->m_function_name = std::make_unique<std::string>(name);
	return child;
}

StackEnvironment* StackEnvironment::pop()
{
	if (!this->parent)
	{
		throw std::runtime_error("Cannot pop top-level environment");
	}
	StackEnvironment* parent = this->parent;
	delete parent->child;
	return parent;
}

StackEnvironment* StackEnvironment::pop_to_function()
{
	if (!parent)
	{
		throw std::runtime_error("Cannot pop top-level environment");
	}
	StackEnvironment* parent = this;
	while (!parent->m_function_name)
	{
		parent = parent->parent;
		if (!parent)
		{
			throw std::runtime_error("Attempted to call StackEnvironment::pop_to_function() from an environment that is not inside a function.");
		}
	}
	if (parent->child)
	{
		delete parent->child;
	}
	return parent;
}

StackEnvironment& StackEnvironment::operator=(StackEnvironment&& other) noexcept
{
	if (this != &other)
	{
		this->m_frame_size = other.m_frame_size;
		this->parent = std::move(other.parent);
		this->child = std::move(other.child);
		this->variables = std::move(other.variables);
		this->m_function_name = std::move(other.m_function_name);
	}
	return *this;
}

Register::Register(RegisterHandle handle)
	:register_handle(handle)
{}

Register::~Register()
{}

RegisterHandle Register::handle()
{
	return this->register_handle;
}

RegisterHandle* Register::release()
{
	return new RegisterHandle(std::move(this->register_handle));
}

int Register::index() const
{
	return *this->register_handle;
}

RegisterAllocator::RegisterAllocator(size_t register_count)
{
	this->registers.reserve(register_count);
	for (int i = 0; i < register_count; i++)
	{
		this->registers.push_back(std::make_shared<int>(i));
	}
}

RegisterAllocator::~RegisterAllocator()
{}

Register RegisterAllocator::allocate()
{
	for (size_t i = 0; i < this->registers.size(); i++)
	{
		const auto& reg = this->registers[i];
		if (reg.use_count() == 1)
		{
			return Register(reg);
		}
	}
	throw std::runtime_error("Register allocation failed. No available registers.");
}

CodeGenerator::CodeGenerator(Compiler& compiler, TypeCheckedProgram& program)
	:compiler(compiler), m_program(program), allocator(16), top_env(), env(nullptr), current_placeholder_value(0)
{
	this->env = &top_env;
}

void CodeGenerator::error(const Token& token, const std::string& str)
{
	this->compiler.error(token.line, str);
	throw CodeGenerationError();
}

void CodeGenerator::push_env()
{
	this->env = this->env->spawn();
}

void CodeGenerator::push_env(const std::string& name)
{
	this->env = this->env->spawn_in_function(name);
}

void CodeGenerator::pop_env()
{
	this->env = this->env->pop();
}

static void find_and_replace_in(std::string& str, const std::string& replace, const std::string& with)
{
	size_t found = 0;
	while ((found = str.find(replace, found)) != std::string::npos)
	{
		str.replace(found, replace.length(), with);
		found += with.length();
	}
}

std::string CodeGenerator::generate()
{
	try
	{
		this->pass = Pass::GlobalLinkage;
		for (auto& stmt : this->m_program.statements())
		{
			this->visit_stmt(stmt);
		}
		
		this->emit_raw("push -1\n");
		this->emit_raw("jal ");
		this->emit_raw(this->m_program.env().root()->get_variable(Identifier("main"))->full_type().mangled_name());
		this->emit_raw("\n");
		this->emit_raw("jr -1\n");

		this->pass = Pass::FunctionLinkage;
		for (auto& stmt : this->m_program.statements())
		{
			this->visit_stmt(stmt);
		}
	}
	catch (std::exception& e)
	{
		this->compiler.error(-1, e.what());
	}
	catch(CodeGenerationError& e)
	{
		this->compiler.error(-1, "Error detected, aborting code generation.");
		return "";
	}
	size_t found = 0;
	while ((found = this->code.find("\n@function", found)) != std::string::npos)
	{
		found = found + 1;
		size_t end = this->code.find(':', found);
		std::string identifier = this->code.substr(found, end - found);
		printf("%s\n", identifier.c_str());
		this->code.erase(found, end - found + 1);
		int line = std::count(this->code.begin(), this->code.begin() + found, '\n');
		find_and_replace_in(this->code, identifier, std::to_string(line));
	}
	return std::move(this->code);
}

void CodeGenerator::emit_raw(const std::string& val)
{
	this->code += val;
}

void CodeGenerator::emit_register_use(const Register& reg)
{
	this->emit_raw("r");
	this->emit_raw(std::to_string(reg.index()));
}

void CodeGenerator::emit_register_use(const Register& a, const Register& b)
{
	this->emit_register_use(a);
	this->emit_raw(" ");
	this->emit_register_use(b);
}

void CodeGenerator::emit_peek_stack_from_into(Register* reg)
{
	Register prev_stack_ptr = this->allocator.allocate();
	this->emit_raw("move ");
	this->emit_register_use(prev_stack_ptr);
	this->emit_raw(" sp");
	this->emit_raw("\n");

	this->emit_raw("move sp ");
	this->emit_register_use(*reg);
	this->emit_raw("\n");
	
	this->emit_raw("add sp sp 1");
	
	this->emit_raw("peek ");
	this->emit_register_use(*reg);
	this->emit_raw("\n");

	this->emit_raw("move sp ");
	this->emit_register_use(prev_stack_ptr);
	this->emit_raw("\n");
}

std::unique_ptr<RegisterHandle> CodeGenerator::visit_expr_raw(std::shared_ptr<Expr> expr)
{
	return std::unique_ptr<RegisterHandle>(static_cast<RegisterHandle*>(expr->accept(*this)));
}

Register CodeGenerator::visit_expr(std::shared_ptr<Expr> expr)
{
	std::unique_ptr<RegisterHandle> handle = this->visit_expr_raw(expr);
	if (!handle)
	{
		throw std::runtime_error("No temporary register returned from expr");
	}
	return Register(*handle);
}

void* CodeGenerator::visitExprBinary(Expr::Binary& expr)
{
	std::unique_ptr<RegisterHandle> left_temporary_handle = this->visit_expr_raw(expr.left);
	std::unique_ptr<RegisterHandle> right_temporary_handle = this->visit_expr_raw(expr.right);
	if (!left_temporary_handle || !right_temporary_handle)
	{
		throw std::runtime_error("Binary operation requires two values.");
	}
	Register left_temporary(*left_temporary_handle);
	Register right_temporary(*right_temporary_handle);
	switch (expr.op.type)
	{
	case TokenType::PLUS:
		this->emit_raw("add ");
		break;
	case TokenType::MINUS:
		this->emit_raw("sub ");
		break;
	case TokenType::STAR:
		this->emit_raw("mul ");
		break;
	case TokenType::SLASH:
		this->emit_raw("div ");
		break;
	case TokenType::GREATER:
		this->emit_raw("sgt ");
		break;
	case TokenType::LESS:
		this->emit_raw("slt ");
		break;
	case TokenType::GREATER_EQUAL:
		this->emit_raw("sge ");
		break;
	case TokenType::LESS_EQUAL:
		this->emit_raw("sle ");
		break;
	default:
		throw std::runtime_error("Binary operation had non +-*/ type.");
	}
	// store result in left temporary
	this->emit_register_use(left_temporary);
	this->emit_raw(" ");
	this->emit_register_use(left_temporary, right_temporary);
	this->emit_raw("\n");
	return left_temporary.release();
}

void* CodeGenerator::visitExprGrouping(Expr::Grouping& expr)
{
	return this->visit_expr(expr.expression).release();
}

void* CodeGenerator::visitExprLiteral(Expr::Literal& expr)
{
	Register reg = allocator.allocate();
	this->emit_raw("move ");
	this->emit_register_use(reg);
	this->emit_raw(" ");
	if (expr.literal.literal.string)
	{
		throw std::runtime_error("Attempted to generate instruction for using a string literal.");
	}
	else if (expr.literal.literal.boolean)
	{
		if (*expr.literal.literal.boolean) this->emit_raw("1");
		else this->emit_raw("0");
	}
	else if (expr.literal.literal.number)
	{
		this->emit_raw(std::to_string(*expr.literal.literal.number));
	}
	this->emit_raw("\n");
	return reg.release();
}

void* CodeGenerator::visitExprUnary(Expr::Unary& expr)
{
	Register reg = this->visit_expr(expr.right);
	switch (expr.op.type)
	{
	case TokenType::BANG:
		this->emit_raw("seqz ");
		this->emit_register_use(reg, reg);
		this->emit_raw("\n");
		break;
	case TokenType::AMPERSAND:
		if (!expr.right->is<Expr::Variable>())
		{
			this->error(expr.op, "Cannot get address of temporary.");
			throw CodeGenerationError();
		}
		// TODO
		break;
	case TokenType::BAR:
		this->emit_peek_stack_from_into(&reg);
		break;
	default:
		throw std::runtime_error("Attempt to generate unary operation failed, invalid operation.");
	}
	return reg.release();
}

std::string CodeGenerator::get_register_name(const Register& reg)
{
	return std::string("r") + std::to_string(reg.index());
}

void CodeGenerator::emit_load_into(int offset, const std::string& register_label)
{
	if (offset == 0)
	{
		this->emit_raw("peek ");
		this->emit_raw(register_label);
		this->emit_raw("\n");
		return;
	}
	this->emit_raw("sub sp sp ");
	this->emit_raw(std::to_string(offset));
	this->emit_raw("\n");
	this->emit_raw("peek ");
	this->emit_raw(register_label);
	this->emit_raw("\n");
	this->emit_raw("add sp sp ");
	this->emit_raw(std::to_string(offset));
	this->emit_raw("\n");
}

void CodeGenerator::emit_load_into(int offset, Register* reg)
{
	this->emit_load_into(offset, this->get_register_name(*reg));
}

void CodeGenerator::emit_store_into(int offset, const Register& source)
{
	if (offset == 0)
	{
		this->emit_raw("sub sp sp 1\n");

		this->emit_raw("push ");
		this->emit_register_use(source);
		this->emit_raw("\n");
		return;
	}
	this->emit_raw("sub sp sp ");
	this->emit_raw(std::to_string(offset + 1));
	this->emit_raw("\n");

	this->emit_raw("push ");
	this->emit_register_use(source);
	this->emit_raw("\n");

	this->emit_raw("add sp sp ");
	this->emit_raw(std::to_string(offset));
	this->emit_raw("\n");
}

void* CodeGenerator::visitExprVariable(Expr::Variable& expr)
{
	Register reg = this->allocator.allocate();
	std::unique_ptr<StackVariable> var = this->env->resolve(expr.name.lexeme);
	if (!var)
	{
		throw std::runtime_error("Attempt to use undefined variable.");
	}
	this->emit_load_into(var->offset, &reg);
	return reg.release();
}

void* CodeGenerator::visitExprAssignment(Expr::Assignment& expr)
{
	Register value = this->visit_expr(expr.value);
	std::unique_ptr<StackVariable> var = this->env->resolve(expr.name.lexeme);
	if (!var)
	{
		throw std::runtime_error("Attempt to use undefined variable.");
	}
	this->emit_store_into(var->offset, value);
	return nullptr;
}

void* CodeGenerator::visitStmtReturn(Stmt::Return& expr)
{
	this->comment("return statement for ");
	this->comment(this->env->function_name().substr(sizeof("@function")));

	int return_address_offset = this->env->resolve("@return")->offset;
	this->emit_load_into(return_address_offset, "ra");

	std::unique_ptr<Register> return_value;

	// push return value
	if (expr.value)
	{
		return_value = std::make_unique<Register>(this->visit_expr(expr.value));
	}

	StackEnvironment* env_to_pop = this->env;
	int stack_values_to_pop = 0;
	while (env_to_pop->is_in_function())
	{
		stack_values_to_pop += env_to_pop->frame_size();
		env_to_pop = env_to_pop->parent;
	}

	if (stack_values_to_pop > 0)
	{
		this->emit_raw("sub sp sp ");
		this->emit_raw(std::to_string(stack_values_to_pop));
		this->emit_raw("\n");
	}

	if (return_value)
	{
		this->emit_raw("push ");
		this->emit_register_use(*return_value);
		this->emit_raw("\n");
	}
	else
	{
		this->emit_raw("push 0\n");
	}

	// jump to return address (loaded from @return)
	this->emit_raw("j ra\n");

	return nullptr;
}

void* CodeGenerator::visitStmtFunction(Stmt::Function& expr)
{
	// function definitons are only on top level
	std::string name = this->m_program.env().root()->get_variable(expr.name.lexeme)->full_type().mangled_name();

	this->comment("Function definition for");
	this->comment(name.substr(sizeof("@function")));

	this->emit_raw(name);
	this->emit_raw(":");
	this->push_env(name);

	// get all arguments
	for (const auto& param : expr.params)
	{
		this->env->define(param.name.lexeme, 1);
	}

	// then get return address
	this->env->define("@return", 1);

	for (auto& stmt : expr.body)
	{
		this->visit_stmt(stmt);
	}

	this->env = this->env->pop_to_function();
	this->pop_env();

	return nullptr;
}

void* CodeGenerator::visitExprCall(Expr::Call& expr)
{
	std::string name;
	if (expr.callee->is<Expr::Variable>())
	{
		const Variable* var = this->m_program.env().root()->get_variable(dynamic_cast<Expr::Variable*>(expr.callee.get())->name.lexeme);
		if (!var)
		{
			throw std::runtime_error("Attempted to call a non-existent function.");
		}
		name = var->full_type().mangled_name();
		this->comment("calling function");
		this->comment(var->full_type().unmangled_name());
	}
	else
	{
		throw std::runtime_error("Non-static functions not implemented yet.");
	}

	// pushes all arguments
	// then pushes return address

	for (auto& arg : expr.arguments)
	{
		Register loaded = this->visit_expr(arg);
		this->emit_raw("push ");
		this->emit_register_use(loaded);
		this->emit_raw("\n");
	}
	this->emit_raw("push ra\n");
	
	this->emit_raw("jal ");
	this->emit_raw(name);
	this->emit_raw("\n");

	Register return_value = this->allocator.allocate();
	this->emit_raw("pop ");
	this->emit_register_use(return_value);
	this->emit_raw("\n");
	
	return return_value.release();
}

void* CodeGenerator::visitExprLogical(Expr::Logical& expr)
{
	Register left = this->visit_expr(expr.left);
	Register right = this->visit_expr(expr.right);
	switch (expr.op.type)
	{
	case TokenType::AND:
		this->emit_raw("min ");
		this->emit_register_use(left);
		this->emit_raw(" ");
		this->emit_register_use(left, right);
		this->emit_raw("\n");
		break;
	case TokenType::OR:
		this->emit_raw("max ");
		this->emit_register_use(left);
		this->emit_raw(" ");
		this->emit_register_use(left, right);
		this->emit_raw("\n");
		break;
	default:
		throw std::runtime_error("");
	}
	return left.release();
}

void CodeGenerator::visit_stmt(std::unique_ptr<Stmt>& stmt)
{
	if (this->pass == Pass::GlobalLinkage)
	{
		if (typeid(*stmt) == typeid(Stmt::Variable))
		{
			stmt->accept(*this);
		}
		return;
	}
	if (this->pass == Pass::FunctionLinkage)
	{
		if (!this->env->is_in_function() && typeid(*stmt) == typeid(Stmt::Variable))
		{
			return;
		}
		stmt->accept(*this);
		return;
	}
}

void* CodeGenerator::visitStmtExpression(Stmt::Expression& expr)
{
	std::unique_ptr<RegisterHandle> tmp(this->visit_expr_raw(expr.expression));

	return nullptr;
}

void* CodeGenerator::visitStmtAsm(Stmt::Asm& expr)
{
	if (!expr.literal->is<Expr::Literal>())
	{
		throw std::runtime_error("ASM statement was non-literal");
	}
	Expr::Literal& str = *dynamic_cast<Expr::Literal*>(expr.literal.get());
	if (!str.literal.literal.string)
	{
		throw std::runtime_error("ASM statement was non-string");
	}
	this->emit_raw(*str.literal.literal.string);
	this->emit_raw("\n");

	return nullptr;
}

void* CodeGenerator::visitStmtPrint(Stmt::Print& expr)
{
	return nullptr;
}

void* CodeGenerator::visitStmtVariable(Stmt::Variable& expr)
{
	Register value = this->visit_expr(expr.initalizer);
	this->emit_raw("push ");
	this->emit_register_use(value);
	this->emit_raw("\n");

	this->env->define(expr.name.lexeme, 1);

	return nullptr;
}

void* CodeGenerator::visitStmtBlock(Stmt::Block& expr)
{
	this->push_env();
	for (auto& stmt : expr.statements)
	{
		this->visit_stmt(stmt);
		if (stmt->is<Stmt::Return>())
		{
			return nullptr;
		}
	}
	if (this->env->frame_size() > 0)
	{
		this->emit_raw("sub sp sp ");
		this->emit_raw(std::to_string(this->env->frame_size()));
		this->emit_raw("\n");
	}
	this->pop_env();

	return nullptr;
}


Placeholder CodeGenerator::emit_placeholder()
{
	int val = this->current_placeholder_value++;
	this->emit_raw("__@placeholder#");
	this->emit_raw(std::to_string(val));
	return Placeholder{ val };
}

void CodeGenerator::emit_replace_placeholder(const Placeholder& placeholder, const std::string& val)
{
	std::string placeholder_text = std::string("__@placeholder#") + std::to_string(placeholder.id);
	size_t placeholder_pos = this->code.find(placeholder_text);
	if (placeholder_pos == std::string::npos)
	{
		throw std::runtime_error("Attempted to replace a placeholder which does not exist.");
	}
	this->code.erase(placeholder_pos, placeholder_text.length());
	this->code.insert(placeholder_pos, val);
}

int CodeGenerator::current_line()
{
	return static_cast<int>(std::count(this->code.begin(), this->code.end(), '\n'));
}

void* CodeGenerator::visitStmtIf(Stmt::If& expr)
{
	Register value = this->visit_expr(expr.condition);
	
	this->emit_raw("breqz ");
	this->emit_register_use(value);
	this->emit_raw(" ");
	Placeholder placeholder = this->emit_placeholder();
	this->emit_raw("\n");

	int length_before = this->current_line();
	if (expr.branch_false)
	{
		this->visit_stmt(expr.branch_false);
	}
	this->visit_stmt(expr.branch_true);
	int jump_length = this->current_line() - length_before;
	this->emit_replace_placeholder(placeholder, std::to_string(jump_length));

	this->visit_stmt(expr.branch_true);

	return nullptr;
}

void CodeGenerator::comment(const std::string& val)
{
	this->emit_raw("#");
	this->emit_raw(val);
	this->emit_raw("\n");
}

void* CodeGenerator::visitStmtWhile(Stmt::While& expr)
{
	if (expr.condition->is<Expr::Literal>())
	{
		Expr::Literal& condition = *dynamic_cast<Expr::Literal*>(expr.condition.get());
		if (*condition.literal.literal.boolean)
		{
			int line = this->current_line();
			// while (true)
			this->visit_stmt(expr.body);
			this->emit_raw("j ");
			this->emit_raw(std::to_string(line));
			this->emit_raw("\n");
		}
		return nullptr;
	}
	int line = this->current_line();
	Register condition = this->visit_expr(expr.condition);
	
	this->emit_raw("brgtz ");
	this->emit_register_use(condition);
	this->emit_raw(" ");
	Placeholder placeholder = this->emit_placeholder();
	this->emit_raw("\n");
	
	this->visit_stmt(expr.body);

	this->emit_raw("j ");
	this->emit_raw(std::to_string(line));
	this->emit_raw("\n");

	this->emit_replace_placeholder(placeholder, std::to_string(this->current_line() - line));

	return nullptr;
}
