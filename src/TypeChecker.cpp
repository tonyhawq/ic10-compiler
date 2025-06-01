#include "TypeChecker.h"

#include "Compiler.h"

#define CHECK_IS_FUNCTION_OR_ERROR(reason) do {if (!this->is_function) {throw std::runtime_error(std::string("TYPECHECK ERROR: !ATTEMPTED TO GET ") + std::string(reason) + " OF NON FUNCTION TYPE " + this->type.type_name());}} while(0)

TypeName& TypeID::return_type()
{
	CHECK_IS_FUNCTION_OR_ERROR("RETURN TYPE");
	return *this->m_return_type;
}

std::string& TypeID::unmangled_name()
{
	CHECK_IS_FUNCTION_OR_ERROR("UNMANGLED_NAME");
	return *this->m_unmangled_name;
}

std::vector<TypeID::FunctionParam>& TypeID::arguments()
{
	CHECK_IS_FUNCTION_OR_ERROR("ARGUMENTS");
	return *this->m_arguments;
}

const TypeName& TypeID::return_type() const
{
	CHECK_IS_FUNCTION_OR_ERROR("RETURN TYPE");
	return *this->m_return_type;
}

const std::string& TypeID::unmangled_name() const
{
	CHECK_IS_FUNCTION_OR_ERROR("UNMANGLED_NAME");
	return *this->m_unmangled_name;
}

const std::vector<TypeID::FunctionParam>& TypeID::arguments() const
{
	CHECK_IS_FUNCTION_OR_ERROR("ARGUMENTS");
	return *this->m_arguments;
}

bool TypeChecker::Operator::Overload::Type::matches(const TypeName& other) const
{
	if (this->matches_underlying)
	{
		return this->type.underlying() == other.underlying();
	}
	return this->type == other;
}

#define OPERATOR_OVERLOADING_RETURN_TYPE_GET_OR_MAKE_FAILURE_MODE(val) do { if (return_type.failed()) {\
	return_type = ReturnType(val);\
} } while (0)

TypeName TypeChecker::Operator::Overload::return_type(const std::vector<TypeName>& args) const
{
	if (this->transformation)
	{
		if (args.size() != 1)
		{
			throw std::runtime_error("TYPECHECKER ERROR: !OPERATOR WHICH TRANSFORMS INPUT ARG CANNOT TRANSFORM > 1 or 0 ARGS!");
		}
		TypeName transformed = this->unannotated_return_type;
		int height = this->transformation->height();
		// 5 layer pointer
		for (int i = 0; i < height; i++)
		{
			TypeName* transformation_to_use = this->transformation.get();
			// to get 1st layer (4 from top)
			// iterate 4 times
			for (int j = 0; j < height - i; j++)
			{
				transformation_to_use = transformation_to_use->m_pointed_to_type.get();
			}
			bool constant = transformation_to_use->constant;
			if (transformation_to_use->pointer)
			{
				transformed.make_pointer_type(constant);
			}
			else
			{
				transformed.constant = constant;
			}
		}
		return transformed;
	}
	return this->unannotated_return_type;
}

TypeChecker::Operator::ReturnType TypeChecker::Operator::return_type(const std::vector<TypeName>& args)
{
	ReturnType return_type = ReturnType();
	for (const auto& overload : this->overloads)
	{
		if (overload.args.size() != args.size())
		{
			OPERATOR_OVERLOADING_RETURN_TYPE_GET_OR_MAKE_FAILURE_MODE(std::string("Operator overloading failed for operator ") + this->operator_name);
			std::string& failure = return_type.failure();
			failure += "\n    -";
			failure += " Requires ";
			failure += std::to_string(overload.args.size());
			failure += " arguments, got ";
			failure += std::to_string(args.size());
			continue;
		}
		bool arg_resolution_failed = false;
		for (int i = 0; i < overload.args.size(); i++)
		{
			const Overload::Type& overload_arg = overload.args[i];
			const TypeName& arg = args[i];
			if (!overload_arg.matches(arg))
			{
				arg_resolution_failed = true;
				OPERATOR_OVERLOADING_RETURN_TYPE_GET_OR_MAKE_FAILURE_MODE(std::string("Operator overloading failed for operator ") + this->operator_name);
				std::string& failure = return_type.failure();
				failure += "\n    -";
				failure += " Argument ";
				failure += std::to_string(i);
				failure += " is of type ";
				failure += arg.type_name();
				failure += " while wanted type is ";
				failure += overload_arg.type.type_name();
				continue;
			}
		}
		if (arg_resolution_failed)
		{
			continue;
		}
		return ReturnType(overload.return_type(args));
	}
	if (!return_type.failed())
	{
		throw std::runtime_error("TYPECHECKER ERROR: !NO OVERLOADS PROVIDED FOR OPERATOR!");
	}
	return return_type;
}

