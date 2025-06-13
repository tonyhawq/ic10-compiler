#include "Optimizer.h"
#include "Compiler.h"

#define BOOL_TO_STR(val) ((val) ? "true" : "false")
#define FOLD_INTO(thing_to_fold_into, accepted) do { void* value = accepted; if (value) { thing_to_fold_into = std::shared_ptr<Expr::Literal>(static_cast<Expr::Literal*>(value)); } }  while (0)

Optimizer::Optimizer(Compiler& compiler, TypeCheckedProgram& env)
	:compiler(compiler), m_env(env), local_env(env.env().root())
{}

void Optimizer::optimize()
{
	this->evaluate(this->m_env.statements());
}

void Optimizer::evaluate(std::vector<std::unique_ptr<Stmt>>& statements)
{
	for (int i = 0; i < statements.size(); i++)
	{
		std::unique_ptr<Stmt> stmt = this->visit_stmt(statements[i]);
		if (stmt)
		{
			statements[i] = std::move(stmt);
		}
	}
}

std::unique_ptr<Stmt> Optimizer::visit_stmt(std::unique_ptr<Stmt>& stmt)
{
	return std::unique_ptr<Stmt>(static_cast<Stmt*>(stmt->accept(*this)));
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
				this->compiler.info(std::string("emitting literal when simplifying ") +
					std::to_string(left) + " + " +
					std::to_string(right) + " on line " +
					std::to_string(expr.op.line) + ": " +
					std::to_string(result));
				break;
			case TokenType::MINUS:
				result = left - right;
				this->compiler.info(std::string("emitting literal when simplifying ") +
					std::to_string(left) + " - " +
					std::to_string(right) + " on line " +
					std::to_string(expr.op.line) + ": " +
					std::to_string(result));
				break;
			case TokenType::STAR:
				result = left * right;
				this->compiler.info(std::string("emitting literal when simplifying ") +
					std::to_string(left) + " * " +
					std::to_string(right) + " on line " +
					std::to_string(expr.op.line) + ": " +
					std::to_string(result));
				break;
			case TokenType::SLASH:
				result = left / right;
				this->compiler.info(std::string("emitting literal when simplifying ") +
					std::to_string(left) + " / " +
					std::to_string(right) + " on line " +
					std::to_string(expr.op.line) + ": " +
					std::to_string(result));
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
				this->compiler.info(std::string("Emitting literal when simplifying ") +
					BOOL_TO_STR(left) + " == " +
					BOOL_TO_STR(right) + " on line " +
					std::to_string(expr.op.line) + ": " +
					BOOL_TO_STR(result));
				return emit_boolean_literal(expr.op, result);
				break;
			case TokenType::BANG_EQUAL:
				result = left != right;
				this->compiler.info(std::string("Emitting literal when simplifying ") +
					BOOL_TO_STR(left) + " != " +
					BOOL_TO_STR(right) + " on line " +
					std::to_string(expr.op.line) + ": " +
					BOOL_TO_STR(result));
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
				this->compiler.info(std::string("Emitting literal when simplifying ") +
					std::to_string(left) + " == " +
					std::to_string(right) + " on line " +
					std::to_string(expr.op.line) + ": " +
					std::to_string(left == right));
				return emit_boolean_literal(expr.op, left == right);
			case TokenType::BANG_EQUAL:
				this->compiler.info(std::string("Emitting literal when simplifying ") +
					std::to_string(left) + " != " +
					std::to_string(right) + " on line " +
					std::to_string(expr.op.line) + ": " +
					std::to_string(left != right));
				return emit_boolean_literal(expr.op, left != right);
			case TokenType::GREATER:
				this->compiler.info(std::string("Emitting literal when simplifying ") +
					std::to_string(left) + " > " +
					std::to_string(right) + " on line " +
					std::to_string(expr.op.line) + ": " +
					std::to_string(left == right));
				return emit_boolean_literal(expr.op, left > right);
			case TokenType::GREATER_EQUAL:
				this->compiler.info(std::string("Emitting literal when simplifying ") +
					std::to_string(left) + " >= " +
					std::to_string(right) + " on line " +
					std::to_string(expr.op.line) + ": " +
					std::to_string(left >= right));
				return emit_boolean_literal(expr.op, left >= right);
			case TokenType::LESS:
				this->compiler.info(std::string("Emitting literal when simplifying ") +
					std::to_string(left) + " < " +
					std::to_string(right) + " on line " +
					std::to_string(expr.op.line) + ": " +
					std::to_string(left < right));
				return emit_boolean_literal(expr.op, left < right);
			case TokenType::LESS_EQUAL:
				this->compiler.info(std::string("Emitting literal when simplifying ") +
					std::to_string(left) + " <= " +
					std::to_string(right) + " on line " +
					std::to_string(expr.op.line) + ": " +
					std::to_string(left <= right));
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
	FOLD_INTO(expr.right, expr.right->accept(*this));
	if (expr.right->is<Expr::Literal>())
	{
		Expr::Literal& right = expr.right->as<Expr::Literal>();
		Literal result;
		switch (expr.op.type)
		{
		case TokenType::BANG:
			result = !right.literal.literal.as_boolean();
			return new Expr::Literal(Token(expr.op.line, result.type(), result.to_lexeme(), result.as_boolean()));
		case TokenType::MINUS:
			result = -right.literal.literal.as_number();
			return new Expr::Literal(Token(expr.op.line, result.type(), result.to_lexeme(), result.as_number()));
		default:
			throw std::runtime_error("Invalid operation for unary expression.");
		}
	}
	return nullptr;
}

