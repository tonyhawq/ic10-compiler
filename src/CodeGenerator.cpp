#include "CodeGenerator.h"
#include "Compiler.h"


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

RegisterHandle* CodeGenerator::visit(std::shared_ptr<Expr> expr)
{
	return static_cast<RegisterHandle*>(expr->accept(*this));
}

void* CodeGenerator::visitExprBinary(Expr::Binary& expr)
{
	std::unique_ptr<RegisterHandle> left_temporary_handle = std::unique_ptr<RegisterHandle>(this->visit(expr.left));
	std::unique_ptr<RegisterHandle> right_temporary_handle = std::unique_ptr<RegisterHandle>(this->visit(expr.right));
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
	return this->visit(expr.expression);
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
	std::unique_ptr<RegisterHandle> handle = std::unique_ptr<RegisterHandle>(this->visit(expr.right));
	if (!handle)
	{
		throw std::runtime_error("Attempt to generate unary operation failed, register handle was nullptr.");
	}
	Register reg(*handle);
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

void* CodeGenerator::visitExprVariable(Expr::Variable& expr)
{
	Register reg = this->allocator.allocate();
	// TODO
}

void* CodeGenerator::visitExprAssignment(Expr::Assignment& expr)
{

}

void* CodeGenerator::visitExprCall(Expr::Call& expr)
{

}

void* CodeGenerator::visitExprLogical(Expr::Logical& expr)
{

}

void* CodeGenerator::visitStmtExpression(Stmt::Expression& expr)
{

}

void* CodeGenerator::visitStmtAsm(Stmt::Asm& expr)
{

}

void* CodeGenerator::visitStmtPrint(Stmt::Print& expr)
{

}

void* CodeGenerator::visitStmtVariable(Stmt::Variable& expr)
{

}

void* CodeGenerator::visitStmtBlock(Stmt::Block& expr)
{

}

void* CodeGenerator::visitStmtIf(Stmt::If& expr)
{

}

void* CodeGenerator::visitStmtFunction(Stmt::Function& expr)
{

}

void* CodeGenerator::visitStmtReturn(Stmt::Return& expr)
{

}

void* CodeGenerator::visitStmtWhile(Stmt::While& expr)
{

}
