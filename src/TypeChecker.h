#pragma once

#include <unordered_map>
#include <string>
#include <list>

#include "OwningPtr.h"
#include "AST.h"
#include "Parser.h"

class Compiler;

struct TypeID
{
	struct FunctionParam;
	
	explicit TypeID(const TypeName& type);
	TypeID(const TypeName& type, bool is_function, std::unique_ptr<TypeName> return_type, std::unique_ptr<std::string> m_unmangled_name, std::unique_ptr<std::vector<FunctionParam>> m_arguments);
	explicit TypeID(const TypeID& other);

	TypeName type;
	TypeName& return_type();
	std::string& mangled_name();
	std::string& unmangled_name();
	std::vector<FunctionParam>& arguments();
	const TypeName& return_type() const;
	const std::string& mangled_name() const;
	const std::string& unmangled_name() const;
	const std::vector<FunctionParam>& arguments() const;

	bool is_function = false;
	std::unique_ptr<TypeName> m_return_type;
	std::unique_ptr<std::string> m_mangled_name;
	std::unique_ptr<std::string> m_unmangled_name;
	std::unique_ptr<std::vector<FunctionParam>> m_arguments;
};

struct TypeID::FunctionParam
{
	TypeName type;
	std::string name;
};

class Identifier
{
public:
	Identifier(const std::string& name);

	bool operator==(const Identifier& other) const;
	
	std::string name() const;
private:
	std::string m_name;
};

namespace std {
	template <>
	struct hash<Identifier> {
		std::size_t operator()(const Identifier& key) const {
			return std::hash<std::string>()(key.name());
		}
	};
}

class Variable
{
public:
	Variable(const Identifier& identifier, const TypeID& type);

	const TypeName& type() const;
	const TypeID& full_type() const;
	const Identifier& identifier() const;
private:
	Identifier m_identifier;
	TypeID m_type;
};

class TypedEnvironment
{
public:
	class Leaf;

	TypedEnvironment();
	~TypedEnvironment();
	TypedEnvironment(const TypedEnvironment& other) = delete;
	TypedEnvironment(TypedEnvironment&& other) = default;

	Leaf* root();
	size_t advance(Leaf* advance_for);
	void reassign(size_t id, Leaf* new_node);
private:
	std::unordered_map<size_t, Leaf*> leaves;
	size_t next_id;
	std::unique_ptr<Leaf> m_root;
};

class TypedEnvironment::Leaf
{
public:
	explicit Leaf(TypedEnvironment& progenitor);
	Leaf(Leaf* parent);
	~Leaf();
	Leaf(const Leaf& other) = delete;
	Leaf(Leaf&& other) noexcept;

	TypedEnvironment& progenitor();

	Leaf* spawn_inside_function(const std::string& name);
	Leaf* spawn();
	Leaf* get_parent();
	size_t leaf_id() const;

	const std::string* function() const;
	bool define_variable(const TypeID& type, const Identifier& identifier);
	enum class LongerLived
	{
		A,
		B,
		Same,
		Undefined
	};
	LongerLived longer_lived(const Variable& a, const Variable& b);

	const std::list<Leaf>& children() const;
	int scopes_to_definition(const Identifier& identifier) const;
	const Variable* get_variable(const Identifier& identifier) const;
private:
	TypedEnvironment* m_progenitor;
	size_t id;
	std::unique_ptr<std::string> m_function_name;
	Leaf* parent;
	std::list<Leaf> m_children;
	std::unordered_map<Identifier, Variable> m_variables;
};

class TypeCheckedProgram
{
public:
	class Statement;

	TypeCheckedProgram(std::vector<std::unique_ptr<Stmt>> statements);
	void add_statement(std::unique_ptr<Stmt>& statement, TypedEnvironment::Leaf& containing_env);
	
	const std::vector<std::unique_ptr<Stmt>>& statements() const;
	std::vector<std::unique_ptr<Stmt>>& statements();
	TypedEnvironment& env();
	TypedEnvironment::Leaf& statement_environment(Stmt* statement);
private:
	std::unordered_map<Stmt*, TypedEnvironment::Leaf*> ptr_to_leaf;
	TypedEnvironment m_env;
	std::vector<std::unique_ptr<Stmt>> m_statements;
};

class TypeChecker : public Expr::Visitor, public Stmt::Visitor
{
public:
	enum class Pass
	{
		Linking,
		TypeChecking,
	};
	
	struct Operator;
	class OperatorOverload;

