#include "Compiler.h"
#include "Interpreter.h"
#include "TypeChecker.h"
#include "CodeGenerator.h"
#include "Timer.h"

const std::unordered_map<std::string, Builtin::reference_type>& Compiler::builtins()
{
	static std::unordered_map<std::string, Builtin::reference_type> _builtins = {
		{"load", (new Builtin("load", TypeName("number"), {{TypeName("number"), Builtin::token_literal_string("dummy")}}))->add_asm("l $&dummy d0 Setting").add_return("dummy").refit()}
	};
	return _builtins;
}

void Compiler::compile(const std::string& path)
{
	Timer total_timer;
	Timer timer;
	std::ifstream file;
	file.open(path.c_str());
	if (!file.is_open())
	{
		throw std::runtime_error(std::string("File ") + path + " could not be opened.");
	}
	printf("Compiling source file %s\n", path.c_str());
	std::stringstream temp;
	temp << file.rdbuf();
	this->info("Scanning...");
	timer.start();
	Scanner scanner(*this, temp.str());
	std::vector<Token> tokens = scanner.scan();
	this->info(std::string("Scanning took ") + std::to_string(timer.start() * 1000.0) + "ms.");
	this->info("Parsing...");
	Parser parser(*this, std::move(tokens));
	std::vector<std::unique_ptr<Stmt>> program = parser.parse();
	for (const auto& builtin : this->builtins())
	{
		program.push_back(builtin.second->splice());
	}
	this->info(std::string("Parsing took ") + std::to_string(timer.start() * 1000.0) + "ms.");
	this->info("Typechecking...");
	TypeChecker checker(*this, std::move(program));
	TypeCheckedProgram env = checker.check();
	this->info(std::string("Typing took ") + std::to_string(timer.start() * 1000.0) + "ms.");
	if (this->had_error)
	{
		printf("Errors detected.\n");
		printf("Aborting before code generation.\n");
		return;
	}
	this->info("Optimizing...");
	Optimizer optimizer(*this, env);
	optimizer.optimize();
	this->info(std::string("Optimizing took ") + std::to_string(timer.start() * 1000.0) + "ms.");
	this->info("Generating code...");
	CodeGenerator generator(*this, env);
	std::string code = generator.generate();
	this->info(std::string("Code generation took ") + std::to_string(timer.start() * 1000.0) + "ms.");
	if (this->had_error)
	{
		printf("Errors detected.\n");
		printf("Aborting before writing file.\n");
		return;
	}
	std::ofstream generated_file;
	std::string output = (path + ".ic10").c_str();
	generated_file.open(output);
	generated_file << code;
	printf("Wrote to %s\n", output.c_str());
	generated_file.close();
	printf("Compiling took %fms.\n", total_timer.time() * 1000.0);
}

void Compiler::error(int line, const std::string& message)
{
	this->had_error = true;
	this->report(line, "", message);
}

void Compiler::report(int line, const std::string& where, const std::string& message)
{
	printf("ERROR: on line %i: %s %s\n", line, where.c_str(), message.c_str());
}

void Compiler::info(const std::string& message)
{
	if (this->level >= ReportingLevel::All)
	{
		printf("    (i): %s\n", message.c_str());
	}
}

void Compiler::warn(int line, const std::string& warning)
{
	if (this->level >= ReportingLevel::Warnings)
	{
		printf("  WARN: on line %i: %s\n", line, warning.c_str());
	}
}
