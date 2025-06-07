#pragma once

#include "TypeChecker.h"

class CodeGenerator : public Expr::Visitor, public Stmt::Visitor
{
public:
	CodeGenerator(Compiler& compiler, TypeCheckedProgram& program);
	void error(const Token& token, const std::string& str);

	std::string generate();
private:
	Compiler& compiler;
	TypeCheckedProgram& m_program;
};

