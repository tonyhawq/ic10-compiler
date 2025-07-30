#include "Builtin.h"

Builtin::Builtin(const std::string& name, TypeName return_type, std::vector<Stmt::Function::Param> params)
	:definition(Token(0, TokenType::IDENTIFIER, name, name.c_str()), return_type, std::move(params), std::move(std::vector<std::unique_ptr<Stmt>>{}), FunctionSource::Builtin)
{}

Builtin::reference_type Builtin::make_reference(const std::string& name, TypeName return_type, std::vector<Stmt::Function::Param> params)
{
	return std::make_shared<Builtin>(name, return_type, params);
}

std::shared_ptr<Builtin> Builtin::refit()
{
	return reference_type(this);
}

Builtin& Builtin::add_asm(const std::string& src)
{
	this->definition.body.push_back(std::move(std::make_unique<Stmt::Asm>(std::make_shared<Expr::Literal>(Builtin::token_literal_string(src)), Builtin::token_fake())));
	return *this;
}

Builtin& Builtin::add_asm(const std::vector<std::string>& src)
{
	std::string built;
	for (const auto& str : src)
	{
		built += str;
	}
	return this->add_asm(built);
}

Builtin& Builtin::add_return(const std::string& value)
{
	this->definition.body.push_back(std::move(std::make_unique<Stmt::Return>(Builtin::token_fake(), std::make_shared<Expr::Variable>(Builtin::token_literal_string(value)))));
	return *this;
}

std::unique_ptr<Stmt> Builtin::splice()
{
	return this->definition.clone();
}

Token Builtin::token_literal_string(const std::string& val)
{
	return Token(0, TokenType::STRING, val, val);
}

Token Builtin::token_fake()
{
	return Token(0, TokenType::FUNCTION, "@builtin");
}