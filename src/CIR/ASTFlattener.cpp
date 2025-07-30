#include "ASTFlattener.h"

ASTFlattener::ASTFlattener(Compiler& compiler, TypeCheckedProgram prog)
	:compiler(compiler), program(prog), current_temporary(0)
{}

CIRProgram ASTFlattener::flatten_program()
{
	for (auto& stmt : this->program.statements())
	{
		this->visit(stmt);
	}
	return CIRProgram(std::move(this->instructions), std::move(this->program));
}

void ASTFlattener::visit(Stmt& stmt)
{
	stmt.accept(*this);
}

cir::Temporary ASTFlattener::flatten(Expr& expr)
{
	cir::Temporary* temp = (cir::Temporary*)expr.accept(*this);
	if (!temp)
	{
		throw std::logic_error("Sub-expression did not return a temporary.");
	}
	cir::Temporary value = *temp;
	delete temp;
	return value;
}

cir::Temporary ASTFlattener::allocate()
{
	this->current_temporary++;
	return cir::Temporary(this->current_temporary);
}

void ASTFlattener::emit(cir::Instruction::ref&& instruction)
{
	this->instructions.push_back(std::move(instruction));
}

void ASTFlattener::visit(std::unique_ptr<Stmt>& stmt)
{
	this->visit(*stmt);
}

cir::Temporary ASTFlattener::flatten(std::shared_ptr<Expr>& expr)
{
	return this->flatten(*expr);
}

void* ASTFlattener::visitStmtExpression(Stmt::Expression& expr)
{
	this->flatten(expr.expression);
	return nullptr;
}

void* ASTFlattener::visitStmtAsm(Stmt::Asm& expr)
{
	throw std::runtime_error("Not implemented");
}

void* ASTFlattener::visitStmtPrint(Stmt::Print& expr)
{
	cir::Temporary val = this->flatten(expr.expression);
	this->emit(cir::Instruction::from(new cir::Instruction::Print(val)));
	return nullptr;
}

void* ASTFlattener::visitStmtVariable(Stmt::Variable& expr)
{
	cir::Temporary reg = this->allocate();
	this->variable_mappings.emplace(&this->program.table.lookup(expr.downcast()).var(), reg);
	cir::Temporary initalizer = this->flatten(expr.initalizer);
	this->emit(cir::Instruction::from(new cir::Instruction::Move(reg, initalizer)));
	return nullptr;
}

void* ASTFlattener::visitStmtBlock(Stmt::Block& expr)
{
	for (auto& stmt : expr.statements)
	{
		this->visit(stmt);
	}
	return nullptr;
}

void* ASTFlattener::visitStmtIf(Stmt::If& expr)
{
	throw std::runtime_error("Not implemented");
}

void* ASTFlattener::visitStmtFunction(Stmt::Function& expr)
{
	throw std::runtime_error("Not implemented");
}

void* ASTFlattener::visitStmtReturn(Stmt::Return& expr)
{
	std::optional<cir::Temporary> result;
	if (expr.value)
	{
		result = this->flatten(expr.value);
	}
	this->emit(cir::Instruction::from(new cir::Instruction::Return(result)));
	return nullptr;
}

void* ASTFlattener::visitStmtWhile(Stmt::While& expr)
{
	throw std::runtime_error("Not implemented");
}

void* ASTFlattener::visitStmtStatic(Stmt::Static& expr)
{
	this->visit(expr.var);
	return nullptr;
}

void* ASTFlattener::visitExprBinary(Expr::Binary& expr)
{
	cir::Temporary left = this->flatten(expr.left);
	cir::Temporary right = this->flatten(expr.right);
	cir::Temporary output = this->allocate();
	switch (expr.op.type)
	{
	case TokenType::PLUS:
		this->emit(cir::Instruction::from(new cir::Instruction::Add(output, left, right)));
		break;
	case TokenType::MINUS:
		this->emit(cir::Instruction::from(new cir::Instruction::Sub(output, left, right)));
		break;
	case TokenType::STAR:
		this->emit(cir::Instruction::from(new cir::Instruction::Mul(output, left, right)));
		break;
	case TokenType::SLASH:
		this->emit(cir::Instruction::from(new cir::Instruction::Div(output, left, right)));
		break;
	default:
		throw std::runtime_error("Not implemented");
	}
	return new cir::Temporary(output);
}

void* ASTFlattener::visitExprGrouping(Expr::Grouping& expr)
{
	return new cir::Temporary(this->flatten(expr.expression));
}

void* ASTFlattener::visitExprLiteral(Expr::Literal& expr)
{
	cir::Temporary value = this->allocate();
	if (expr.literal.literal.is_number())
	{
		this->emit(cir::Instruction::from(new cir::Instruction::MoveImm(value, cir::Number(expr.literal.literal.as_number()))));
		return new cir::Temporary(value);
	}
	if (expr.literal.literal.is_boolean())
	{
		this->emit(cir::Instruction::from(new cir::Instruction::MoveImm(value, expr.literal.literal.as_boolean() ? cir::Number(1.0) : cir::Number(0.0))));
		return new cir::Temporary(value);
	}
	throw std::runtime_error("Not implemented");
}

void* ASTFlattener::visitExprUnary(Expr::Unary& expr)
{
	throw std::runtime_error("Not implemented");
}

void* ASTFlattener::visitExprVariable(Expr::Variable& expr)
{
	return new cir::Temporary(this->variable_mappings.at(&this->program.table.lookup(expr.downcast()).var()));
}

void* ASTFlattener::visitExprAssignment(Expr::Assignment& expr)
{
	cir::Temporary into = this->variable_mappings.at(&this->program.table.lookup(expr.downcast()).var());
	cir::Temporary from = this->flatten(expr.value);
	this->emit(cir::Instruction::from(new cir::Instruction::Move(into, from)));
	return new cir::Temporary(into);
}

void* ASTFlattener::visitExprCall(Expr::Call& expr)
{
	if (!expr.callee->is<Expr::Variable>())
	{
		throw std::runtime_error("Not implemented");
	}
	std::vector<cir::Temporary> args;
	for (auto& arg : expr.arguments)
	{
		args.push_back(this->flatten(arg));
	}
	cir::Temporary result = this->allocate();
	this->emit(cir::Instruction::from(new cir::Instruction::Call(expr.callee->as<Expr::Variable>().name.lexeme, result, args)));
	return new cir::Temporary(result);
}

void* ASTFlattener::visitExprLogical(Expr::Logical& expr)
{
	cir::Temporary left = this->flatten(expr.left);
	cir::Temporary right = this->flatten(expr.right);
	switch (expr.op.type)
	{
	case TokenType::AND:
		throw std::runtime_error("Not implemented");
	case TokenType::OR:
		throw std::runtime_error("Not implemented");
	default:
		throw std::runtime_error("Not implemented");
	}
}