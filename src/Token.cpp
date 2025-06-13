#include "Token.h"

Literal::Literal()
	:number(nullptr), string(nullptr), boolean(nullptr), string_hashed(false)
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
	:number(nullptr), string(nullptr), boolean(nullptr), string_hashed(other.string_hashed)
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
	:number(other.number), string(other.string), boolean(other.boolean), string_hashed(other.string_hashed)
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

bool Literal::is_number() const
{
	return this->number;
}

bool Literal::is_string() const
{
	return this->string && !this->string_hashed;
}

bool Literal::is_hashstring() const
{
	return this->string && this->string_hashed;
}

bool Literal::is_boolean() const
{
	return this->boolean;
}

bool Literal::is_integral() const
{
	if (!this->is_number())
	{
		return false;
	}
	return static_cast<double>(static_cast<int>(this->as_number())) == this->as_number();
}

double Literal::as_number() const
{
	if (!this->is_number())
	{
		throw std::runtime_error("Attempted to get number literal while literal is not of type number.");
	}
	return *this->number;
}

int Literal::as_integer() const
{
	if (!this->is_integral())
	{
		throw std::runtime_error("Attempted to convert number literal to integer while literal is not integral (possibly not a number)");
	}
	return static_cast<int>(*this->number);
}

const std::string& Literal::as_string() const
{
	if (!this->is_string())
	{
		throw std::runtime_error("Attempted to get string literal while literal is not of type string.");
	}
	return *this->string;
}

const std::string& Literal::as_hash_string() const
{
	if (!this->is_hashstring())
	{
		throw std::runtime_error("Attempted to get hash string literal while literal is not of type hash string.");
	}
	return *this->string;
}

bool Literal::as_boolean() const
{
	if (!this->is_boolean())
	{
		throw std::runtime_error("Attempted to get boolean literal while literal is not of type boolean.");
	}
	return *this->boolean;
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
