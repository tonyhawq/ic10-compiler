#include "TypeChecker.h"

#include "Compiler.h"

TypeName TypeChecker::t_number = TypeName("number");
TypeName TypeChecker::t_boolean = TypeName("boolean");
TypeName TypeChecker::t_string = TypeName("string");
TypeName TypeChecker::t_void = TypeName("void");

Identifier::Identifier(const std::string& name)
	:m_name(name)
{}

bool Identifier::operator==(const Identifier& other) const
{
	return this->m_name == other.m_name;
}

std::string Identifier::name() const
{
	return this->m_name;
}

Variable::Variable(const Identifier& identifier, const TypeID& type, const SymbolUseNode& node)
	:m_identifier(identifier), m_type(type), m_definition(node)
{}

SymbolUseNode Variable::def() const
{
	return this->m_definition;
}

const TypeName& Variable::type() const
{
	return this->m_type.type;
}

TypeName& Variable::type()
{
	return this->m_type.type;
}

TypeID& Variable::full_type()
{
	return this->m_type;
}

const TypeID& Variable::full_type() const
{
	return this->m_type;
}

const Identifier& Variable::identifier() const
{
	return this->m_identifier;
}

TypedEnvironment::TypedEnvironment()
	:next_id(0)
{
	this->m_root = std::make_unique<TypedEnvironment::Leaf>(*this);
	this->leaves[this->next_id] = this->m_root.get();
}

TypedEnvironment::~TypedEnvironment()
{}

size_t TypedEnvironment::advance(Leaf* advance_for)
{
	size_t id = this->next_id;
	this->next_id++;
	this->leaves[id] = advance_for;
	return id;
}

void TypedEnvironment::reassign(size_t id, Leaf* new_node)
{
	if (!this->leaves.count(id))
	{
		throw std::logic_error("No leaf exists.");
	}
	this->leaves[id] = new_node;
}

TypedEnvironment::Leaf* TypedEnvironment::root()
{
	return this->m_root.get();
}

TypedEnvironment::Leaf::Leaf(TypedEnvironment& progenitor) :parent(nullptr), owning_statement(nullptr), m_progenitor(&progenitor), id(0)
{
	this->id = this->m_progenitor->advance(this);
}

TypedEnvironment::Leaf::Leaf(Leaf* parent, Stmt* statement) :parent(parent), owning_statement(statement), m_progenitor(parent->m_progenitor)
{
	this->id = this->m_progenitor->advance(this);
}

TypedEnvironment::Leaf* TypedEnvironment::Leaf::enter(Stmt* statement)
{
	for (auto& child : this->m_children)
	{
		if (child.owning_statement == statement)
		{
			return &child;
		}
	}
	return nullptr;
}

const Stmt* TypedEnvironment::Leaf::get_owning_statement()
{
	return this->owning_statement;
}

void TypedEnvironment::Leaf::set_owning_statement(Stmt* stmt)
{
	this->owning_statement = stmt;
}

TypedEnvironment::Leaf::~Leaf()
{}

TypedEnvironment::Leaf::Leaf(Leaf&& other) noexcept
	:m_function_name(std::move(other.m_function_name)), parent(other.parent), m_children(std::move(other.m_children)), m_variables(std::move(other.m_variables)),
	m_progenitor(other.m_progenitor), id(other.id), owning_statement(other.owning_statement)
{
	this->m_progenitor->reassign(this->id, this);
}

TypedEnvironment::Leaf* TypedEnvironment::Leaf::spawn_inside_function(Stmt* statement, const std::string& name)
{
	Leaf* env = this->spawn(statement);
	env->m_function_name = std::make_unique<std::string>(name);
	return env;
}

TypedEnvironment::Leaf* TypedEnvironment::Leaf::spawn(Stmt* statement)
{
	this->m_children.push_back(Leaf(this, statement));
	return &this->m_children.back();
}

TypedEnvironment::Leaf* TypedEnvironment::Leaf::get_parent()
{
	return this->parent;
}

const std::string* TypedEnvironment::Leaf::function() const
{
	if (!this->m_function_name)
	{
		if (!this->parent)
		{
			return nullptr;
		}
		return this->parent->function();
	}
	return this->m_function_name.get();
}

