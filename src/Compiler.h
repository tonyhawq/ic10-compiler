#pragma once

#include <string>
#include <fstream>
#include <sstream>

#include "Scanner.h"
#include "Parser.h"
#include "Optimizer.h"
#include "native/NativeFunction.h"

class Compiler : public Expr::Visitor, public Stmt::Visitor
{
public:
	void compile(const std::string& path);
	void info(const std::string& message);
	void warn(int line, const std::string& warning);
	void error(int line, const std::string& message);
	void report(int line, const std::string& where, const std::string& message);
private:
	static const std::unordered_map<std::string, NativeFunction::reference_type>& native_functions();
	enum class ReportingLevel
	{
		Errors,
		Warnings,
		All
	};
	ReportingLevel level = ReportingLevel::All;
	bool had_error = false;
};
