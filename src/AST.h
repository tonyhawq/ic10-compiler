#pragma once

#include <memory>
#include <vector>

#include "Token.h"

#define NODE_VISIT_IMPL(base_name, node_name) virtual void* accept(base_name::Visitor& visitor) override {return visitor.visit##base_name##node_name(*this);}

#define UNDEFINED_TYPE TypeName("undefined")
#define VOID_TYPE TypeName("void")

struct TypeName
{
	explicit TypeName(const std::string& type_name) :m_type_name(std::make_unique<std::string>(type_name)), constant(false), pointer(false), compile_time(false) {};
	TypeName(bool constant, const std::string& type_name) :m_type_name(std::make_unique<std::string>(type_name)), constant(constant), pointer(false), compile_time(false) {};
	static TypeName create_compile_time(const std::string& type_name);
	TypeName(const TypeName& other);
	TypeName& operator=(const TypeName& other);
	TypeName(TypeName&& other) noexcept = default;
	TypeName& operator=(TypeName&& other) noexcept = default;

	void make_pointer_type(bool constant, bool nullable);

	TypeName underlying() const;
	std::string type_name() const;
	bool nullable() const;

	int height() const;

	bool const_unqualified_equals(const TypeName& other) const;
	bool operator==(const TypeName& other) const;
	bool operator!=(const TypeName& other) const;

	bool compile_time;
	bool constant;
	bool pointer;
	std::unique_ptr<bool> m_nullable;
	std::unique_ptr<TypeName> m_pointed_to_type;
	std::unique_ptr<std::string> m_type_name;
private:
	TypeName() : constant(false), compile_time(false), pointer(false), m_pointed_to_type(nullptr), m_type_name(nullptr) {};
};

namespace std {
	template <>
	struct hash<TypeName> {
		std::size_t operator()(const TypeName& key) const {
			return std::hash<std::string>()(key.type_name());
		}
	};
}

struct Expr
{
	struct Binary;
	struct Grouping;
	struct Literal;
	struct Unary;
	struct Variable;
	struct Assignment;
	struct Call;
	struct Logical;
	struct DeviceLoad;
	class Visitor;

	template <typename T>
	bool is();

	virtual void* accept(Visitor&) = 0;
	
	Expr(TypeName type) :type(type) {};
	
	TypeName type;
};

class Expr::Visitor
{
public:
	virtual void* visitExprBinary(Expr::Binary& expr) { return nullptr; };
	virtual void* visitExprGrouping(Expr::Grouping& expr) { return nullptr; };
	virtual void* visitExprLiteral(Expr::Literal& expr) { return nullptr; };
	virtual void* visitExprUnary(Expr::Unary& expr) { return nullptr; };
	virtual void* visitExprVariable(Expr::Variable& expr) { return nullptr; };
	virtual void* visitExprAssignment(Expr::Assignment& expr) { return nullptr; };
	virtual void* visitExprCall(Expr::Call& expr) { return nullptr; }
	virtual void* visitExprLogical(Expr::Logical& expr) { return nullptr; }
	virtual void* visitExprDeviceLoad(Expr::DeviceLoad& expr) { return nullptr; }
private:
};

template <typename T>
bool Expr::is()
{
	return typeid(T).hash_code() == typeid(*this).hash_code();
}

struct Stmt
{
	struct Expression;
	struct Asm;
	struct Print;
	struct Variable;
	struct Block;
	struct If;
	struct Function;
	struct Return;
	struct While;
	struct Static;
	struct DeviceSet;
	class Visitor;

	template <typename T>
	bool is();

	virtual void* accept(Stmt::Visitor&) = 0;
};

template <typename T>
bool Stmt::is()
{
	return typeid(T).hash_code() == typeid(*this).hash_code();
}

class Stmt::Visitor
{
public:
	virtual void* visitStmtExpression(Stmt::Expression& expr) { return nullptr; };
	virtual void* visitStmtAsm(Stmt::Asm& expr) { return nullptr; };
	virtual void* visitStmtPrint(Stmt::Print& expr) { return nullptr; };
	virtual void* visitStmtVariable(Stmt::Variable& expr) { return nullptr; };
	virtual void* visitStmtBlock(Stmt::Block& expr) { return nullptr; }
	virtual void* visitStmtIf(Stmt::If& expr) { return nullptr; }
	virtual void* visitStmtFunction(Stmt::Function& expr) { return nullptr; }
	virtual void* visitStmtReturn(Stmt::Return& expr) { return nullptr; }
	virtual void* visitStmtWhile(Stmt::While& expr) { return nullptr; }
	virtual void* visitStmtStatic(Stmt::Static& expr) { return nullptr; }
	virtual void* visitStmtDeviceSet(Stmt::DeviceSet& expr) { return nullptr; }
private:
};

struct Stmt::Static : public Stmt
{
	Static(std::unique_ptr<Stmt::Variable> var) :var(std::move(var)) {};
	std::unique_ptr<Stmt::Variable> var;
	NODE_VISIT_IMPL(Stmt, Static)
};

struct Stmt::Return : public Stmt
{
	Return(const Token& keyword, std::shared_ptr<Expr> value) :keyword(keyword), value(value) {};
	Token keyword;
	std::shared_ptr<Expr> value;
	NODE_VISIT_IMPL(Stmt, Return)
};

