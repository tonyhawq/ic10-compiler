#pragma once

#include <string>
#include <fstream>
#include <sstream>

#include "Scanner.h"
#include "Parser.h"
#include "Optimizer.h"

class Compiler : public Expr::Visitor, public Stmt::Visitor
{
public:
	void compile(const std::string& path);
	void error(int line, std::string message);
	void report(int line, std::string where, std::string message);
private:
	bool had_error = false;
};
