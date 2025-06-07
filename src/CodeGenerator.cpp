#include "CodeGenerator.h"
#include "Compiler.h"

CodeGenerator::CodeGenerator(Compiler& compiler, TypeCheckedProgram& program)
	:compiler(compiler), m_program(program)
{}

void CodeGenerator::error(const Token& token, const std::string& str)
{
	this->compiler.error(token.line, str);
	throw CodeGenerationError();
}

std::string CodeGenerator::generate()
{
	try
	{
		this->m_program.statement_environment()
	}
	catch (CodeGenerationError)
	{
		this->compiler.error(-1, "Error detected, aborting code generation.");
		return "";
	}
}
