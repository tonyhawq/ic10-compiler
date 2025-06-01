#pragma once

#include <stdexcept>

#include "AST.h"

class Interpreter : public Expr::Visitor, public Stmt::Visitor
{
public:
	Literal* evaluate(Expr& expression);
	bool is_truthy(Literal& value);
	bool is_equal(Literal& a, Literal& b);

	virtual void* visitExprBinary(Expr::Binary& expr) override;
	virtual void* visitExprGrouping(Expr::Grouping& expr) override;
	virtual void* visitExprLiteral(Expr::Literal& expr) override;
	virtual void* visitExprUnary(Expr::Unary& expr) override;
	virtual void* visitExprVariable(Expr::Variable& expr) override;
	virtual void* visitExprAssignment(Expr::Assignment& expr) override;

	virtual void* visitStmtExpression(Stmt::Expression& stmt) override;
	virtual void* visitStmtAsm(Stmt::Asm& stmt) override;
	virtual void* visitStmtPrint(Stmt::Print& stmt) override;
private:

};

