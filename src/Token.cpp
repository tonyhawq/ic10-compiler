#include "Token.h"

Literal::Literal()
	:number(nullptr), string(nullptr), boolean(nullptr)
{}

Literal::Literal(const char* string)
	:Literal()
{
	this->string = new std::string(string);
}

Literal::Literal(double number)
	:Literal()
{
	this->number = new double(number);
}

Literal::Literal(bool boolean)
	:Literal()
{
	this->boolean = new bool(boolean);
}

Literal::~Literal()
{
	if (this->number)
	{
		delete this->number;
	}
	if (this->string)
	{
		delete this->string;
	}
	if (this->boolean)
	{
		delete this->boolean;
	}
}

Literal::Literal(const Literal& other)
	:number(nullptr), string(nullptr), boolean(nullptr)
{
	if (other.number)
	{
		this->number = new double(*other.number);
	}
	if (other.string)
	{
		this->string = new std::string(*other.string);
	}
	if (other.boolean)
	{
		this->boolean = new bool(*other.boolean);
	}
}

Literal::Literal(Literal&& other) noexcept
	:number(other.number), string(other.string), boolean(other.boolean)
{
	other.number = nullptr;
	other.string = nullptr;
	other.boolean = nullptr;
}

std::string Literal::display()
{
	if (this->string)
	{
		return std::string(*this->string);
	}
	if (this->number)
	{
		return std::to_string(*this->number);
	}
	if (this->boolean)
	{
		return std::to_string(*this->boolean);
	}
	return std::string("NULL");
}

Token::Token(int line, TokenType type, const std::string& lexeme)
	:line(line), type(type), lexeme(lexeme), literal()
{}
Token::Token(int line, TokenType type, const std::string& lexeme, double literal)
	:line(line), type(type), lexeme(lexeme), literal()
{
	this->literal.number = new double(literal);
}

Token::Token(int line, TokenType type, const std::string& lexeme, bool literal)
	:line(line), type(type), lexeme(lexeme), literal()
{
	this->literal.boolean = new bool(literal);
}

Token::Token(int line, TokenType type, const std::string& lexeme, const char* literal)
	:line(line), type(type), lexeme(lexeme), literal()
{
	if (!literal)
	{
		return;
	}
	this->literal.string = new std::string(literal);
}

Token::Token(const Token& other)
	:line(other.line), literal(other.literal), type(other.type), lexeme(other.lexeme)
{}

Token::Token(Token&& other) noexcept
	:line(other.line), literal(other.literal), type(other.type), lexeme(other.lexeme)
{}

Token::~Token()
{}

std::string Token::to_string() const
{
	std::string val = this->lexeme + " Line " + std::to_string(this->line) + " type " + std::to_string(static_cast<int>(this->type));
	if (this->literal.number)
	{
		val += " Literal: ";
		val += std::to_string(*this->literal.number);
	}
	if (this->literal.string)
	{
		val += " Literal: ";
		val += *this->literal.string;
	}
	return val;
}
