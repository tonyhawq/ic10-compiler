#include "CodeGenerator.h"
#include "Compiler.h"

std::unique_ptr<StackVariable> StackEnvironment::resolve(const std::string& name)
{
	const auto& val = this->variables.find(name);
	if (val != this->variables.end())
	{
		std::unique_ptr<StackVariable> var = std::make_unique<StackVariable>(this->variables.at(name));
		if (var->is_static)
		{
			var->offset = -(var->offset + 1);
		}
		return var;
	}
	if (this->parent)
	{
		std::unique_ptr<StackVariable> var = this->parent->resolve(name);
		if (var->is_static)
		{
			return var;
		}
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

void StackEnvironment::forget(const std::string& name)
{
	for (auto iterator = this->variables.begin(); iterator != this->variables.end(); iterator++)
	{
		if (iterator->first == name)
		{
			size_t id = iterator->second.id;
			int size = iterator->second.size;
			this->variables.erase(iterator);
			for (size_t i = 0; i < id; i++)
			{
				this->variable_list[i]->offset -= size;
			}
			this->m_frame_size -= size;
			this->variable_list.erase(this->variable_list.begin() + id);
			for (size_t i = 0; i < this->variable_list.size(); i++)
			{
				this->variable_list[i]->id = i;
			}
			return;
		}
	}
	throw std::runtime_error(std::string("Attempted to forget a non-existent variable: ") + name);
}

StackVariable& StackEnvironment::define(const std::string& name, int size)
{
	for (auto& var : this->variables)
	{
		var.second.offset += size;
	}
	this->variables.emplace(name, StackVariable(name, size));
	StackVariable* var = &this->variables.at(name);
	var->id = this->variable_list.size();
	this->variable_list.push_back(var);
	this->m_frame_size += size;
	return *var;
}

StackVariable& StackEnvironment::define_static(const std::string& name, int size)
{
	StackVariable& var = this->define(name, size);
	var.is_static = true;
	return var;
}

void StackEnvironment::set_frame_size(int value)
{
	this->m_frame_size = value;
}

int StackEnvironment::frame_size() const
{
	return this->m_frame_size;
}

const std::vector<StackVariable*>& StackEnvironment::see_variables() const
{
	return this->variable_list;
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

Register::Register(const Register& other)
	:register_handle(other.register_handle)
{}

Register::Register(Register&& other) noexcept
	:register_handle(std::move(other.register_handle))
{}

std::string Register::to_string() const
{
	return std::string("r") + std::to_string(this->index());
}

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

RegisterOrLiteral::RegisterOrLiteral(const Register& reg)
	:m_reg(std::make_unique<Register>(reg)), m_literal(nullptr)
{}

RegisterOrLiteral::RegisterOrLiteral(const Literal& literal)
	:m_literal(std::make_unique<Literal>(literal)), m_reg(nullptr)
{}

RegisterOrLiteral::RegisterOrLiteral(RegisterOrLiteral&& other) noexcept
	:m_literal(std::move(other.m_literal)), m_reg(std::move(other.m_reg))
{}

bool RegisterOrLiteral::is_register() const
{
	return this->m_reg.get();
}

bool RegisterOrLiteral::is_literal() const
{
	return this->m_literal.get();
}

Register& RegisterOrLiteral::get_register()
{
	if (!this->is_register())
	{
		throw std::runtime_error("Attempted to get register while this was not a register.");
	}
	return *this->m_reg;
}

Literal& RegisterOrLiteral::get_literal()
{
	if (!this->is_literal())
	{
		throw std::runtime_error("Attempted to get literal while this was not a literal.");
	}
	return *this->m_literal;
}

const Register& RegisterOrLiteral::get_register() const
{
	if (!this->is_register())
	{
		throw std::runtime_error("Attempted to get register while this was not a register.");
	}
	return *this->m_reg;
}

const Literal& RegisterOrLiteral::get_literal() const
{
	if (!this->is_literal())
	{
		throw std::runtime_error("Attempted to get literal while this was not a literal.");
	}
	return *this->m_literal;
}

std::string RegisterOrLiteral::to_string() const
{
	if (this->is_literal())
	{
		Literal literal = *this->m_literal;
		if (literal.is_string())
		{
			return literal.as_string();
		}
		if (literal.is_hashstring())
		{
			throw std::runtime_error("NOT IMPLEMENTED");
		}
		if (literal.is_number())
		{
			return std::to_string(literal.as_number());
		}
		if (literal.is_boolean())
		{
			return literal.as_boolean() ? "1" : "0";
		}
	}
	if (this->is_register())
	{
		return this->m_reg->to_string();
	}
	throw std::runtime_error("Attempted to call ::to_string on an invalid RegisterOrLiteral.");
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

std::vector<Register> RegisterAllocator::registers_in_use() const
{
	std::vector<Register> used_registers;
	used_registers.reserve(this->registers.size());
	for (size_t i = 0; i < this->registers.size(); i++)
	{
		const auto& reg = this->registers[i];
		if (reg.use_count() > 1)
		{
			used_registers.push_back(Register(reg));
		}
	}
	return used_registers;
}

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
		
		this->emit_raw("jal ");
		this->emit_raw(this->m_program.env().root()->get_variable(Identifier("main"))->full_type().mangled_name());
		this->emit_raw("\n");

		this->emit_raw("sub sp sp 1\n");
		this->emit_raw("jr -2\n");

		printf("entering function linkage\n");

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
	this->emit_raw(reg.to_string());
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
	
	this->emit_raw("add sp sp 1\n");
	
	this->emit_raw("peek ");
	this->emit_register_use(*reg);
	this->emit_raw("\n");

	this->emit_raw("move sp ");
	this->emit_register_use(prev_stack_ptr);
	this->emit_raw("\n");
}

std::unique_ptr<RegisterOrLiteral> CodeGenerator::visit_expr_raw(std::shared_ptr<Expr> expr)
{
	return std::unique_ptr<RegisterOrLiteral>(static_cast<RegisterOrLiteral*>(expr->accept(*this)));
}

std::unique_ptr<RegisterOrLiteral> CodeGenerator::visit_expr(std::shared_ptr<Expr> expr)
{
	std::unique_ptr<RegisterOrLiteral> handle = this->visit_expr_raw(expr);
	if (!handle)
	{
		throw std::runtime_error("No register or literal returned from expr");
	}
	return handle;
}

void* CodeGenerator::visitExprBinary(Expr::Binary& expr)
{
	std::unique_ptr<RegisterOrLiteral> left_temporary_handle = this->visit_expr_raw(expr.left);
	std::unique_ptr<RegisterOrLiteral> right_temporary_handle = this->visit_expr_raw(expr.right);
	if (!left_temporary_handle || !right_temporary_handle)
	{
		throw std::runtime_error("Binary operation requires two values.");
	}
	RegisterOrLiteral& left_temporary = *left_temporary_handle;
	RegisterOrLiteral& right_temporary = *right_temporary_handle;
	Register output = this->get_or_make_output_register(left_temporary, right_temporary);
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
	case TokenType::EQUAL_EQUAL:
		this->emit_raw("seq ");
		break;
	case TokenType::BANG_EQUAL:
		this->emit_raw("sne ");
		break;
	default:
		throw std::runtime_error("Binary operation had invalid type.");
	}
	// store result in left temporary
	this->emit_register_use(output);
	this->emit_raw(" ");
	this->emit_raw(left_temporary.to_string());
	this->emit_raw(" ");
	this->emit_raw(right_temporary.to_string());
	this->emit_raw("\n");
	return new RegisterOrLiteral(output);
}

void* CodeGenerator::visitExprGrouping(Expr::Grouping& expr)
{
	return this->visit_expr(expr.expression).release();
}

void* CodeGenerator::visitExprLiteral(Expr::Literal& expr)
{
	return new RegisterOrLiteral(expr.literal.literal);
}

void* CodeGenerator::visitExprUnary(Expr::Unary& expr)
{
	std::unique_ptr<RegisterOrLiteral> handle = this->visit_expr(expr.right);
	RegisterOrLiteral& reg = *handle;
	switch (expr.op.type)
	{
	case TokenType::BANG:
		if (reg.is_literal())
		{
			return new RegisterOrLiteral(Literal(!reg.get_literal().as_boolean()));
		}
		this->emit_raw("seqz ");
		this->emit_register_use(reg.get_register(), reg.get_register());
		this->emit_raw("\n");
		return handle.release();
	case TokenType::MINUS:
		if (reg.is_literal())
		{
			return new RegisterOrLiteral(Literal(-reg.get_literal().as_number()));
		}
		this->emit_raw("sub ");
		this->emit_raw(reg.to_string());
		this->emit_raw(" 0 ");
		this->emit_raw(reg.to_string());
		this->emit_raw("\n");
		return handle.release();
	case TokenType::AMPERSAND:
		if (!expr.right->is<Expr::Variable>())
		{
			this->error(expr.op, "Cannot get address of temporary.");
			throw CodeGenerationError();
		}
		// TODO
		break;
	case TokenType::BAR:
		// TODO
		break;
	default:
		throw std::runtime_error("Attempt to generate unary operation failed, invalid operation.");
	}
	throw std::runtime_error("Return missed while generating unary operation.");
}

std::string CodeGenerator::get_register_name(const Register& reg)
{
	return std::string("r") + std::to_string(reg.index());
}

void CodeGenerator::emit_load_into(int offset, const std::string& register_label)
{
	if (offset < 0)
	{

		Register sp = this->allocator.allocate();
		this->emit_raw("move ");
		this->emit_register_use(sp);
		this->emit_raw(" sp\n");

		// offsets are off-by-one
		// -1 is actually 0
		/*
		static fixed number device = -1; # offset 3 -> -4 # push 0 peek 1
		static number x = 42;            # offset 2 -> -3 # push 1 peek 2
		static number y = 69;            # offset 1 -> -2 # push 2 peek 3
		static number z = -42;           # offset 0 -> -1 # push 3 peek 4
		*/

		printf("For loading offset %i moving sp to %i\n", offset, this->top_env.frame_size() + offset + 1);

		this->emit_raw("move sp ");
		this->emit_raw(std::to_string(this->top_env.frame_size() + offset + 1));
		this->emit_raw("\n");

		this->emit_raw("peek ");
		this->emit_raw(register_label);
		this->emit_raw("\n");

		this->emit_raw("move sp ");
		this->emit_register_use(sp);
		this->emit_raw("\n");

		return;
	}

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

void CodeGenerator::emit_store_into(int offset, const RegisterOrLiteral& source)
{
	if (offset < 0)
	{
		printf("storing into stack\n");
		Register sp = this->allocator.allocate();
		this->emit_raw("move ");
		this->emit_register_use(sp);
		this->emit_raw(" sp \n");

		// offsets are off-by-one
		// -1 is actually 0

		this->emit_raw("move sp ");
		this->emit_raw(std::to_string(this->top_env.frame_size() + offset));
		this->emit_raw("\n");

		this->emit_raw("push ");
		this->emit_raw(source.to_string());
		this->emit_raw("\n");

		this->emit_raw("move sp ");
		this->emit_register_use(sp);
		this->emit_raw("\n");

		return;
	}

	if (offset == 0)
	{
		this->emit_raw("sub sp sp 1\n");

		this->emit_raw("push ");
		this->emit_raw(source.to_string());
		this->emit_raw("\n");
		return;
	}
	this->emit_raw("sub sp sp ");
	this->emit_raw(std::to_string(offset + 1));
	this->emit_raw("\n");

	this->emit_raw("push ");
	this->emit_raw(source.to_string());
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
	printf("loading var %s\n", expr.name.lexeme.c_str());
	this->emit_load_into(var->offset, &reg);
	return new RegisterOrLiteral(reg);
}

void* CodeGenerator::visitExprAssignment(Expr::Assignment& expr)
{
	std::unique_ptr<RegisterOrLiteral> handle = this->visit_expr(expr.value);
	RegisterOrLiteral& value = *handle;
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

	std::unique_ptr<RegisterOrLiteral> return_value;

	// push return value
	if (expr.value)
	{
		return_value = this->visit_expr(expr.value);
	}

	int return_address_offset = this->env->resolve("@return")->offset;
	this->emit_load_into(return_address_offset, "ra");

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
		this->emit_raw(return_value->to_string());
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

	// then define return address of previous function
	this->env->define("@return", 1);

	// this is loaded into @return
	this->emit_raw("push ra\n");

	for (auto& stmt : expr.body)
	{
		this->visit_stmt(stmt);
	}

	this->env = this->env->pop_to_function();
	this->pop_env();

	return nullptr;
}

void CodeGenerator::store_register_values()
{
	if (this->stored_registers.size() > 0)
	{
		throw std::runtime_error("Attempted to store register values while already having registers stored.");
	}
	this->stored_registers = this->allocator.registers_in_use();
	for (int i = 0; i < this->stored_registers.size(); i++)
	{
		const Register& reg = this->stored_registers[i];
		this->env->define(std::string("@s_") + reg.to_string(), 1);
		this->emit_raw("push ");
		this->emit_register_use(reg);
		this->emit_raw("\n");
	}
}

void CodeGenerator::restore_register_values()
{
	if (this->stored_registers.size() == 0)
	{
		return;
	}
	for (int i = this->stored_registers.size() - 1; i >= 0; i--)
	{
		const Register& reg = this->stored_registers[i];
		this->env->forget(std::string("@s_") + reg.to_string());
		this->emit_raw("pop ");
		this->emit_register_use(reg);
		this->emit_raw("\n");
	}
	this->stored_registers.clear();
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

	printf("BEFORE:\n");
	for (const auto& val : this->env->see_variables())
	{
		printf("   - var %s has offset %i\n", val->name.c_str(), val->offset);
	}

	this->comment("Storing register values");
	this->store_register_values();
	this->comment("Stored.");
	
	printf("DURING:\n");
	for (const auto& val : this->env->see_variables())
	{
		printf("   - var %s has offset %i\n", val->name.c_str(), val->offset);
	}

	for (auto& arg : expr.arguments)
	{
		std::unique_ptr<RegisterOrLiteral> loaded = this->visit_expr(arg);
		this->emit_raw("push ");
		this->emit_raw(loaded->to_string());
		this->emit_raw("\n");
	}

	this->emit_raw("jal ");
	this->emit_raw(name);
	this->emit_raw("\n");

	this->comment("Getting return value");
	Register return_value = this->allocator.allocate();
	this->emit_raw("pop ");
	this->emit_register_use(return_value);
	this->emit_raw("\n");

	this->comment("Restoring register values");
	this->restore_register_values();
	this->comment("Restored.");

	printf("AFTER:\n");
	for (const auto& val : this->env->see_variables())
	{
		printf("   - var %s has offset %i\n", val->name.c_str(), val->offset);
	}
	
	return return_value.release();
}

void* CodeGenerator::visitExprLogical(Expr::Logical& expr)
{
	std::unique_ptr<RegisterOrLiteral> left_handle = this->visit_expr(expr.left);
	std::unique_ptr<RegisterOrLiteral> right_handle = this->visit_expr(expr.right);
	RegisterOrLiteral& left = *left_handle;
	RegisterOrLiteral& right = *right_handle;
	Register output = this->get_or_make_output_register(left, right);
	switch (expr.op.type)
	{
	case TokenType::AND:
		this->emit_raw("min ");
		this->emit_register_use(output);
		this->emit_raw(" ");
		this->emit_raw(left.to_string());
		this->emit_raw(" ");
		this->emit_raw(right.to_string());
		this->emit_raw("\n");
		break;
	case TokenType::OR:
		this->emit_raw("max ");
		this->emit_register_use(output);
		this->emit_raw(" ");
		this->emit_raw(left.to_string());
		this->emit_raw(" ");
		this->emit_raw(right.to_string());
		this->emit_raw("\n");
		break;
	default:
		throw std::runtime_error("");
	}
	return new RegisterOrLiteral(output);
}

void CodeGenerator::visit_stmt(std::unique_ptr<Stmt>& stmt)
{
	if (this->pass == Pass::GlobalLinkage)
	{
		if (typeid(*stmt) == typeid(Stmt::Static))
		{
			stmt->accept(*this);
		}
		return;
	}
	if (this->pass == Pass::FunctionLinkage)
	{
		if (typeid(*stmt) == typeid(Stmt::Static))
		{
			return;
		}
		stmt->accept(*this);
		return;
	}
}

void* CodeGenerator::visitStmtExpression(Stmt::Expression& expr)
{
	this->visit_expr_raw(expr.expression);

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

void* CodeGenerator::visitStmtStatic(Stmt::Static& expr)
{
	std::unique_ptr<RegisterOrLiteral> value = this->visit_expr(expr.var->initalizer);
	this->emit_raw("push ");
	this->emit_raw(value->to_string());
	this->emit_raw("\n");
	this->env->define_static(expr.var->name.lexeme, 1);
	return nullptr;
}

void* CodeGenerator::visitStmtVariable(Stmt::Variable& expr)
{
	std::unique_ptr<RegisterOrLiteral> value = this->visit_expr(expr.initalizer);
	this->emit_raw("push ");
	this->emit_raw(value->to_string());
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
		// no need to emit code after a return statement (will never be executed)
		if (stmt->is<Stmt::Return>())
		{
			this->pop_env();
			return nullptr;;
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

Register CodeGenerator::get_or_make_output_register(const RegisterOrLiteral& a, const RegisterOrLiteral& b)
{
	if (a.is_literal() && b.is_literal())
	{
		return this->allocator.allocate();
	}
	if (a.is_register())
	{
		return a.get_register();
	}
	if (b.is_register())
	{
		return b.get_register();
	}
	throw std::runtime_error("Unreachable code.");
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
	std::unique_ptr<RegisterOrLiteral> value = this->visit_expr(expr.condition);
	
	/*

	if == 0 goto label:

	// true branch

	j label + false_length

	label:
	
	// false branch

	// final

	*/

	this->emit_raw("breqz ");
	this->emit_raw(value->to_string());
	this->emit_raw(" ");
	Placeholder placeholder = this->emit_placeholder();
	this->emit_raw("\n");

	int true_branch_length = this->current_line();
	this->visit_stmt(expr.branch_true);
	std::unique_ptr<Placeholder> jump_over_false;
	if (expr.branch_false)
	{
		this->emit_raw("jr ");
		jump_over_false = std::make_unique<Placeholder>(this->emit_placeholder());
		this->emit_raw("\n");
	}

	true_branch_length = this->current_line() - true_branch_length;
	this->emit_replace_placeholder(placeholder, std::to_string(true_branch_length + 1));

	if (expr.branch_false)
	{
		int false_branch_length = this->current_line();
		this->visit_stmt(expr.branch_false);
		false_branch_length = this->current_line() - false_branch_length;
		this->emit_replace_placeholder(*jump_over_false, std::to_string(false_branch_length + 1));
	}

	return nullptr;
}

void CodeGenerator::comment(const std::string& val)
{
	this->emit_raw("#");
	this->emit_raw(val);
	this->emit_raw("\n");
}

void* CodeGenerator::visitExprDeviceLoad(Expr::DeviceLoad& expr)
{
	std::unique_ptr<RegisterOrLiteral> device = this->visit_expr(expr.device);
	const std::string& logic_type = expr.logic_type.literal.as_string();
	Register output = this->allocator.allocate();
	this->emit_raw("l ");
	this->emit_register_use(output);
	this->emit_raw(" ");
	if (device->is_literal())
	{
		this->emit_raw("d");
		this->emit_raw(std::to_string(device->get_literal().as_integer()));
	}
	else
	{
		this->emit_raw("r");
		this->emit_raw(device->get_register().to_string());
	}
	this->emit_raw(" ");
	this->emit_raw(logic_type);
	this->emit_raw("\n");
	return new RegisterOrLiteral(output);
}

void* CodeGenerator::visitStmtDeviceSet(Stmt::DeviceSet& expr)
{
	std::unique_ptr<RegisterOrLiteral> device = this->visit_expr(expr.device);
	const std::string& logic_type = expr.logic_type.literal.as_string();
	std::unique_ptr<RegisterOrLiteral> value = this->visit_expr(expr.value);
	this->emit_raw("s ");
	if (device->is_literal())
	{
		this->emit_raw("d");
		int device_id = device->get_literal().as_integer();
		if (device_id == -1)
		{
			this->emit_raw("b");
		}
		else if (device_id > 5 || device_id < -1)
		{
			this->error(expr.token, std::string("Attempted to set device d") + std::to_string(device_id) + " which is out of range for dx {-1 <= x <= 5}.");
			throw CodeGenerationError();
		}
		else
		{
			this->emit_raw(std::to_string(device->get_literal().as_integer()));
		}
	}
	else
	{
		this->emit_raw("r");
		this->emit_raw(device->get_register().to_string());
	}
	this->emit_raw(" ");
	this->emit_raw(logic_type);
	this->emit_raw(" ");
	this->emit_raw(value->to_string());
	this->emit_raw("\n");
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
	std::unique_ptr<RegisterOrLiteral> condition = this->visit_expr(expr.condition);
	
	this->emit_raw("brlez ");
	this->emit_raw(condition->to_string());
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
