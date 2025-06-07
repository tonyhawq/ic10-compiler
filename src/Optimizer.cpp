#include "Optimizer.h"
#include "Compiler.h"

#define BOOL_TO_STR(val) ((val) ? "true" : "false")
#define FOLD_INTO(thing_to_fold_into, accepted) do { void* value = accepted; if (value) { thing_to_fold_into = std::shared_ptr<Expr::Literal>(static_cast<Expr::Literal*>(value)); } }  while (0)

Optimizer::Optimizer(Compiler& compiler, TypeCheckedProgram & env)
	:compiler(compiler), m_env(env)
{}

void Optimizer::optimize()
{
	this->evaluate(this->m_env.statements());
}

void Optimizer::evaluate(std::vector<std::unique_ptr<Stmt>>& statements)
{
	for (int i = 0; i < statements.size(); i++)
	{
		std::unique_ptr<Stmt>& statement = statements[i];
		std::unique_ptr<Stmt> stmt = std::unique_ptr<Stmt>(static_cast<Stmt*>(statement->accept(*this)));
		if (stmt)
		{
			statements[i] = std::move(stmt);
		}
	}
}

static bool is_arithmentic(TokenType token)
{
	switch (token)
	{
	case TokenType::PLUS:
	case TokenType::MINUS:
	case TokenType::STAR:
	case TokenType::SLASH:
		return true;
	}
	return false;
}

static void* emit_boolean_literal(const Token& parent, bool value)
{
	TokenType type = TokenType::FALSE;
	std::string lexeme = "false";
	if (value)
	{
		type = TokenType::TRUE;
		lexeme = "true";
	}
	return new Expr::Literal(Token(parent.line, type, lexeme, value));
}

void* Optimizer::visitExprBinary(Expr::Binary& expr)
{
	{
		FOLD_INTO(expr.left, expr.left->accept(*this));
	}
	{
		FOLD_INTO(expr.right, expr.right->accept(*this));
	}
	if (expr.left->is<Expr::Literal>() && expr.right->is<Expr::Literal>())
	{
		Expr::Literal& literal_left = dynamic_cast<Expr::Literal&>(*expr.left);
		Expr::Literal& literal_right = dynamic_cast<Expr::Literal&>(*expr.right);
		if (is_arithmentic(expr.op.type))
		{
			double left = *literal_left.literal.literal.number;
			double right = *literal_right.literal.literal.number;
			double result;
			switch (expr.op.type)
			{
			case TokenType::PLUS:
				result = left + right;
				printf("emitting literal when simplifying %f + %f on line %i: %f\n", left, right, expr.op.line, result);
				break;
			case TokenType::MINUS:
				result = left - right;
				printf("emitting literal when simplifying %f - %f on line %i: %f\n", left, right, expr.op.line, result);
				break;
			case TokenType::STAR:
				result = left * right;
				printf("emitting literal when simplifying %f * %f on line %i: %f\n", left, right, expr.op.line, result);
				break;
			case TokenType::SLASH:
				result = left / right;
				printf("emitting literal when simplifying %f / %f on line %i: %f\n", left, right, expr.op.line, result);
				break;
			default:
				throw std::runtime_error("OPTIMIZER ERROR: !ARITHMETIC TOKEN WAS NOT OF ARITHMETIC TYPE!");
				break;
			}
			return new Expr::Literal(Token(expr.op.line, TokenType::NUMBER, std::to_string(result), result));
		}
		if (literal_left.literal.literal.boolean && literal_right.literal.literal.boolean)
		{
			bool left = *literal_left.literal.literal.boolean;
			bool right = *literal_right.literal.literal.boolean;
			bool result;
			switch (expr.op.type)
			{
			case TokenType::EQUAL_EQUAL:
				result = left == right;
				printf("emitting literal when simplifying %s == %s on line %i: %s\n", BOOL_TO_STR(left), BOOL_TO_STR(right), expr.op.line, BOOL_TO_STR(result));
				return emit_boolean_literal(expr.op, result);
				break;
			case TokenType::BANG_EQUAL:
				result = left != right;
				printf("emitting literal when simplifying %s != %s on line %i: %s\n", BOOL_TO_STR(left), BOOL_TO_STR(right), expr.op.line, BOOL_TO_STR(result));
				return emit_boolean_literal(expr.op, result);
				break;
			default:
				throw std::runtime_error("OPTIMIZER ERROR: !ATTEMPTED TO FOLD BAD COMPARISON BETWEEN BOOLEANS!");
				break;
			}
		}
		if (literal_left.literal.literal.number && literal_right.literal.literal.number)
		{
			double left = *literal_left.literal.literal.number;
			double right = *literal_right.literal.literal.number;
			switch (expr.op.type)
			{
			case TokenType::EQUAL_EQUAL:
				printf("emitting literal when simplifying %f == %f on line %i: %s\n", left, right, expr.op.line, (left == right) ? "true" : "false");
				return emit_boolean_literal(expr.op, left == right);
			case TokenType::BANG_EQUAL:
				printf("emitting literal when simplifying %f != %f on line %i: %s\n", left, right, expr.op.line, (left != right) ? "true" : "false");
				return emit_boolean_literal(expr.op, left != right);
			case TokenType::GREATER:
				printf("emitting literal when simplifying %f > %f on line %i: %s\n", left, right, expr.op.line, (left > right) ? "true" : "false");
				return emit_boolean_literal(expr.op, left > right);
			case TokenType::GREATER_EQUAL:
				printf("emitting literal when simplifying %f >= %f on line %i: %s\n", left, right, expr.op.line, (left >= right) ? "true" : "false");
				return emit_boolean_literal(expr.op, left >= right);
			case TokenType::LESS:
				printf("emitting literal when simplifying %f < %f on line %i: %s\n", left, right, expr.op.line, (left < right) ? "true" : "false");
				return emit_boolean_literal(expr.op, left < right);
			case TokenType::LESS_EQUAL:
				printf("emitting literal when simplifying %f <= %f on line %i: %s\n", left, right, expr.op.line, (left <= right) ? "true" : "false");
				return emit_boolean_literal(expr.op, left <= right);
			default:
				throw std::runtime_error("OPTIMIZER ERROR: !ATTEMPTED TO FOLD BAD COMPARISON BETWEEN NUMBERS!");
				break;
			}
		}
	}
	return nullptr;
}

