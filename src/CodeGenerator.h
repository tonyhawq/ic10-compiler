#pragma once

#include "TypeChecker.h"

typedef std::shared_ptr<int> RegisterHandle;

struct StackVariable
{
	std::string name;
	int offset;
	int size;
};

class StackEnvironment
{
public:
	explicit StackEnvironment();
	explicit StackEnvironment(StackEnvironment* parent);
	~StackEnvironment();
	StackEnvironment(StackEnvironment&& other) noexcept = default;
	StackEnvironment(const StackEnvironment& other) = delete;
	StackEnvironment& operator=(const StackEnvironment& other) = delete;
	StackEnvironment& operator=(StackEnvironment&& other)  noexcept;

	void set_frame_size(int value);
	int frame_size() const;

	void define(const std::string& name, int size);
	std::unique_ptr<StackVariable> resolve(const std::string& name);

	StackEnvironment* pop_to_function();
	StackEnvironment* spawn();
	StackEnvironment* spawn_in_function(const std::string& name);
	StackEnvironment* pop();
private:
	StackEnvironment* child;
	StackEnvironment* parent;
	std::unique_ptr<std::string> m_function_name;
	std::unordered_map<std::string, StackVariable> variables;
	int m_frame_size;
};

class Register
{
public:
	explicit Register(RegisterHandle handle);
	~Register();
	Register(const Register& other) = default;
	Register(Register&& other) = default;
	
	RegisterHandle* release();
	RegisterHandle handle();
	int index() const;
private:
	RegisterHandle register_handle;
};

class RegisterAllocator
{
public:
	RegisterAllocator(size_t register_count);
	~RegisterAllocator();
	Register allocate();
private:
	std::vector<RegisterHandle> registers;
};

struct Placeholder
{
	int id;
};

class CodeGenerator : public Expr::Visitor, public Stmt::Visitor
{
public:
	CodeGenerator(Compiler& compiler, TypeCheckedProgram& program);
	void error(const Token& token, const std::string& str);

	std::string generate();

	std::unique_ptr<RegisterHandle> visit_expr_raw(std::shared_ptr<Expr> expression);
	Register visit_expr(std::shared_ptr<Expr> expression);

	void visit_stmt(std::unique_ptr<Stmt>& stmt);

	virtual void* visitExprBinary(Expr::Binary& expr);
	virtual void* visitExprGrouping(Expr::Grouping& expr);
	virtual void* visitExprLiteral(Expr::Literal& expr);
	virtual void* visitExprUnary(Expr::Unary& expr);
	virtual void* visitExprVariable(Expr::Variable& expr);
	virtual void* visitExprAssignment(Expr::Assignment& expr);
	virtual void* visitExprCall(Expr::Call& expr);
	virtual void* visitExprLogical(Expr::Logical& expr);

	virtual void* visitStmtExpression(Stmt::Expression& expr);
	virtual void* visitStmtAsm(Stmt::Asm& expr);
	virtual void* visitStmtPrint(Stmt::Print& expr);
	virtual void* visitStmtVariable(Stmt::Variable& expr);
	virtual void* visitStmtBlock(Stmt::Block& expr);
	virtual void* visitStmtIf(Stmt::If& expr);
	virtual void* visitStmtFunction(Stmt::Function& expr);
	virtual void* visitStmtReturn(Stmt::Return& expr);
	virtual void* visitStmtWhile(Stmt::While& expr);
private:
	std::string get_register_name(const Register& reg);

	void emit_raw(const std::string& val);
	void emit_register_use(const Register& reg);
	void emit_register_use(const Register& a, const Register& b);
	void emit_peek_stack_from_into(Register* reg);
	void emit_load_into(int offset, const std::string& register_label);
	void emit_load_into(int offset, Register* reg);
	Placeholder emit_placeholder();
	void emit_replace_placeholder(const Placeholder& placeholder, const std::string& val);

	int current_placeholder_value;
	int current_line();

	void push_env();
	void push_env(const std::string& name);
	void pop_env();

	StackEnvironment* env;
	StackEnvironment top_env;
	RegisterAllocator allocator;
	std::string code;
	Compiler& compiler;
	TypeCheckedProgram& m_program;
};