bool TypeChecker::Operator::ReturnType::failed() const
{
	if (this->failure_mode)
	{
		return true;
	}
	return false;
}

std::string& TypeChecker::Operator::ReturnType::failure()
{
	return *this->failure_mode;
}

const std::string& TypeChecker::Operator::ReturnType::failure() const
{
	return *this->failure_mode;
}

const TypeName& TypeChecker::Operator::ReturnType::type() const
{
	return *this->m_type;
}

TypeChecker::TypeChecker(Compiler& compiler, std::vector<std::unique_ptr<Stmt>>& statements)
	:compiler(compiler), statements_to_typecheck(statements), env(new Environment(Environment::make_orphaned())), current_pass(Pass::Linking)
{
	TypeName t_number("number");
	TypeName t_string("string");
	TypeName t_Device("Device");
	TypeName t_boolean("boolean");
	TypeName t_void("void");
	this->types.emplace(t_number, TypeID{ t_number, false, nullptr, nullptr });
	this->types.emplace(t_string, TypeID{ t_string, false, nullptr, nullptr });
	this->types.emplace(t_Device, TypeID{ t_Device, false, nullptr, nullptr });
	this->types.emplace(t_boolean, TypeID{ t_boolean, false, nullptr, nullptr });
	this->types.emplace(t_void, TypeID{ t_void, false, nullptr, nullptr });
	
	this->define_operator(TokenType::GREATER, "op_greater", { { t_boolean, { t_number, t_number } } });
	this->define_operator(TokenType::GREATER_EQUAL, "op_greater_equal", { { t_boolean, { t_number, t_number } } });
	this->define_operator(TokenType::LESS, "op_less", {{ t_boolean, { t_number, t_number } } });
	this->define_operator(TokenType::LESS_EQUAL, "op_less_equal", { {t_boolean, { t_number, t_number } } });
	this->define_operator(TokenType::PLUS, "op_plus", { {t_number, { t_number, t_number } } });
	this->define_operator(TokenType::MINUS, "op_minus", { { t_number, { t_number, t_number } }, {t_number, {t_number}} }); // includes unary -
	this->define_operator(TokenType::STAR, "op_star", { { t_number, { t_number, t_number } } });
	this->define_operator(TokenType::SLASH, "op_div", { { t_number, { t_number, t_number } } });
	this->define_operator(TokenType::EQUAL_EQUAL, "op_equality", { { t_boolean, { t_number, t_number }}, { t_boolean, { t_boolean, t_boolean } } });
	this->define_operator(TokenType::BANG_EQUAL, "op_inverse_equality", { { t_boolean, { t_number, t_number } }, { t_boolean, { t_boolean, t_boolean } } });
	this->define_operator(TokenType::BANG, "unary_not", { {t_boolean, {t_boolean} } });

	this->define_library_function("dopen", t_Device, { t_number });
	this->define_library_function("yield", t_void, {});
}

void TypeChecker::define_operator(TokenType type, const std::string& name, const std::vector<>& types)
{
	if (this->operator_types.count(type))
	{
		throw std::runtime_error("TYPECHECK ERROR: !ATTEMPTED TO CREATE EXISTENT OPERATOR!");
	}
	for (const auto& operator_type : types)
	{
		std::vector<TypeID::FunctionParam> params;
		params.reserve(operator_type.arg_types.size());
		for (const auto& arg_type : operator_type.arg_types)
		{
			params.push_back({ arg_type, "_" });
		}
		this->operator_types[type].push_back(TypeID{
			this->get_function_signature(params, operator_type.return_type),
			true,
			std::make_unique<TypeName>(operator_type.return_type),
			std::make_unique<std::string>(name),
			std::make_unique<std::vector<TypeID::FunctionParam>>(std::move(params))
		});
	}
}