bool TypedEnvironment::Leaf::define_variable(const TypeID& type, const Identifier& identifier, const SymbolUseNode& def)
{
	const auto& var = this->m_variables.find(identifier);
	if (var != this->m_variables.end())
	{
		return false;
	}
	this->m_variables.emplace(identifier, Variable(identifier, type, def));
	return true;
}

size_t TypedEnvironment::Leaf::leaf_id() const
{
	return this->id;
}

const std::list<TypedEnvironment::Leaf>& TypedEnvironment::Leaf::children() const
{
	return this->m_children;
}

const Variable* TypedEnvironment::Leaf::get_variable(const Identifier& identifier) const
{
	const auto& var = this->m_variables.find(identifier);
	if (var != this->m_variables.end())
	{
		return &var->second;
	}
	if (this->parent)
	{
		return this->parent->get_variable(identifier);
	}
	return nullptr;
}

Variable* TypedEnvironment::Leaf::get_mut_variable(const Identifier& identifier)
{
	const Variable* var = this->get_variable(identifier);
	return const_cast<Variable*>(var);
}

TypedEnvironment& TypedEnvironment::Leaf::progenitor()
{
	return *this->m_progenitor;
}

int TypedEnvironment::Leaf::scopes_to_definition(const Identifier& identifier) const
{
	if (this->m_variables.find(identifier) != this->m_variables.end())
	{
		return 0;
	}
	if (!this->parent)
	{
		return -1;
	}
	int parent_scopes = this->parent->scopes_to_definition(identifier);
	if (parent_scopes == -1)
	{
		return -1;
	}
	return parent_scopes + 1;
}

TypedEnvironment::Leaf::LongerLived TypedEnvironment::Leaf::longer_lived(const Variable& a, const Variable& b)
{
	int a_scopes = this->scopes_to_definition(a.identifier());
	int b_scopes = this->scopes_to_definition(b.identifier());
	if (a_scopes == -1 || b_scopes == -1)
	{
		return LongerLived::Undefined;
	}
	if (a_scopes == b_scopes)
	{
		return LongerLived::Same;
	}
	if (a_scopes > b_scopes)
	{
		return LongerLived::A;
	}
	return LongerLived::B;
}

#define OPERATOR_OVERLOADING_RETURN_TYPE_GET_OR_MAKE_FAILURE_MODE(val) do { if (!return_type.failed()) {\
	return_type = ReturnType(val);\
} } while (0)

TypeChecker::OperatorOverload::OperatorOverload(const TypeName& return_type, const std::vector<TypeName>& args) :m_return_type(return_type), arg_types(args)
{}

TypeChecker::OperatorOverload::~OperatorOverload()
{}

TypeChecker::Operator::ReturnType TypeChecker::OperatorOverload::return_type(const std::vector<TypeName>& args) const
{
	if (args.size() != this->arg_types.size())
	{
		return std::string("Requires ") + std::to_string(this->arg_types.size()) + " arguments, got " + std::to_string(args.size());
	}
	bool all_fixed = true;
	for (int i = 0; i < this->arg_types.size(); i++)
	{
		const TypeName& arg = args[i];
		const TypeName& param = this->arg_types[i];
		if (!arg.compile_time)
		{
			all_fixed = false;
		}
		if (!TypeChecker::can_assign(param, arg))
		{
			return std::string("Cannot assign argument ") + std::to_string(i + 1) + " of type " + arg.type_name() + " to " + param.type_name();
		}
	}
	if (all_fixed)
	{
		TypeName compile_time_result = this->m_return_type;
		compile_time_result.compile_time = true;
		return compile_time_result;
	}
	return this->m_return_type;
}

TypeChecker::Operator::ReturnType TypeChecker::Operator::return_type(const std::vector<TypeName>& args)
{
	ReturnType return_type = ReturnType();
	for (const auto& overload : this->overloads)
	{
		ReturnType overload_return_type = overload->return_type(args);
		if (overload_return_type.failed())
		{
			OPERATOR_OVERLOADING_RETURN_TYPE_GET_OR_MAKE_FAILURE_MODE(std::string("Operator overloading failed for operator ") + this->operator_name);
			return_type.failure() += overload_return_type.failure();
		}
		else
		{
			return overload_return_type;
		}
	}
	if (!return_type.failed())
	{
		throw std::logic_error("TYPECHECKER ERROR: !NO OVERLOADS PROVIDED FOR OPERATOR!");
	}
	return return_type;
}

