#pragma once

#include "AST.h"
#include "TypeChecker.h"

class Compiler;

class Optimizer : public Expr::Visitor, public Stmt::Visitor
{
public:
	Optimizer(Compiler& compiler, TypeCheckedProgram& env);

	void optimize();

	void evaluate(std::vector<std::unique_ptr<Stmt>>& statements);

	std::unique_ptr<Stmt> visit_stmt(std::unique_ptr<Stmt>& stmt);

	virtual void* visitExprBinary(Expr::Binary& expr) override;
	virtual void* visitExprGrouping(Expr::Grouping& expr) override;
	virtual void* visitExprLiteral(Expr::Literal& expr) override;
	virtual void* visitExprUnary(Expr::Unary& expr) override;
	virtual void* visitExprVariable(Expr::Variable& expr) override;
	virtual void* visitExprAssignment(Expr::Assignment& expr) override;
	virtual void* visitExprCall(Expr::Call& expr) override;
	virtual void* visitExprLogical(Expr::Logical& expr) override;
	virtual void* visitExprDeviceLoad(Expr::DeviceLoad& expr) override;

	virtual void* visitStmtExpression(Stmt::Expression& stmt) override;
	virtual void* visitStmtAsm(Stmt::Asm& stmt) override;
	virtual void* visitStmtPrint(Stmt::Print& stmt) override;
	virtual void* visitStmtVariable(Stmt::Variable& stmt) override;
	virtual void* visitStmtBlock(Stmt::Block& stmt) override;
	virtual void* visitStmtIf(Stmt::If& stmt) override;
	virtual void* visitStmtFunction(Stmt::Function& expr) override;
	virtual void* visitStmtWhile(Stmt::While& expr) override;
	virtual void* visitStmtReturn(Stmt::Return& expr) override;
	virtual void* visitStmtStatic(Stmt::Static& expr) override;
	virtual void* visitStmtDeviceSet(Stmt::DeviceSet& expr) override;

private:
	Compiler& compiler;
	TypedEnvironment::Leaf* local_env;
	TypeCheckedProgram& m_env;
};