void TypeChecker::define_library_function(const std::string& name, const TypeName& return_type, const std::vector<TypeName>& arg_types)
{
	std::vector<TypeID::FunctionParam> params;
	params.reserve(arg_types.size());
	for (const auto& arg_type : arg_types)
	{
		params.push_back({ arg_type, "_" });
	}
	TypeName type = TypeChecker::get_function_signature(params, return_type);
	this->env->define(type, name);
	this->types.emplace(type, TypeID{
		type,
		true,
		std::make_unique<TypeName>(return_type),
		std::make_unique<std::string>(name),
		std::make_unique<std::vector<TypeID::FunctionParam>>(std::move(params))
	});
}

void TypeChecker::check()
{
	this->current_pass = Pass::Linking;
	this->evaluate(this->statements_to_typecheck);
	this->current_pass = Pass::TypeChecking;
	this->evaluate(this->statements_to_typecheck);
}

std::unique_ptr<TypeName> TypeChecker::accept(Expr& expression)
{
	return std::unique_ptr<TypeName>(static_cast<TypeName*>(expression.accept(*this)));
}

void* TypeChecker::visitExprBinary(Expr::Binary& expr)
{
	std::unique_ptr<TypeName> left_type = this->accept(*expr.left);
	std::unique_ptr<TypeName> right_type = this->accept(*expr.right);
	if (!left_type || !right_type)
	{
		return new TypeName(this->operator_types.at(expr.op.type).begin()->return_type());
	}
	if (left_type != right_type)
	{
		this->error(expr.op, "Attempted to perform an operation on two dissimilar types.");
		return nullptr;
	}
	std::vector<std::string> overload_failures;
	for (const auto& op : this->operator_types.at(expr.op.type))
	{
		if (op.arguments().size() != 2)
		{
			overload_failures.push_back(std::string("Operator ") + op.unmangled_name() + " could not match (requires " + std::to_string(op.arguments().size()) + " arguments)");
			continue;
		}
		if (*left_type != op.arguments().at(0).type)
		{
			overload_failures.push_back(std::string("Operator ") + op.unmangled_name() + " could not match left type " + left_type->type_name());
			continue;
		}
		if (*right_type != op.arguments().at(1).type)
		{
			overload_failures.push_back(std::string("Operator ") + op.unmangled_name() + " could not match right type " + right_type->type_name());
			continue;
		}
		return new TypeName(op.return_type());
	}
	std::string error_msg = "Could find matching overload for operator:\n";
	for (const auto& failure : overload_failures)
	{
		error_msg += "  - ";
		error_msg += failure;
		error_msg += '\n';
	}
	this->error(expr.op, error_msg);
	return nullptr;
}

void* TypeChecker::visitExprGrouping(Expr::Grouping& expr)
{
	if (!expr.expression)
	{
		return nullptr;
	}
	return this->accept(*expr.expression).release();
}

void* TypeChecker::visitExprLiteral(Expr::Literal& expr)
{
	if (expr.literal.literal.string)
	{
		return new TypeName(true, "string");
	}
	if (expr.literal.literal.number)
	{
		return new TypeName(true, "number");
	}
	if (expr.literal.literal.boolean)
	{
		return new TypeName(true, "boolean");
	}
	this->error(expr.literal, "Literal value did not have a type.");
	return nullptr;
}

void* TypeChecker::visitExprUnary(Expr::Unary& expr)
{
	std::unique_ptr<TypeName> type = this->accept(*expr.right);
	if (!type)
	{
		return nullptr;
	}
	Operator& op = this->operator_types.at(expr.op.type);
	std::vector<std::string> overload_failures;
	for (const auto& op : operators)
	{
		if (op.arguments().size() != 1)
		{
			overload_failures.push_back(std::string("Operator ") + op.unmangled_name() + " could not match (requires " + std::to_string(op.arguments().size()) + " arguments)");
			continue;
		}
		if (op.arguments().at(0).type != *type)
		{
			overload_failures.push_back(std::string("Operator ") + op.unmangled_name() + " could not match type " + type->type_name()); 
			continue;
		}
		return new TypeName(op.return_type());
	}
	std::string error_msg = "Could find matching overload for operator:\n";
	for (const auto& failure : overload_failures)
	{
		error_msg += "  - ";
		error_msg += failure;
		error_msg += '\n';
	}
	this->error(expr.op, error_msg);
	return nullptr;
}