TypeChecker::OperatorOverload::Addressof::Addressof()
	:OperatorOverload(TypeChecker::t_void, {})
{}

TypeChecker::OperatorOverload::Addressof::~Addressof()
{}

TypeChecker::Operator::ReturnType TypeChecker::OperatorOverload::Addressof::return_type(const std::vector<TypeName>& args) const
{
	if (args.size() != 1)
	{
		return Operator::ReturnType("Requires 1 argument.");
	}
	TypeName arg = args.at(0);
	if (arg.compile_time)
	{
		return Operator::ReturnType("Cannot take the address of a compile-time fixed variable.");
	}
	arg.make_pointer_type(false, false);
	return arg;
}

TypeChecker::OperatorOverload::Deref::Deref()
	:OperatorOverload(TypeChecker::t_void, {})
{}

TypeChecker::OperatorOverload::Deref::~Deref()
{}

TypeChecker::Operator::ReturnType TypeChecker::OperatorOverload::Deref::return_type(const std::vector<TypeName>& args) const
{
	if (args.size() != 1)
	{
		return Operator::ReturnType("Requires 1 argument.");
	}
	TypeName arg = args.at(0);
	if (!arg.pointer)
	{
		return Operator::ReturnType("Cannot dereference a non-pointer type.");
	}
	return *arg.m_pointed_to_type;
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

TypeCheckedProgram::TypeCheckedProgram(std::vector<std::unique_ptr<Stmt>> statements)
	:m_statements(std::move(statements))
{}

const std::vector<std::unique_ptr<Stmt>>& TypeCheckedProgram::statements() const
{
	return this->m_statements;
}

std::vector<std::unique_ptr<Stmt>>& TypeCheckedProgram::statements()
{
	return this->m_statements;
}

void TypeCheckedProgram::add_statement(std::unique_ptr<Stmt>& statement, TypedEnvironment::Leaf& containing_env)
{
	this->ptr_to_leaf[statement.get()] = &containing_env;
}

TypedEnvironment::Leaf& TypeCheckedProgram::statement_environment(Stmt* statement)
{
	if (!statement)
	{
		throw std::logic_error("Attempted to get env of nullptr statement.");
	}
	return *this->ptr_to_leaf.at(statement);
}

TypedEnvironment& TypeCheckedProgram::env()
{
	return this->m_env;
}

TypeChecker::TypeChecker(Compiler& compiler, std::vector<std::unique_ptr<Stmt>> statements)
	:compiler(compiler), env(nullptr), current_pass(Pass::Linking), program(std::move(statements))
{
	env = this->program.env().root();
	this->types.emplace(t_number, TypeID{ t_number });
	this->types.emplace(t_string, TypeID{ t_string });
	this->types.emplace(t_boolean, TypeID{ t_boolean });
	this->types.emplace(t_void, TypeID{ t_void });
	
	this->define_operator(TokenType::GREATER, "op_greater", {
		new OperatorOverload{t_boolean, {t_number, t_number}}
		});
	this->define_operator(TokenType::GREATER_EQUAL, "op_greater_equal", {
		new OperatorOverload{t_boolean, {t_number, t_number}}
		});
	this->define_operator(TokenType::LESS, "op_less", {
		new OperatorOverload{t_boolean, {t_number, t_number}}
		});
	this->define_operator(TokenType::LESS_EQUAL, "op_less_equal", {
		new OperatorOverload{t_boolean, {t_number, t_number}}
		});
	this->define_operator(TokenType::PLUS, "op_plus", {
		new OperatorOverload{t_number, {t_number, t_number}}
		});
	this->define_operator(TokenType::MINUS, "op_minus", {
		new OperatorOverload{t_number, {t_number}},
		new OperatorOverload{t_number, {t_number, t_number}}}
		); // includes unary -
	this->define_operator(TokenType::STAR, "op_star", {
		new OperatorOverload{t_number, {t_number, t_number}}
		});
	this->define_operator(TokenType::SLASH, "op_div", {
		new OperatorOverload{t_number, {t_number, t_number}}
		});
	this->define_operator(TokenType::EQUAL_EQUAL, "op_equality", {
		new OperatorOverload{t_boolean, {t_number, t_number}},
		new OperatorOverload{t_boolean, {t_boolean, t_boolean}}
		});
	this->define_operator(TokenType::BANG_EQUAL, "op_inverse_equality", {
		new OperatorOverload{t_boolean, {t_number, t_number}},
		new OperatorOverload{t_boolean, {t_boolean, t_boolean}}
		});
	this->define_operator(TokenType::BANG, "unary_not", {
		new OperatorOverload{t_number, {t_number}},
		});
	this->define_operator(TokenType::AMPERSAND, "op_addressof", {
		new OperatorOverload::Addressof()
		});
	this->define_operator(TokenType::BAR, "op_deref", {
		new OperatorOverload::Deref()
		});
}

void TypeChecker::define_operator(TokenType type, const std::string& name, std::vector<OwningPtr<OperatorOverload>> overloads)
{
	if (this->operator_types.count(type))
	{
		throw std::logic_error("TYPECHECK ERROR: !ATTEMPTED TO CREATE EXISTENT OPERATOR!");
	}
	std::vector<std::unique_ptr<OperatorOverload>> fixed_overloads;
	fixed_overloads.reserve(overloads.size());
	for (auto& overload : overloads)
	{
		fixed_overloads.emplace_back(overload.ptr);
	}
	this->operator_types.emplace(type, std::move(Operator(name, std::move(fixed_overloads))));
}

TypeCheckedProgram TypeChecker::check()
{
	this->current_pass = Pass::Linking;
	this->evaluate(this->program.statements());
	this->current_pass = Pass::TypeChecking;
	this->evaluate(this->program.statements());
	if (!this->seen_main)
	{
		this->error(Token(-1, TokenType::T_EOF, "\n"), "No entry point.");
	}
	return std::move(this->program);
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
		return nullptr;
	}
	Operator& op = this->operator_types.at(expr.op.type);
	Operator::ReturnType returned = op.return_type({ *left_type, *right_type });
	if (returned.failed())
	{
		this->error(expr.op, returned.failure());
		return nullptr;
	}
	expr.type = returned.type();
	return std::make_unique<TypeName>(returned.type()).release();
}

