#pragma once

#include <unordered_map>
#include <string>

#include "AST.h"
#include "Parser.h"

class Compiler;

struct TypeID
{
	struct FunctionParam;

	TypeName type;
	TypeName& return_type();
	std::string& unmangled_name();
	std::vector<FunctionParam>& arguments();
	const TypeName& return_type() const;
	const std::string& unmangled_name() const;
	const std::vector<FunctionParam>& arguments() const;

	bool is_function = false;
	std::unique_ptr<TypeName> m_return_type;
	std::unique_ptr<std::string> m_unmangled_name;
	std::unique_ptr<std::vector<FunctionParam>> m_arguments;
};

struct TypeID::FunctionParam
{
	TypeName type;
	std::string name;
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

	TypeChecker(Compiler& compiler, std::vector<std::unique_ptr<Stmt>>& statements);
	Environment* env;

	std::unordered_map<TypeName, TypeID> types;
	std::unordered_map<TokenType, Operator> operator_types;

	void define_library_function(const std::string& name, const TypeName& return_type, const std::vector<TypeName>& params);
	void define_operator(TokenType type, const std::string& name, std::vector<Operator::Overload> overloads);

	void check();
	void evaluate(std::vector<std::unique_ptr<Stmt>>& statements);
	void error(const Token& token, const std::string& message);

	static bool can_initalize(const TypeName& to, const TypeName& from);
	static bool can_assign(const TypeName& to, const TypeName& from);

	std::unique_ptr<TypeName> accept(Expr& expression);

	static std::string get_mangled_function_name(const std::string& name, const std::vector<TypeID::FunctionParam> params, const TypeName& return_type);
	static TypeName get_function_signature(const std::vector<TypeID::FunctionParam> params, const TypeName& return_type);

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
private:
	Pass current_pass;
	std::vector<std::unique_ptr<Stmt>>& statements_to_typecheck;
	Compiler& compiler;
};

struct TypeChecker::Operator
{
	class Overload;
	class ReturnType;

	ReturnType return_type(const std::vector<TypeName>& args);

	Operator(const std::string& operator_name, std::vector<Overload> overloads) :operator_name(operator_name), overloads(std::move(overloads)) {};

	std::string operator_name;
	std::vector<std::unique_ptr<Overload>> overloads;
};

class TypeChecker::Operator::Overload
{
public:
	class SimpleOverload;

	Overload(const TypeName& return_type);
	virtual ~Overload();
	virtual ReturnType return_type(const std::vector<TypeName>& args) const;
protected:
	TypeName m_return_type;
	std::vector<TypeName> arg_types;
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