void* Optimizer::visitExprGrouping(Expr::Grouping& expr)
{
	Expr::Literal* literal = static_cast<Expr::Literal*>(expr.expression->accept(*this));
	return literal;
}

void* Optimizer::visitExprLiteral(Expr::Literal& expr)
{
	return new Expr::Literal(expr);
}

void* Optimizer::visitExprUnary(Expr::Unary& expr)
{
	return nullptr;
}

void* Optimizer::visitExprVariable(Expr::Variable& expr)
{
	return nullptr;
}

void* Optimizer::visitExprAssignment(Expr::Assignment& expr)
{
	FOLD_INTO(expr.value, expr.value->accept(*this));
	return nullptr;
}

void* Optimizer::visitExprCall(Expr::Call& expr)
{
	for (int i = 0; i < expr.arguments.size(); i++)
	{
		auto& arg = expr.arguments[i];
		FOLD_INTO(expr.arguments[i], arg->accept(*this));
	}
	return nullptr;
}

void* Optimizer::visitExprLogical(Expr::Logical& expr)
{
	{
		FOLD_INTO(expr.left, expr.left->accept(*this));
	}
	{
		FOLD_INTO(expr.right, expr.right->accept(*this));
	}
	if (expr.left->is<Expr::Literal>() && expr.right->is<Expr::Literal>())
	{
		Expr::Literal& literal_left = dynamic_cast<Expr::Literal&>(*expr.left);
		Expr::Literal& literal_right = dynamic_cast<Expr::Literal&>(*expr.right);
		if (literal_left.literal.literal.boolean && literal_right.literal.literal.boolean)
		{
			if (expr.op.type == TokenType::AND)
			{
				bool result = (*literal_left.literal.literal.boolean) && (*literal_right.literal.literal.boolean);
				printf("emitting literal when simplifying AND between booleans on line %i: %s\n", expr.op.line, result ? "true" : "false");
				return emit_boolean_literal(expr.op, result);
			}
			else if(expr.op.type == TokenType::OR)
			{
				bool result = (*literal_left.literal.literal.boolean) || (*literal_right.literal.literal.boolean);
				printf("emitting literal when simplifying OR between booleans on line %i: %s\n", expr.op.line, result ? "true" : "false");
				return emit_boolean_literal(expr.op, result);
			}
		}
		return nullptr;
	}
	return nullptr;
}

void* Optimizer::visitStmtExpression(Stmt::Expression& stmt)
{
	FOLD_INTO(stmt.expression, stmt.expression->accept(*this));
	return nullptr;
}

void* Optimizer::visitStmtAsm(Stmt::Asm& stmt)
{
	return nullptr;
}

void* Optimizer::visitStmtPrint(Stmt::Print& stmt)
{
	return nullptr;
}

void* Optimizer::visitStmtVariable(Stmt::Variable& stmt)
{
	FOLD_INTO(stmt.initalizer, stmt.initalizer->accept(*this));
	return nullptr;
}

void* Optimizer::visitStmtBlock(Stmt::Block& stmt)
{
	this->evaluate(stmt.statements);
	return nullptr;
}

void* Optimizer::visitStmtIf(Stmt::If& stmt)
{
	Expr::Literal* condition_literal = static_cast<Expr::Literal*>(stmt.condition->accept(*this));
	if (condition_literal)
	{
		if (condition_literal->literal.literal.boolean)
		{
			if (*condition_literal->literal.literal.boolean)
			{
				printf("Branching on constant simplified to true branch.\n");
				return stmt.branch_true.release();
			}
			if (stmt.branch_false)
			{
				printf("Branching on constant simplified to false branch.\n");
				return stmt.branch_false.release();
			}
			return new Stmt::Block({});
		}
	}
	stmt.branch_true->accept(*this);
	if (stmt.branch_false)
	{
		stmt.branch_false->accept(*this);
	}
	return nullptr;
}

void* Optimizer::visitStmtFunction(Stmt::Function& expr)
{
	this->evaluate(expr.body);
	return nullptr;
}

void* Optimizer::visitStmtReturn(Stmt::Return& expr)
{
	if (expr.value)
	{
		FOLD_INTO(expr.value, expr.value->accept(*this));
	}
	return nullptr;
}