#include "Interpreter.h"

Literal* Interpreter::evaluate(Expr& expression)
{
	return static_cast<Literal*>(expression.accept(*this));
}

bool Interpreter::is_truthy(Literal& value)
{
	if (value.boolean)
	{
		return *value.boolean;
	}
	if (value.number)
	{
		return (*value.number) >= 1.0;
	}
	throw std::runtime_error("Attempt to see if string is truthy");
}

bool Interpreter::is_equal(Literal& a, Literal& b)
{
	if (a.boolean && b.boolean)
	{
		return (*a.boolean) == (*b.boolean);
	}
	if (a.number && b.number)
	{
		return (*a.number) == (*b.number);
	}
	if (a.string && b.string)
	{
		return (*a.string) == (*b.string);
	}
	throw std::runtime_error("Attempted to compare two different types.");
}

void* Interpreter::visitExprBinary(Expr::Binary& expr)
{
	Literal* left = this->evaluate(*expr.left);
	Literal* right = this->evaluate(*expr.right);
	switch (expr.op.type)
	{
	case TokenType::MINUS:
	{
		Literal* result = new Literal((*left->number) - (*right->number));
		delete left;
		delete right;
		return result;
	}
	case TokenType::PLUS:
	{
		Literal* result = new Literal((*left->number) + (*right->number));
		delete left;
		delete right;
		return result;
	}
	case TokenType::SLASH:
	{
		Literal* result = new Literal((*left->number) / (*right->number));
		delete left;
		delete right;
		return result;
	}
	case TokenType::STAR:
	{
		Literal* result = new Literal((*left->number) * (*right->number));
		delete left;
		delete right;
		return result;
	}
	case TokenType::GREATER:
	{
		Literal* result = new Literal((*left->number) > (*right->number));
		delete left;
		delete right;
		return result;
	}
	case TokenType::LESS:
	{
		Literal* result = new Literal((*left->number) < (*right->number));
		delete left;
		delete right;
		return result;
	}
	case TokenType::GREATER_EQUAL:
	{
		Literal* result = new Literal((*left->number) >= (*right->number));
		delete left;
		delete right;
		return result;
	}
	case TokenType::LESS_EQUAL:
	{
		Literal* result = new Literal((*left->number) <= (*right->number));
		delete left;
		delete right;
		return result;
	}
	case TokenType::EQUAL_EQUAL:
	{
		Literal* result = new Literal(this->is_equal(*left, *right));
		delete left;
		delete right;
		return result;
	}
	case TokenType::BANG_EQUAL:
	{
		Literal* result = new Literal(!this->is_equal(*left, *right));
		delete left;
		delete right;
		return result;
	}
	}
	return nullptr;
}

void* Interpreter::visitExprVariable(Expr::Variable& expr)
{
	return new Literal(42.0);
}

void* Interpreter::visitExprAssignment(Expr::Assignment& expr)
{
	Literal* value = this->evaluate(*expr.value);
	// assign
	return value;
}

void* Interpreter::visitExprGrouping(Expr::Grouping& expr)
{
	return this->evaluate(*expr.expression);
}

void* Interpreter::visitExprLiteral(Expr::Literal& expr)
{
	return new Literal(expr.literal.literal);
}

void* Interpreter::visitExprUnary(Expr::Unary& expr)
{
	Literal* right = this->evaluate(*expr.right);
	switch (expr.op.type)
	{
	case TokenType::BANG:
	{
		bool truthy = this->is_truthy(*right);
		delete right;
		right = new Literal(truthy);
		return right;
	}
	case TokenType::MINUS:
	{
		(*right->number) *= -1;
		return right;
	}
	}
	return nullptr;
}

void* Interpreter::visitStmtExpression(Stmt::Expression& stmt)
{
	delete this->evaluate(*stmt.expression);
	return nullptr;
}

void* Interpreter::visitStmtAsm(Stmt::Asm& stmt)
{
	// TODO
	return nullptr;
}

void* Interpreter::visitStmtPrint(Stmt::Print& stmt)
{
	Literal* literal = this->evaluate(*stmt.expression);
	printf("PRINT: %s\n", literal->display().c_str());
	delete literal;
	return nullptr;
}