void* TypeChecker::visitExprVariable(Expr::Variable& expr)
{
	Environment::VariableInfo* info = this->env->resolve(expr.name.lexeme);
	if (!info)
	{
		this->error(expr.name, std::string("Attempted to use variable \"") + expr.name.lexeme + "\" before it was defined.");
		return nullptr;
	}
	if (this->types.count(info->type.underlying()))
	{
		return new TypeName(info->type);
	}
	this->error(expr.name, std::string("No such type exists with name ") + info->type.type_name() + " for using variable " + expr.name.lexeme);
	return nullptr;
}

bool TypeChecker::can_initalize(const TypeName& to, const TypeName& from)
{
	if (to == from)
	{
		return true;
	}
	if (!to.const_unqualified_equals(from))
	{
		// complete inequality
		// eg: number, string
		return false;
	}
	// possible equality
	// eg: number const, number
	if (to.pointer)
	{
		if (to.constant)
		{
			return true;
		}
		// cannot initalize a number* from a number const*
		return false;
	}
	// can initalize a number from a number const
	return true;
}

bool TypeChecker::can_assign(const TypeName& to, const TypeName& from)
{
	if (to == from)
	{
		return true;
	}
	if (!to.const_unqualified_equals(from))
	{
		return false;
	}
	if (to.pointer && from.pointer)
	{
		return TypeChecker::can_initalize(to, from);
	}
	if (to.constant)
	{
		return false;
	}
	return true;
}

void* TypeChecker::visitExprAssignment(Expr::Assignment& expr)
{
	Environment::VariableInfo* info = this->env->resolve(expr.name.lexeme);
	if (!info)
	{
		this->error(expr.name, std::string("Attempted to assign variable \"") + expr.name.lexeme + "\" before it was defined.");
		return nullptr;
	}
	std::unique_ptr<TypeName> var_type;
	if (this->types.count(info->type.underlying()))
	{
		var_type = std::make_unique<TypeName>(info->type);
	}
	if (!var_type)
	{
		this->error(expr.name, std::string("No such type exists with name ") + info->type.type_name() + " for assigning to " + expr.name.lexeme);
		return nullptr;
	}
	std::unique_ptr<TypeName> value_type = this->accept(*expr.value);
	if (!value_type)
	{
		return nullptr;
	}
	if (!this->can_assign(*var_type, *value_type))
	{
		this->error(expr.name, std::string("Type mismatch for assignment of ") + expr.name.lexeme + ", has type " + var_type->type_name() +
			" and value is of type " + value_type->type_name());
		return nullptr;
	}
	return value_type.release();
}

void* TypeChecker::visitExprCall(Expr::Call& expr)
{
	std::unique_ptr<TypeName> resolved_type = this->accept(*expr.callee);
	if (!resolved_type)
	{
		return nullptr;
	}
	if (!this->types.count(*resolved_type))
	{
		this->error(expr.paren, std::string("No type exists for function call: type name was ") + resolved_type->type_name());
		return nullptr;
	}
	TypeID& function_type = this->types.at(*resolved_type);
	if (!function_type.is_function)
	{
		this->error(expr.paren, std::string("Attempted to call a non-function type."));
		return nullptr;
	}
	bool had_error = false;
	for (int i = 0; i < std::max(expr.arguments.size(), function_type.arguments().size()); i++)
	{
		if (i >= function_type.arguments().size())
		{
			this->error(expr.paren, std::string("Argument ") + std::to_string(i) + " out of range. " + function_type.unmangled_name() +
				" expects at most " + std::to_string(function_type.arguments().size()) + " arguments.");
			continue;
		}
		if (i >= expr.arguments.size())
		{
			this->error(expr.paren, std::string("Not enough arguments in function call. ") + function_type.unmangled_name() +
				" expects " + std::to_string(function_type.arguments().size()) + " got " + std::to_string(expr.arguments.size()));
			continue;
		}
		TypeName& wanted_arg_type = (function_type.arguments())[i].type;
		std::unique_ptr<TypeName> arg_type = this->accept(*expr.arguments[i]);
		if (!arg_type)
		{
			continue;
		}
		if (!this->can_initalize(wanted_arg_type, *arg_type))
		{
			had_error = true;
			this->error(expr.paren, std::string("Argument ") + std::to_string(i) + " type mismatch, wants " + wanted_arg_type.type_name() + " but got " +
				arg_type->type_name());
		}
	}
	if (had_error)
	{
		return nullptr;
	}
	if (!this->types.count(function_type.return_type().underlying()))
	{
		this->error(expr.paren, std::string("Attempted to call an invalid function type. (Invalid return type \"") +
			function_type.return_type().type_name() + "\")");
		return nullptr;
	}
	return new TypeName(function_type.return_type());
}

