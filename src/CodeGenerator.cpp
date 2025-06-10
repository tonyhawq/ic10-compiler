#include "CodeGenerator.h"
#include "Compiler.h"

std::unique_ptr<StackVariable> StackEnvironment::resolve(const std::string& name)
{
	const auto& val = this->variables.find(name);
	if (val != this->variables.end())
	{
		return std::make_unique<StackVariable>(this->variables.at(name));
	}
	if (this->parent)
	{
		std::unique_ptr<StackVariable> var = dynamic_cast<StackEnvironment*>(this->parent)->resolve(name);
		var->offset += this->m_frame_size;
		return var;
	}
	return nullptr;
}

StackEnvironment::StackEnvironment()
	:Environment<StackVariable>(Environment::make_orphaned()), m_frame_size(0)
{}

StackEnvironment::StackEnvironment(StackEnvironment* parent)
	:Environment<StackVariable>(parent), m_frame_size(0)
{}

StackEnvironment::~StackEnvironment()
{}

void StackEnvironment::define(const std::string& name, int size)
{
	for (auto& var : this->variables)
	{
		var.second.offset += size;
	}
	this->Environment::define(name, { name, 0, size });
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

StackEnvironment& StackEnvironment::spawn()
{
	if (this->child)
	{
		throw std::runtime_error("Attempted to create child of Environment which already has a child.");
	}
	this->child = new StackEnvironment(this);
	StackEnvironment* derived = dynamic_cast<StackEnvironment*>(this->child);
	derived->function = this->function;
	return *derived;
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
	:compiler(compiler), m_program(program), allocator(16)
{}

void CodeGenerator::error(const Token& token, const std::string& str)
{
	this->compiler.error(token.line, str);
	throw CodeGenerationError();
}

std::string CodeGenerator::generate()
{
	try
	{
	}
	catch (CodeGenerationError)
	{
		this->compiler.error(-1, "Error detected, aborting code generation.");
		return "";
	}
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
	this->emit_raw("sub sp sp ");
	this->emit_raw(std::to_string(offset));
	this->emit_raw("\n");
	this->emit_raw("peek ");
	this->emit_raw(register_label);
	this->emit_raw("\n");
	this->emit_raw("add sp sp ");
	this->emit_raw(std::to_string(offset));
}

void CodeGenerator::emit_load_into(int offset, Register* reg)
{
	this->emit_load_into(offset, this->get_register_name(*reg));
}

void* CodeGenerator::visitExprVariable(Expr::Variable& expr)
{
	Register reg = this->allocator.allocate();
	std::unique_ptr<StackVariable> var = this->env.resolve(expr.name.lexeme);
	if (!var)
	{
		throw std::runtime_error("Attempt to use undefined variable.");
	}
	this->emit_load_into(var->offset, &reg);
	return reg.release();
}

void* CodeGenerator::visitExprAssignment(Expr::Assignment& expr)
{
	
}

void* CodeGenerator::visitExprCall(Expr::Call& expr)
{
	for (auto& arg : expr.arguments)
	{
		Register loaded = this->visit_expr(arg);
		this->emit_raw("push ");
		this->emit_register_use(loaded);
		this->emit_raw("\n");
	}
	this->emit_raw("push ra\n");

	this->emit_raw("jal ");
	this->emit_raw(expr.callee->type.type_name());
	this->emit_raw("\n");

	this->emit_raw("pop ra\n");
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
	stmt->accept(*this);
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

	this->env.define(expr.name.lexeme, 1);

	return nullptr;
}

void* CodeGenerator::visitStmtBlock(Stmt::Block& expr)
{
	this->env.spawn();
	for (auto& stmt : expr.statements)
	{
		this->visit_stmt(stmt);
	}
	this->env.pop();

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
	return std::count(this->code.begin(), this->code.end(), '\n');
}

void* CodeGenerator::visitStmtIf(Stmt::If& expr)
{
	Register value = this->visit_expr(expr.condition);
	
	this->emit_raw("brgtz ");
	this->emit_register_use(value);
	Placeholder placeholder = this->emit_placeholder();
	this->emit_raw("\n");

	int length_before = this->current_line();
	if (expr.branch_false)
	{
		this->visit_stmt(expr.branch_false);
	}
	int jump_length = this->current_line() - length_before;
	this->emit_replace_placeholder(placeholder, std::to_string(jump_length));

	this->visit_stmt(expr.branch_true);

	return nullptr;
}

void* CodeGenerator::visitStmtFunction(Stmt::Function& expr)
{
	// function definitons are only on top level
	this->emit_raw(TypeChecker::get_function_signature(expr.params, expr.return_type).type_name());
	this->emit_raw(":\n");
	this->env.spawn();
	for (const auto& param : expr.params)
	{
		this->env.define(param.name.lexeme, 1);
	}
	// return address
	this->env.define("@return", 1);
	for (auto& stmt : expr.body)
	{
		this->visit_stmt(stmt);
	}
	this->env.pop();

	return nullptr;
}

void* CodeGenerator::visitStmtReturn(Stmt::Return& expr)
{
	std::unique_ptr<StackVariable> return_address_offset = this->env.resolve("@return");
	if (!return_address_offset)
	{
		throw std::runtime_error("Attempted to return without a return address being defined.");
	}
	Register return_address = this->allocator.allocate();
	this->emit_load_into(return_address_offset->offset, &return_address);
	this->emit_raw("sub sp sp ");
	this->emit_raw(std::to_string(this->env.frame_size()));

	if (expr.value)
	{
		Register value = this->visit_expr(expr.value);
		this->emit_raw("push ");
		this->emit_register_use(value);
		this->emit_raw("\n");
	}
	else
	{
		this->emit_raw("push 0\n");
	}

	this->emit_raw("push ");
	this->emit_register_use(return_address);
	this->emit_raw("\n");
	this->emit_raw("j ra\n");

	this->env.pop_to_function();
	this->env.pop();

	return nullptr;
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
