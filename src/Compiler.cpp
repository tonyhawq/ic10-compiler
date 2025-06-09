#include "Compiler.h"
#include "Interpreter.h"
#include "TypeChecker.h"
#include "CodeGenerator.h"
#include "Timer.h"

void Compiler::compile(const std::string& path)
{
	Timer timer;
	std::ifstream file;
	file.open(path.c_str());
	if (!file.is_open())
	{
		throw std::runtime_error(std::string("File ") + path + " could not be opened.");
	}
	std::stringstream temp;
	temp << file.rdbuf();
	printf("Scanning...\n");
	timer.start();
	Scanner scanner(*this, temp.str());
	std::vector<Token> tokens = scanner.scan();
	printf("Scanning took %fms\n", timer.start() * 1000.0);
	printf("Parsing...\n");
	Parser parser(*this, std::move(tokens));
	std::vector<std::unique_ptr<Stmt>> program = parser.parse();
	printf("Parsing took %fms\n", timer.start() * 1000.0);
	printf("Typechecking...\n");
	TypeChecker checker(*this, std::move(program));
	TypeCheckedProgram env = checker.check();
	printf("Typing took %fms\n", timer.start() * 1000.0);
	if (this->had_error)
	{
		printf("Aborting before code generation.\n");
		return;
	}
	printf("Optimizing...\n");
	Optimizer optimizer(*this, env);
	optimizer.optimize();
	printf("Optimizing took %fms\n", timer.start() * 1000.0);
	printf("Generating code...\n");
	CodeGenerator generator(*this, env);
	std::string code = generator.generate();
	printf("Code generation took %fms\n", timer.start() * 1000.0);
	std::ofstream generated_file;
	generated_file.open((path + ".ic10").c_str());
	generated_file << code;
	generated_file.close();
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