void* TypeChecker::visitExprLogical(Expr::Logical& expr)
{
	std::unique_ptr<TypeName> left_type = this->accept(*expr.left);
	std::unique_ptr<TypeName> right_type = this->accept(*expr.right);
	if (!left_type || !right_type)
	{
		return nullptr;
	}
	if (*left_type != TypeName("boolean") || *right_type != TypeName("boolean"))
	{
		this->error(expr.op, std::string("Attempted to logically compare ") + left_type->type_name() + " with " +
			right_type->type_name() + " (requires boolean operations)");
		return nullptr;
	}
	return left_type.release();
}

void* TypeChecker::visitStmtExpression(Stmt::Expression& stmt)
{
	if (this->current_pass == Pass::Linking)
	{
		return nullptr;
	}
	std::unique_ptr<TypeName> expression_type = this->accept(*stmt.expression);
	return nullptr;
}

void* TypeChecker::visitStmtAsm(Stmt::Asm& stmt)
{
	if (this->current_pass == Pass::Linking)
	{
		return nullptr;
	}
	return nullptr;
}

void* TypeChecker::visitStmtPrint(Stmt::Print& stmt)
{
	if (this->current_pass == Pass::Linking)
	{
		return nullptr;
	}
	std::unique_ptr<TypeName> to_print_type = this->accept(*stmt.expression);
	return nullptr;
}

void* TypeChecker::visitStmtVariable(Stmt::Variable& stmt)
{
	if (this->current_pass == Pass::Linking)
	{
		return nullptr;
	}
	if (!this->types.count(stmt.type.underlying()))
	{
		this->error(stmt.name, std::string("Unknown type ") + stmt.type.type_name());
		return nullptr;
	}
	TypeID& underlying = this->types.at(stmt.type.underlying());
	if (underlying.type == TypeName("void"))
	{
		this->error(stmt.name, std::string("Attempted to declare variable ") + stmt.name.lexeme + " as void.");
		return nullptr;
	}
	bool success = this->env->define(stmt.type, stmt.name.lexeme);
	if (!stmt.initalizer)
	{
		this->error(stmt.name, std::string("Variable ") + stmt.name.lexeme + " is uninitalized.");
		return nullptr;
	}
	std::unique_ptr<TypeName> initalizer_type(static_cast<TypeName*>(stmt.initalizer->accept(*this)));
	if (!initalizer_type)
	{
		return nullptr;
	}
	if (!success)
	{
		this->error(stmt.name, std::string("Redefining variable ") + stmt.name.lexeme);
	}
	if (!this->can_initalize(stmt.type, *initalizer_type))
	{
		this->error(stmt.name, std::string("Attempted to set ") + stmt.name.lexeme + " (of type " + stmt.type.type_name() +
			") to a value of type " + initalizer_type->type_name());
		return nullptr;
	}
	return nullptr;
}

void* TypeChecker::visitStmtBlock(Stmt::Block& stmt)
{
	if (this->current_pass == Pass::Linking)
	{
		return nullptr;
	}
	this->env = &this->env->spawn();
	this->evaluate(stmt.statements);
	this->env = this->env->pop();
	return nullptr;
}

void* TypeChecker::visitStmtIf(Stmt::If& stmt)
{
	if (this->current_pass == Pass::Linking)
	{
		return nullptr;
	}
	std::unique_ptr<TypeName> condition_type = this->accept(*stmt.condition);
	if (!condition_type)
	{
		stmt.branch_true->accept(*this);
		if (stmt.branch_false)
		{
			stmt.branch_false->accept(*this);
		}
		return nullptr;
	}
	bool had_error = false;
	if (*condition_type != TypeName("boolean"))
	{
		this->error(stmt.token, "Attempted to branch on a non-boolean condition.");
		had_error = true;
	}
	stmt.branch_true->accept(*this);
	if (stmt.branch_false)
	{
		stmt.branch_false->accept(*this);
	}
	if (had_error)
	{
		return nullptr;
	}
	return nullptr;
}