void* Optimizer::visitExprVariable(Expr::Variable& expr)
{
	const Variable* var = this->local_env->get_variable(Identifier(expr.name.lexeme));
	if (!var)
	{
		throw std::runtime_error(std::string("Could not get variable ") + expr.name.lexeme);
	}
	if (var->type().compile_time)
	{
		const Literal& literal = var->full_type().fixed_value;
		return new Expr::Literal(Token(expr.name.line, literal.type(), literal.to_lexeme(), literal));
	}
	return nullptr;
}

void* Optimizer::visitExprDeviceLoad(Expr::DeviceLoad& expr)
{
	FOLD_INTO(expr.device, expr.device->accept(*this));
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
		if (literal_left.literal.literal.is_boolean() && literal_right.literal.literal.is_boolean())
		{
			if (expr.op.type == TokenType::AND)
			{
				bool result = (literal_left.literal.literal.as_boolean()) && (literal_right.literal.literal.as_boolean());
				this->compiler.info(std::string("Emitting literal when simplifying ") +
					BOOL_TO_STR(literal_left.literal.literal.as_boolean()) + " and " +
					BOOL_TO_STR(literal_right.literal.literal.as_boolean()) + " on line " +
					std::to_string(expr.op.line) + ": " +
					BOOL_TO_STR(result)
				);
				return emit_boolean_literal(expr.op, result);
			}
			else if(expr.op.type == TokenType::OR)
			{
				bool result = (literal_left.literal.literal.as_boolean()) || (literal_right.literal.literal.as_boolean());
				this->compiler.info(std::string("Emitting literal when simplifying ") +
					BOOL_TO_STR(literal_left.literal.literal.as_boolean()) + " or " +
					BOOL_TO_STR(literal_right.literal.literal.as_boolean()) + " on line " +
					std::to_string(expr.op.line) + ": " +
					BOOL_TO_STR(result)
				);
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

void* Optimizer::visitStmtDeviceSet(Stmt::DeviceSet& expr)
{
	FOLD_INTO(expr.device, expr.device->accept(*this));
	FOLD_INTO(expr.value, expr.value->accept(*this));
	return nullptr;
}

void* Optimizer::visitStmtStatic(Stmt::Static& stmt)
{
	std::unique_ptr<Stmt> converted(stmt.var.get());
	Stmt* folded = this->visit_stmt(converted).release();
	converted.release();
	return folded;
}

void* Optimizer::visitStmtVariable(Stmt::Variable& stmt)
{
	FOLD_INTO(stmt.initalizer, stmt.initalizer->accept(*this));
	Variable* var = this->local_env->get_mut_variable(stmt.name.lexeme);
	if (!var)
	{
		throw std::runtime_error(std::string("Could not get variable ") + stmt.name.lexeme);
	}
	if (var->type().compile_time)
	{
		if (!stmt.initalizer->is<Expr::Literal>())
		{
			throw std::runtime_error(std::string("Optimizer could not fold compile time constant value ") + stmt.name.lexeme);
		}
		var->full_type().fixed_value = stmt.initalizer->as<Expr::Literal>().literal.literal;
		this->compiler.info(std::string("Folded fixed value ") + var->identifier().name() + " into " + var->full_type().fixed_value.to_lexeme());
		return new Stmt::NoOp();
	}
	return nullptr;
}

void* Optimizer::visitStmtBlock(Stmt::Block& stmt)
{
	this->local_env = this->local_env->enter(&stmt);
	this->evaluate(stmt.statements);
	this->local_env = this->local_env->get_parent();
	return nullptr;
}

void* Optimizer::visitStmtWhile(Stmt::While& expr)
{
	FOLD_INTO(expr.condition, expr.condition->accept(*this));
	this->visit_stmt(expr.body);
	return nullptr;
}

void* Optimizer::visitStmtIf(Stmt::If& stmt)
{
	Expr::Literal* condition_literal = static_cast<Expr::Literal*>(stmt.condition->accept(*this));
	if (condition_literal)
	{
		Literal& literal = condition_literal->literal.literal;
		if (literal.is_boolean())
		{
			if (literal.as_boolean())
			{
				this->compiler.info(std::string("Branching on line ") +
					std::to_string(condition_literal->literal.line) +
					" simplified to true brach."
				);
				return stmt.branch_true.release();
			}
			if (stmt.branch_false)
			{
				this->compiler.info(std::string("Branching on line ") +
					std::to_string(condition_literal->literal.line) +
					" simplified to false branch."
				);
				return stmt.branch_false.release();
			}
			this->compiler.info(std::string("Branching on line ") +
				std::to_string(condition_literal->literal.line) +
				" simplified to removal of condition."
			);
			return new Stmt::Block({}, stmt.token);
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
	this->local_env = this->local_env->enter(&expr);
	if (!this->local_env)
	{
		throw std::runtime_error("Attempted to enter a statement which does not confer an environment.");
	}
	// todo: fold/inline functions
	this->evaluate(expr.body);
	this->local_env = this->local_env->get_parent();
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