void* TypeChecker::visitExprGrouping(Expr::Grouping& expr)
{
	if (!expr.expression)
	{
		return nullptr;
	}
	std::unique_ptr<TypeName> type = this->accept(*expr.expression);
	expr.type = *type;
	return type.release();
}

void* TypeChecker::visitExprLiteral(Expr::Literal& expr)
{
	if (expr.literal.literal.string)
	{
		TypeName* type = new TypeName(true, "string");
		type->compile_time = true;
		expr.type = *type;
		return type;
	}
	if (expr.literal.literal.number)
	{
		TypeName* type = new TypeName(true, "number");
		type->compile_time = true;
		expr.type = *type;
		return type;
	}
	if (expr.literal.literal.boolean)
	{
		TypeName* type = new TypeName(true, "boolean");
		type->compile_time = true;
		expr.type = *type;
		return type;
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
	Operator::ReturnType returned = op.return_type({ *type });
	if (returned.failed())
	{
		this->error(expr.op, returned.failure());
		return nullptr;
	}
	const TypeName& return_type = returned.type();
	expr.type = return_type;
	return std::make_unique<TypeName>(return_type).release();
}


void TypeChecker::symbol_visit_expr_variable(Expr::Variable& expr, const Variable* info)
{
	SymbolTable::Index i = this->program.table.lookup_index(info->def().id());
	this->program.table.alias_symbol(i, SymbolUseNode(expr.downcast(), UseLocation::During));
}

void TypeChecker::symbol_visit_stmt_variable(Stmt::Variable& stmt, const Variable* info)
{
	this->program.table.create_symbol(SymbolUseNode(stmt.downcast()));
}

void* TypeChecker::visitExprVariable(Expr::Variable& expr)
{
	const Variable* info = this->env->get_variable(expr.name.lexeme);
	if (!info)
	{
		this->error(expr.name, std::string("Attempted to use variable \"") + expr.name.lexeme + "\" before it was defined.");
		return nullptr;
	}
	this->symbol_visit_expr_variable(expr, info);
	if (this->types.count(info->type().underlying()))
	{
		TypeName* type = new TypeName(info->type());
		expr.type = *type;
		return type;
	}
	this->error(expr.name, std::string("No such type exists with name ") + info->type().type_name() + " for using variable " + expr.name.lexeme);
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
	// eg: const number, number
	if (to.pointer)
	{
		if (to.constant)
		{
			return true;
		}
		// cannot initalize a number* from a const number*
		return false;
	}
	if (to.compile_time && (!from.compile_time))
	{
		// cannot initalize a compile time variable from a non-compile-time var
		return false;
	}
	// can initalize a number from a const number (with copy)
	return true;
}

bool TypeChecker::can_assign(const TypeName& to, const TypeName& from)
{
	if (to.compile_time)
	{
		// can never reassign a compile time variable
		return false;
	}
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
	const Variable* info = this->env->get_variable(expr.name.lexeme);
	if (!info)
	{
		this->error(expr.name, std::string("Attempted to assign variable \"") + expr.name.lexeme + "\" before it was defined.");
		return nullptr;
	}
	std::unique_ptr<TypeName> var_type;
	if (this->types.count(info->type().underlying()))
	{
		var_type = std::make_unique<TypeName>(info->type());
	}
	if (!var_type)
	{
		this->error(expr.name, std::string("No such type exists with name ") + info->type().type_name() + " for assigning to " + expr.name.lexeme);
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
	expr.type = *value_type;
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
	expr.callee->type = function_type.type;
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
	TypeName* type = new TypeName(function_type.return_type());
	expr.type = *type;
	return type;
}

void* TypeChecker::visitExprDeviceLoad(Expr::DeviceLoad& expr)
{
	std::unique_ptr<TypeName> device = this->accept(*expr.device);
	if (device)
	{
		if (!device->const_unqualified_equals(this->t_number))
		{
			this->error(expr.logic_type, std::string("Cannot perform a dload operation on a non-number device (device type was ") + device->type_name());
		}
	}
	if (!expr.logic_type.literal.is_string())
	{
		this->error(expr.logic_type, "A dload operation requires a string literal for the logic type.");
	}
	if (!this->types.count(expr.operation_type))
	{
		this->error(expr.logic_type, std::string("No type exists with name ") + expr.operation_type.type_name());
		return nullptr;
	}
	expr.type = expr.operation_type;
	return new TypeName(expr.type);
}

void* TypeChecker::visitExprLogical(Expr::Logical& expr)
{
	std::unique_ptr<TypeName> left_type = this->accept(*expr.left);
	std::unique_ptr<TypeName> right_type = this->accept(*expr.right);
	if (!left_type || !right_type)
	{
		return nullptr;
	}
	if (!left_type->const_unqualified_equals(this->t_void) || !right_type->const_unqualified_equals(this->t_boolean))
	{
		this->error(expr.op, std::string("Attempted to logically compare ") + left_type->type_name() + " with " +
			right_type->type_name() + " (requires boolean operations)");
		return nullptr;
	}
	expr.type = *left_type;
	return left_type.release();
}

bool TypeChecker::is_intrinsic(const TypeName& type)
{
	return type.const_unqualified_equals(TypeChecker::t_number) ||
		type.const_unqualified_equals(TypeChecker::t_boolean) ||
		type.const_unqualified_equals(TypeChecker::t_string);
}

void* TypeChecker::visitStmtDeviceSet(Stmt::DeviceSet& expr)
{
	if (this->current_pass == Pass::Linking)
	{
		return nullptr;
	}
	std::unique_ptr<TypeName> device = this->accept(*expr.device);
	if (device)
	{
		if (!device->const_unqualified_equals(this->t_number))
		{
			this->error(expr.token, "A dset operation requires a numerical device id.");
		}
	}
	if (!expr.logic_type.literal.is_string())
	{
		this->error(expr.logic_type, "A dset operation requires a string literal for the logic type.");
	}
	std::unique_ptr<TypeName> value_type = this->accept(*expr.value);
	if (value_type)
	{
		if (!this->is_intrinsic(*value_type))
		{
			this->error(expr.token, "Cannot set device value to a non-intrinsic type (number, boolean).");
		}
		if (!value_type->const_unqualified_equals(this->t_number) && !value_type->const_unqualified_equals(this->t_boolean))
		{
			this->error(expr.token, std::string("Cannot set device value to a value of type ") + value_type->type_name());
		}
	}
	return nullptr;
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

void* TypeChecker::visitStmtStatic(Stmt::Static& expr)
{
	return expr.var->accept(*this);
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
	if (underlying.type == this->t_void)
	{
		this->error(stmt.name, std::string("Attempted to declare variable ") + stmt.name.lexeme + " as void.");
		return nullptr;
	}
	bool success = this->env->define_variable(TypeID(stmt.type), stmt.name.lexeme, SymbolUseNode(stmt.downcast()));
	if (success)
	{
		if (const Variable* info = this->env->get_variable(stmt.name.lexeme))
		{
			this->symbol_visit_stmt_variable(stmt, info);
		}
	}
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
	this->env = this->env->spawn(&stmt);
	this->evaluate(stmt.statements);
	this->env = this->env->get_parent();
	return nullptr;
}

void* TypeChecker::visitStmtWhile(Stmt::While& expr)
{
	if (this->current_pass == Pass::Linking)
	{
		return nullptr;
	}
	std::unique_ptr<TypeName> condition_type = this->accept(*expr.condition);
	if (!condition_type)
	{
		expr.body->accept(*this);
		return nullptr;
	}
	if (!condition_type->const_unqualified_equals(this->t_boolean))
	{
		this->error(expr.token, "Attempted to loop on a non-boolean condition.");
	}
	expr.body->accept(*this);
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
	if (!condition_type->const_unqualified_equals(this->t_boolean))
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
	if (!this->env->get_parent())
	{
		this->error(expr.keyword, "Attempted to return from outside of a function.");
		return nullptr;
	}
	const std::string& function_name = *this->env->function();
	const Variable* info = this->env->get_variable(function_name);
	if (!info)
	{
		throw std::logic_error("TYPECHECKER ERROR: !INSIDE NON-EXISTENT FUNCTION!");
		return nullptr;
	}
	TypeID& function_type = this->types.at(info->type());
	if (function_type.return_type().underlying() == this->t_void)
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
		if (expr.name.lexeme == "main")
		{
			this->seen_main = true;
			// main function
			if (expr.params.size())
			{
				this->error(expr.name, "Entry point (main) cannot have any arguments.");
			}
			if (expr.return_type != this->t_void)
			{
				this->error(expr.name, "Entry point (main) must return void.");
			}
		}
		std::vector<FunctionParam> args;
		for (const auto& param : expr.params)
		{
			const TypeName& param_type = param.type;
			const Token& param_name = param.name;
			args.push_back({ param_type, param_name.lexeme });
		}

		TypeName function_type = types::get_function_signature(args, expr.return_type);
		TypeID function_type_id(
			function_type,
			expr.return_type,
			expr.name.lexeme,
			args
		);

		bool success = this->env->define_variable(function_type_id, expr.name.lexeme, expr.downcast());
		this->types.emplace(function_type, function_type_id);
		if (!success)
		{
			this->error(expr.name, std::string("Redefining function ") + expr.name.lexeme);
		}
		return nullptr;
	}

	// typechecking

	bool had_error = false;
	this->env = this->env->spawn_inside_function(&expr, expr.name.lexeme);
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
			this->env->define_variable(TypeID(param_type), param_name.lexeme, expr.downcast());
		}
	}
	if (!this->types.count(expr.return_type.underlying()))
	{
		this->error(expr.name, std::string("No type exists with name ") + expr.return_type.type_name());
		had_error = true;
	}
	this->evaluate(expr.body);
	this->env = this->env->get_parent();
	return nullptr;
}

TypeName get_function_signature(const std::vector<Stmt::Function::Param>& params, const TypeName& return_type)
{
	std::vector<FunctionParam> new_params;
	new_params.reserve(params.size());
	for (const auto& param : params)
	{
		new_params.push_back({
			param.type,
			param.name.lexeme
			});
	}
	return types::get_function_signature(new_params, return_type);
}

void TypeChecker::evaluate(std::vector<std::unique_ptr<Stmt>>& statements)
{
	for (auto& statement : statements)
	{
		this->program.add_statement(statement, *this->env);
		statement->accept(*this);
	}
}

void TypeChecker::error(const Token& token, const std::string& message)
{
	this->compiler.error(token.line, message);
}
