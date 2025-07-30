#pragma once

#include "../AST.h"

class Builtin
{
public:
	typedef std::shared_ptr<Builtin> reference_type;

	static reference_type make_reference(const std::string& name, TypeName return_type, std::vector<Stmt::Function::Param> params);
	Builtin(const std::string& name, TypeName return_type, std::vector<Stmt::Function::Param> params);
	Builtin& add_asm(const std::string& src);
	Builtin& add_asm(const std::vector<std::string>& src);
	Builtin& add_return(const std::string& value);

	std::unique_ptr<Stmt> splice();
	std::shared_ptr<Builtin> refit();

	static Token token_literal_string(const std::string& val);
	static Token token_fake();
private:
	Stmt::Function definition;
};