void* TypeChecker::visitStmtReturn(Stmt::Return& expr)
{
	if (!this->env->inside_function())
	{
		this->error(expr.keyword, "Attempted to return from outside of a function.");
		return nullptr;
	}
	const std::string& function_name = *this->env->function_name();
	Environment::VariableInfo* info = this->env->resolve(function_name);
	if (!info)
	{
		this->error(expr.keyword, "TYPECHECKER ERROR: !INSIDE NON-EXISTENT FUNCTION!");
		return nullptr;
	}
	TypeID& function_type = this->types.at(info->type);
	if (function_type.return_type().underlying() == TypeName("void"))
	{
		if (expr.value)
		{
			this->error(expr.keyword, "Cannot return anything from a function which returns void.");
			return nullptr;
		}
		return nullptr;
	}
	if (!expr.value)
	{
		this->error(expr.keyword, "Non-void function must return a value.");
		return nullptr;
	}
	std::unique_ptr<TypeName> return_type = this->accept(*expr.value);
	if (!return_type)
	{
		return nullptr;
	}
	if (!this->can_initalize(function_type.return_type(), *return_type))
	{
		this->error(expr.keyword, std::string("Mismatched return types, wants ") + function_type.return_type().type_name() +
			" while value is of type " + return_type->type_name());
		return nullptr;
	}
	return nullptr;
}

void* TypeChecker::visitStmtFunction(Stmt::Function& expr)
{
	if (this->current_pass == Pass::Linking)
	{
		std::vector<TypeID::FunctionParam> args;
		for (const auto& param : expr.params)
		{
			const TypeName& param_type = param.type;
			const Token& param_name = param.name;
			args.push_back({ param_type, param_name.lexeme });
		}
		TypeName function_type = this->get_function_signature(args, expr.return_type);
		bool success = this->env->define(function_type, expr.name.lexeme);
		this->types.emplace(function_type, TypeID{
			function_type,
			true,
			std::make_unique<TypeName>(expr.return_type),
			std::make_unique<std::string>(expr.name.lexeme),
			std::make_unique<std::vector<TypeID::FunctionParam>>(args)
			});
		if (!success)
		{
			this->error(expr.name, std::string("Redefining function ") + expr.name.lexeme);
		}
		return nullptr;
	}

	// typechecking

	bool had_error = false;
	this->env = &this->env->spawn_inside_function(expr.name.lexeme);
	for (const auto& param : expr.params)
	{
		const TypeName& param_type = param.type;
		const Token& param_name = param.name;
		if (!this->types.count(param_type.underlying()))
		{
			this->error(param_name, std::string("No type exists with name ") + param_type.type_name());
			had_error = true;
		}
		else
		{
			this->env->define(param_type, param_name.lexeme);
		}
	}
	if (!this->types.count(expr.return_type.underlying()))
	{
		this->error(expr.name, std::string("No type exists with name ") + expr.return_type.type_name());
		had_error = true;
	}
	this->evaluate(expr.body);
	this->env = this->env->pop();
	return nullptr;
}

std::string TypeChecker::get_mangled_function_name(const std::string& name, const std::vector<TypeID::FunctionParam> params, const TypeName& return_type)
{
	std::string type_name = std::string("@function_") + name + "_return_" + return_type.type_name() + "_params_";
	for (int i = 0; i < params.size(); i++)
	{
		const TypeID::FunctionParam& param = params[i];
		type_name += '@';
		type_name += param.type.type_name();
		type_name += '_';
		type_name += param.name;
	}
	return type_name;
}

TypeName TypeChecker::get_function_signature(const std::vector<TypeID::FunctionParam> params, const TypeName& return_type)
{
	std::string signature = "@function__";
	if (params.size())
	{
		signature += "@params__";
	}
	for (const auto& param : params)
	{
		signature += param.type.type_name();
		signature += "_";
	}
	signature += "@return";
	signature += return_type.type_name();
	return TypeName(std::move(signature));
}

void TypeChecker::evaluate(std::vector<std::unique_ptr<Stmt>>& statements)
{
	for (auto& statement : statements)
	{
		statement->accept(*this);
	}
}

void TypeChecker::error(const Token& token, const std::string& message)
{
	this->compiler.error(token.line, message);
}