	TypeChecker(Compiler& compiler, std::vector<std::unique_ptr<Stmt>> statements);

	void define_library_function(const std::string& name, const TypeName& return_type, const std::vector<TypeName>& params);
	void define_operator(TokenType type, const std::string& name, std::vector<OwningPtr<OperatorOverload>> overloads);

	TypeCheckedProgram check();
	void evaluate(std::vector<std::unique_ptr<Stmt>>& statements);
	void error(const Token& token, const std::string& message);

	static bool can_initalize(const TypeName& to, const TypeName& from);
	static bool can_assign(const TypeName& to, const TypeName& from);

	std::unique_ptr<TypeName> accept(Expr& expression);

	static std::string get_mangled_function_name(const std::string& name, const std::vector<TypeID::FunctionParam> params, const TypeName& return_type);
	static TypeName get_function_signature(const std::vector<TypeID::FunctionParam>& params, const TypeName& return_type);
	static TypeName get_function_signature(const std::vector<Stmt::Function::Param>& params, const TypeName& return_type);

	virtual void* visitExprBinary(Expr::Binary& expr) override;
	virtual void* visitExprGrouping(Expr::Grouping& expr) override;
	virtual void* visitExprLiteral(Expr::Literal& expr) override;
	virtual void* visitExprUnary(Expr::Unary& expr) override;
	virtual void* visitExprVariable(Expr::Variable& expr) override;
	virtual void* visitExprAssignment(Expr::Assignment& expr) override;
	virtual void* visitExprCall(Expr::Call& expr) override;
	virtual void* visitExprLogical(Expr::Logical& expr) override;

	virtual void* visitStmtExpression(Stmt::Expression& stmt) override;
	virtual void* visitStmtAsm(Stmt::Asm& stmt) override;
	virtual void* visitStmtPrint(Stmt::Print& stmt) override;
	virtual void* visitStmtVariable(Stmt::Variable& stmt) override;
	virtual void* visitStmtBlock(Stmt::Block& stmt) override;
	virtual void* visitStmtIf(Stmt::If& stmt) override;
	virtual void* visitStmtFunction(Stmt::Function& expr) override;
	virtual void* visitStmtReturn(Stmt::Return& expr) override;
	virtual void* visitStmtStatic(Stmt::Static& expr) override;
private:
	bool seen_main = false;

	TypeCheckedProgram program;
	TypedEnvironment::Leaf* env;

	std::unordered_map<TypeName, TypeID> types;
	std::unordered_map<TokenType, Operator> operator_types;

	Pass current_pass;
	Compiler& compiler;
};

struct TypeChecker::Operator
{
	class ReturnType;

	ReturnType return_type(const std::vector<TypeName>& args);

	Operator(const std::string& operator_name, std::vector<std::unique_ptr<OperatorOverload>> overloads) :operator_name(operator_name), overloads(std::move(overloads)) {};

	std::string operator_name;
	std::vector<std::unique_ptr<OperatorOverload>> overloads;
};

class TypeChecker::OperatorOverload
{
public:
	class Addressof;
	class Deref;

	OperatorOverload(const TypeName& return_type, const std::vector<TypeName>& arg_types);
	virtual ~OperatorOverload();
	virtual Operator::ReturnType return_type(const std::vector<TypeName>& args) const;
protected:
	TypeName m_return_type;
	std::vector<TypeName> arg_types;
};

class TypeChecker::OperatorOverload::Addressof : public TypeChecker::OperatorOverload
{
public:
	Addressof();
	virtual ~Addressof();
	virtual Operator::ReturnType return_type(const std::vector<TypeName>& args) const;
};

class TypeChecker::OperatorOverload::Deref : public TypeChecker::OperatorOverload
{
public:
	Deref();
	virtual ~Deref();
	virtual Operator::ReturnType return_type(const std::vector<TypeName>& args) const;
};

class TypeChecker::Operator::ReturnType
{
public:
	ReturnType(std::string failure) : failure_mode(std::make_unique<std::string>(std::move(failure))), m_type(nullptr) {};
	ReturnType(const TypeName& type) : failure_mode(nullptr), m_type(std::make_unique<TypeName>(type)) {};

	explicit ReturnType() : failure_mode(nullptr), m_type(nullptr) {};

	bool failed() const;
	std::string& failure();
	const std::string& failure() const;
	const TypeName& type() const;
private:
	std::unique_ptr<std::string> failure_mode;
	std::unique_ptr<TypeName> m_type;
};