struct Stmt::While : public Stmt
{
	While(std::shared_ptr<Expr> condition, std::unique_ptr<Stmt> body) :condition(condition), body(std::move(body)) {};
	std::shared_ptr<Expr> condition;
	std::unique_ptr<Stmt> body;
	NODE_VISIT_IMPL(Stmt, While)
};

struct Stmt::Function : public Stmt
{
	struct Param
	{
		TypeName type;
		Token name;
	};
	Function(const Token& name, TypeName return_type, const std::vector<Param>& params, std::vector<std::unique_ptr<Stmt>> body)
		:name(name), return_type(std::move(return_type)), params(params), body(std::move(body)) { };
	Token name;
	TypeName return_type;
	std::vector<Param> params;
	std::vector<std::unique_ptr<Stmt>> body;
	NODE_VISIT_IMPL(Stmt, Function)
};

struct Stmt::If : public Stmt
{
	If(const Token& token, std::shared_ptr<Expr> condition, std::unique_ptr<Stmt> branch_true, std::unique_ptr<Stmt> branch_false) :condition(condition),
		branch_true(std::move(branch_true)), branch_false(std::move(branch_false)), token(token) {};
	std::shared_ptr<Expr> condition;
	std::unique_ptr<Stmt> branch_true;
	std::unique_ptr<Stmt> branch_false;
	Token token;
	NODE_VISIT_IMPL(Stmt, If)
};

struct Stmt::Block : public Stmt
{
	Block(std::vector<std::unique_ptr<Stmt>> statements, const Token& right_brace) :statements(std::move(statements)), right_brace(right_brace) {};
	Token right_brace;
	std::vector<std::unique_ptr<Stmt>> statements;
	NODE_VISIT_IMPL(Stmt, Block)
};

struct Stmt::Variable : public Stmt
{
	Variable(TypeName type, const Token& name, std::shared_ptr<Expr> initalizer) :type(std::move(type)), name(name), initalizer(initalizer) {};
	TypeName type;
	Token name;
	std::shared_ptr<Expr> initalizer;
	NODE_VISIT_IMPL(Stmt, Variable)
};

struct Stmt::Expression : public Stmt
{
	Expression(std::shared_ptr<Expr> expression) : expression(expression) {};
	std::shared_ptr<Expr> expression;
	NODE_VISIT_IMPL(Stmt, Expression)
};

struct Stmt::Print : public Stmt
{
	Print(std::shared_ptr<Expr> expression) : expression(expression) {};
	std::shared_ptr<Expr> expression;
	NODE_VISIT_IMPL(Stmt, Print)
};

struct Stmt::Asm : public Stmt
{
	Asm(std::shared_ptr<Expr> literal) :literal(literal) {};
	std::shared_ptr<Expr> literal;
	NODE_VISIT_IMPL(Stmt, Asm)
};

struct Expr::DeviceLoad : public Expr
{
	DeviceLoad();
	Token logic_type;
	std::shared_ptr<Expr> device;
	NODE_VISIT_IMPL(Expr, DeviceLoad)
};

struct Expr::Logical : public Expr
{
	Logical(std::shared_ptr<Expr> left, Token op, std::shared_ptr<Expr> right) :left(left), op(op), right(right), Expr(UNDEFINED_TYPE) {};
	std::shared_ptr<Expr> left;
	Token op;
	std::shared_ptr<Expr> right;
	NODE_VISIT_IMPL(Expr, Logical)
};

struct Expr::Call : public Expr
{
	Call(std::shared_ptr<Expr> callee, const Token& paren, std::vector<std::shared_ptr<Expr>> arguments) :callee(callee), paren(paren), arguments(arguments),
		Expr(UNDEFINED_TYPE) {};
	std::shared_ptr<Expr> callee;
	std::vector<std::shared_ptr<Expr>> arguments;
	Token paren;
	NODE_VISIT_IMPL(Expr, Call)
};

struct Expr::Variable : public Expr
{
	Variable(const Token& name) :name(name), Expr(UNDEFINED_TYPE) {};
	Token name;
	NODE_VISIT_IMPL(Expr, Variable)
};

struct Expr::Assignment : public Expr
{
	Assignment(const Token& name, std::shared_ptr<Expr> value) :name(name), value(value), Expr(UNDEFINED_TYPE) {};
	std::shared_ptr<Expr> value;
	Token name;
	NODE_VISIT_IMPL(Expr, Assignment)
};

struct Expr::Binary : public Expr
{
	Binary(std::shared_ptr<Expr> left, const Token& op, std::shared_ptr<Expr> right) :left(left), op(op), right(right), Expr(UNDEFINED_TYPE) {};
	std::shared_ptr<Expr> left;
	Token op;
	std::shared_ptr<Expr> right;
	NODE_VISIT_IMPL(Expr, Binary)
};

struct Expr::Grouping : public Expr
{
	Grouping(std::shared_ptr<Expr> expression) :expression(expression), Expr(UNDEFINED_TYPE) {};
	std::shared_ptr<Expr> expression;
	NODE_VISIT_IMPL(Expr, Grouping)
};

struct Expr::Literal : public Expr
{
	Literal(const Token& literal) :literal(literal), Expr(UNDEFINED_TYPE) {};
	Token literal;
	NODE_VISIT_IMPL(Expr, Literal)
};

struct Expr::Unary : public Expr
{
	Unary(const Token& op, std::shared_ptr<Expr> right) :op(op), right(right), Expr(UNDEFINED_TYPE) {};
	Token op;
	std::shared_ptr<Expr> right;
	NODE_VISIT_IMPL(Expr, Unary)
};
