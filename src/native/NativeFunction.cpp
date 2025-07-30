#include "NativeFunction.h"

NativeFunction::NativeFunction(const std::string& name, TypeName return_type, std::vector<Stmt::Function::Param> params)
	:definition(Token(0, TokenType::IDENTIFIER, name, name.c_str()), return_type, std::move(params), std::move(std::vector<std::unique_ptr<Stmt>>{}), FunctionSource::Builtin)
{}

NativeFunction::reference_type NativeFunction::make_reference(const std::string& name, TypeName return_type, std::vector<Stmt::Function::Param> params)
{
	return std::make_shared<NativeFunction>(name, return_type, params);
}

std::shared_ptr<NativeFunction> NativeFunction::refit()
{
	return reference_type(this);
}

NativeFunction& NativeFunction::add_asm(const std::string& src)
{
	this->definition.body.push_back(std::move(std::make_unique<Stmt::Asm>(std::make_shared<Expr::Literal>(NativeFunction::token_literal_string(src)), NativeFunction::token_fake())));
	return *this;
}

NativeFunction& NativeFunction::add_asm(const std::vector<std::string>& src)
{
	std::string built;
	for (const auto& str : src)
	{
		built += str;
	}
	return this->add_asm(built);
}

NativeFunction& NativeFunction::add_return(const std::string& value)
{
	this->definition.body.push_back(std::move(std::make_unique<Stmt::Return>(NativeFunction::token_fake(), std::make_shared<Expr::Variable>(NativeFunction::token_literal_string(value)))));
	return *this;
}

std::unique_ptr<Stmt> NativeFunction::splice()
{
	return this->definition.clone();
}

Token NativeFunction::token_literal_string(const std::string& val)
{
	return Token(0, TokenType::STRING, val, val);
}

Token NativeFunction::token_fake()
{
	return Token(0, TokenType::FUNCTION, "@builtin");
}