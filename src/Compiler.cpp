#include "Compiler.h"
#include "Interpreter.h"
#include "TypeChecker.h"

void Compiler::compile(const std::string& path)
{
	std::ifstream file;
	file.open(path.c_str());
	if (!file.is_open())
	{
		throw std::runtime_error(std::string("File ") + path + " could not be opened.");
	}
	std::stringstream temp;
	temp << file.rdbuf();
	printf("Scanning...\n");
	Scanner scanner(*this, temp.str());
	std::vector<Token> tokens = scanner.scan();
	printf("Parsing...\n");
	Parser parser(*this, std::move(tokens));
	std::vector<std::unique_ptr<Stmt>> program = parser.parse();
	printf("Typechecking...\n");
	TypeChecker checker(*this, std::move(program));
	TypeCheckedProgram env = checker.check();
	if (this->had_error)
	{
		printf("Aborting before code generation.\n");
		return;
	}
	printf("Optimizing...\n");
	Optimizer optimizer(*this, env);
	optimizer.optimize();
}

void Compiler::error(int line, std::string message)
{
	this->had_error = true;
	this->report(line, "", message);
}

void Compiler::report(int line, std::string where, std::string message)
{
	printf("Error on line %i: %s %s\n", line, where.c_str(), message.c_str());
}
