#pragma once

#include "TypeChecker.h"

typedef std::shared_ptr<int> RegisterHandle;

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

class CGEnvironment
{
public:

private:
};

class CodeGenerator : public Expr::Visitor, public Stmt::Visitor
{
public:
	CodeGenerator(Compiler& compiler, TypeCheckedProgram& program);
	void error(const Token& token, const std::string& str);

	std::string generate();

	RegisterHandle* visit(std::shared_ptr<Expr> expression);

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
	void emit_raw(const std::string& val);
	void emit_register_use(const Register& reg);
	void emit_register_use(const Register& a, const Register& b);
	void emit_peek_stack_from_into(Register* reg);

	std::vector<CGEnvironment> stack;
	int current_frame_sp;
	RegisterAllocator allocator;
	std::string code;
	Compiler& compiler;
	TypeCheckedProgram& m_program;
};

