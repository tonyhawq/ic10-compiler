#pragma once

#include "CIRProgram.h"

class ASTFlattener : public Stmt::Visitor, public Expr::Visitor
{
public:
	ASTFlattener(Compiler& compiler, TypeCheckedProgram prog);

	CIRProgram flatten_program();

	void visit(Stmt& stmt);
	cir::Temporary flatten(Expr& expr);
	void visit(std::unique_ptr<Stmt>& stmt);
	cir::Temporary flatten(std::shared_ptr<Expr>& expr);

	virtual void* visitStmtExpression(Stmt::Expression& expr) override;
	virtual void* visitStmtAsm(Stmt::Asm& expr) override;
	virtual void* visitStmtPrint(Stmt::Print& expr) override;
	virtual void* visitStmtVariable(Stmt::Variable& expr) override;
	virtual void* visitStmtBlock(Stmt::Block& expr) override;
	virtual void* visitStmtIf(Stmt::If& expr) override;
	virtual void* visitStmtFunction(Stmt::Function& expr) override;
	virtual void* visitStmtReturn(Stmt::Return& expr) override;
	virtual void* visitStmtWhile(Stmt::While& expr) override;
	virtual void* visitStmtStatic(Stmt::Static& expr) override;

	virtual void* visitExprBinary(Expr::Binary& expr) override;
	virtual void* visitExprGrouping(Expr::Grouping& expr) override;
	virtual void* visitExprLiteral(Expr::Literal& expr) override;
	virtual void* visitExprUnary(Expr::Unary& expr) override;
	virtual void* visitExprVariable(Expr::Variable& expr) override;
	virtual void* visitExprAssignment(Expr::Assignment& expr) override;
	virtual void* visitExprCall(Expr::Call& expr) override;
	virtual void* visitExprLogical(Expr::Logical& expr) override;

	cir::Temporary allocate();
	void emit(cir::Instruction::ref&& instruction);
private:
	std::vector<cir::Instruction::ref> instructions;
	size_t current_temporary;
	std::unordered_map<const Variable*, cir::Temporary> variable_mappings;
	Compiler& compiler;
	TypeCheckedProgram& program;
};

