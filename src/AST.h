#pragma once

#include <memory>
#include <vector>

#include "Token.h"
#include "Typing.h"

#define NODE_VISIT_IMPL(base_name, node_name) virtual void* accept(base_name::Visitor& visitor) override {return visitor.visit##base_name##node_name(*this);}\
virtual std::string to_string() override {return #node_name;}

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

	template <typename T>
	T& as();

	virtual void* accept(Visitor&) = 0;

	virtual std::shared_ptr<Expr> clone() const = 0;
	static std::vector<std::shared_ptr<Expr>> clone_vec(const std::vector<std::shared_ptr<Expr>>& exprs);

	Expr(TypeName type) :type(type) {};
	virtual std::string to_string() { return "Expr"; }

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

template <typename T>
T& Expr::as()
{
	return dynamic_cast<T&>(*this);
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
	struct NoOp;
	class Visitor;

	virtual std::unique_ptr<Stmt> clone() const = 0;
	static std::vector<std::unique_ptr<Stmt>> clone_vec(const std::vector<std::unique_ptr<Stmt>>& exprs);

	Stmt() {};
	Stmt(const Stmt&) = delete;
	Stmt& operator=(const Stmt&) = delete;
	Stmt(Stmt&&) = default;
	Stmt& operator=(Stmt&&) = default;

	template <typename T>
	bool is();

	template <typename T>
	T& as();

	virtual void* accept(Stmt::Visitor&) = 0;
	virtual std::string to_string() { return "Stmt"; }
};

template <typename T>
bool Stmt::is()
{
	return typeid(T).hash_code() == typeid(*this).hash_code();
}

template <typename T>
T& Stmt::as()
{
	return dynamic_cast<T&>(*this);
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
	virtual void* visitStmtNoOp(Stmt::NoOp& expr) { return nullptr; }
private:
};

struct Stmt::NoOp : public Stmt
{
	NoOp() {};
	virtual std::unique_ptr<Stmt> clone() const override { return std::make_unique<Stmt::NoOp>(); }
	NODE_VISIT_IMPL(Stmt, NoOp)
};

struct Stmt::Variable : public Stmt
{
	Variable(TypeName type, const Token& name, std::shared_ptr<Expr> initalizer) :type(std::move(type)), name(name), initalizer(initalizer) {};
	TypeName type;
	Token name;
	std::shared_ptr<Expr> initalizer;
	virtual std::unique_ptr<Stmt> clone() const override { return std::make_unique<Stmt::Variable>(this->type, this->name, this->initalizer->clone()); }
	NODE_VISIT_IMPL(Stmt, Variable)
};

struct Stmt::DeviceSet : public Stmt
{
	DeviceSet(Token token, std::shared_ptr<Expr> device, Token logic_type, std::shared_ptr<Expr> value) :token(token), device(device), logic_type(logic_type),
		value(value) {};
	Token token;
	std::shared_ptr<Expr> device;
	Token logic_type;
	std::shared_ptr<Expr> value;
	virtual std::unique_ptr<Stmt> clone() const override { return std::make_unique<Stmt::DeviceSet>(this->token, this->device->clone(), this->logic_type, this->value->clone()); }
	NODE_VISIT_IMPL(Stmt, DeviceSet)
};

struct Stmt::Static : public Stmt
{
	Static(std::unique_ptr<Stmt> var) :var(std::move(var)) {};
	std::unique_ptr<Stmt> var;
	virtual std::unique_ptr<Stmt> clone() const override { return std::make_unique<Stmt::Static>(this->var->clone()); }
	NODE_VISIT_IMPL(Stmt, Static)
};

struct Stmt::Return : public Stmt
{
	Return(const Token& keyword, std::shared_ptr<Expr> value) :keyword(keyword), value(value) {};
	Token keyword;
	std::shared_ptr<Expr> value;
	virtual std::unique_ptr<Stmt> clone() const override { return std::make_unique<Stmt::Return>(this->keyword, this->value->clone()); }
	NODE_VISIT_IMPL(Stmt, Return)
};

struct Stmt::While : public Stmt
{
	While(const Token& token, std::shared_ptr<Expr> condition, std::unique_ptr<Stmt> body) :token(token), condition(condition), body(std::move(body)) {};
	Token token;
	std::shared_ptr<Expr> condition;
	std::unique_ptr<Stmt> body;
	virtual std::unique_ptr<Stmt> clone() const override { return std::make_unique<Stmt::While>(this->token, this->condition->clone(), this->body->clone()); }
	NODE_VISIT_IMPL(Stmt, While)
};

struct Stmt::Function : public Stmt
{
	struct Param
	{
		TypeName type;
		Token name;
	};
	Function(const Token& name, TypeName return_type, const std::vector<Param>& params, std::vector<std::unique_ptr<Stmt>>&& body)
		:name(name), return_type(std::move(return_type)), params(params), body(std::move(body)) { };
	Token name;
	TypeName return_type;
	std::vector<Param> params;
	std::vector<std::unique_ptr<Stmt>> body;
	virtual std::unique_ptr<Stmt> clone() const override { return std::make_unique<Stmt::Function>(this->name, this->return_type, this->params, Stmt::clone_vec(this->body)); }
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
	virtual std::unique_ptr<Stmt> clone() const override { return std::make_unique<Stmt::If>(this->token, this->condition->clone(), this->branch_true->clone(), this->branch_false->clone()); }
	NODE_VISIT_IMPL(Stmt, If)
};

struct Stmt::Block : public Stmt
{
	Block(std::vector<std::unique_ptr<Stmt>> statements, const Token& right_brace) :statements(std::move(statements)), right_brace(right_brace) {};
	Token right_brace;
	std::vector<std::unique_ptr<Stmt>> statements;
	virtual std::unique_ptr<Stmt> clone() const override { return std::make_unique<Stmt::Block>(Stmt::clone_vec(this->statements), this->right_brace); }
	NODE_VISIT_IMPL(Stmt, Block)
};

struct Stmt::Expression : public Stmt
{
	Expression(std::shared_ptr<Expr> expression) : expression(expression) {};
	std::shared_ptr<Expr> expression;
	virtual std::unique_ptr<Stmt> clone() const override { return std::make_unique<Stmt::Expression>(this->expression->clone()); }
	NODE_VISIT_IMPL(Stmt, Expression)
};

struct Stmt::Print : public Stmt
{
	Print(std::shared_ptr<Expr> expression) : expression(expression) {};
	std::shared_ptr<Expr> expression;
	virtual std::unique_ptr<Stmt> clone() const override { return std::make_unique<Stmt::Print>(this->expression->clone()); }
	NODE_VISIT_IMPL(Stmt, Print)
};

struct Stmt::Asm : public Stmt
{
	Asm(std::shared_ptr<Expr> literal, Token token) :literal(literal), token(token) {};
	std::shared_ptr<Expr> literal;
	Token token;
	virtual std::unique_ptr<Stmt> clone() const override { return std::make_unique<Stmt::Asm>(this->literal->clone(), this->token); }
	NODE_VISIT_IMPL(Stmt, Asm)
};

struct Expr::DeviceLoad : public Expr
{
	DeviceLoad(std::shared_ptr<Expr> device, Token logic_type, TypeName operation_type) :device(device), logic_type(logic_type), operation_type(operation_type),
		Expr(UNDEFINED_TYPE) {};
	std::shared_ptr<Expr> device;
	Token logic_type;
	TypeName operation_type;
	virtual std::shared_ptr<Expr> clone() const override { return std::make_shared<Expr::DeviceLoad>(this->device->clone(), this->logic_type, this->operation_type); }
	NODE_VISIT_IMPL(Expr, DeviceLoad)
};

struct Expr::Logical : public Expr
{
	Logical(std::shared_ptr<Expr> left, Token op, std::shared_ptr<Expr> right) :left(left), op(op), right(right), Expr(UNDEFINED_TYPE) {};
	std::shared_ptr<Expr> left;
	Token op;
	std::shared_ptr<Expr> right;
	virtual std::shared_ptr<Expr> clone() const override { return std::make_shared<Expr::Logical>(this->left->clone(), this->op, this->right->clone()); }
	NODE_VISIT_IMPL(Expr, Logical)
};

struct Expr::Call : public Expr
{
	Call(std::shared_ptr<Expr> callee, const Token& paren, std::vector<std::shared_ptr<Expr>> arguments) :callee(callee), paren(paren), arguments(arguments),
		Expr(UNDEFINED_TYPE) {};
	std::shared_ptr<Expr> callee;
	std::vector<std::shared_ptr<Expr>> arguments;
	Token paren;
	virtual std::shared_ptr<Expr> clone() const override { return std::make_shared<Expr::Call>(this->callee->clone(), this->paren, Expr::clone_vec(this->arguments)); }
	NODE_VISIT_IMPL(Expr, Call)
};

struct Expr::Variable : public Expr
{
	Variable(const Token& name) :name(name), Expr(UNDEFINED_TYPE) {};
	Token name;
	virtual std::shared_ptr<Expr> clone() const override { return std::make_shared<Expr::Variable>(this->name); }
	NODE_VISIT_IMPL(Expr, Variable)
};

struct Expr::Assignment : public Expr
{
	Assignment(const Token& name, std::shared_ptr<Expr> value) :name(name), value(value), Expr(UNDEFINED_TYPE) {};
	std::shared_ptr<Expr> value;
	Token name;
	virtual std::shared_ptr<Expr> clone() const override { return std::make_shared<Expr::Assignment>(this->name, this->value->clone()); }
	NODE_VISIT_IMPL(Expr, Assignment)
};

struct Expr::Binary : public Expr
{
	Binary(std::shared_ptr<Expr> left, const Token& op, std::shared_ptr<Expr> right) :left(left), op(op), right(right), Expr(UNDEFINED_TYPE) {};
	std::shared_ptr<Expr> left;
	Token op;
	std::shared_ptr<Expr> right;
	virtual std::shared_ptr<Expr> clone() const override { return std::make_shared<Expr::Binary>(this->left->clone(), this->op, this->right->clone()); }
	NODE_VISIT_IMPL(Expr, Binary)
};

struct Expr::Grouping : public Expr
{
	Grouping(std::shared_ptr<Expr> expression) :expression(expression), Expr(UNDEFINED_TYPE) {};
	std::shared_ptr<Expr> expression;
	virtual std::shared_ptr<Expr> clone() const override { return std::make_shared<Expr::Grouping>(this->expression->clone()); }
	NODE_VISIT_IMPL(Expr, Grouping)
};

struct Expr::Literal : public Expr
{
	Literal(const Token& literal) :literal(literal), Expr(UNDEFINED_TYPE) {};
	Token literal;
	virtual std::shared_ptr<Expr> clone() const override { return std::make_shared<Expr::Literal>(this->literal); }
	NODE_VISIT_IMPL(Expr, Literal)
};

struct Expr::Unary : public Expr
{
	Unary(const Token& op, std::shared_ptr<Expr> right) :op(op), right(right), Expr(UNDEFINED_TYPE) {};
	Token op;
	std::shared_ptr<Expr> right;
	virtual std::shared_ptr<Expr> clone() const override { return std::make_shared<Expr::Unary>(this->op, this->right->clone()); }
	NODE_VISIT_IMPL(Expr, Unary)
};
