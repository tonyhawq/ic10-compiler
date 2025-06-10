#pragma once

#include <vector>
#include <stdexcept>
#include <unordered_map>

#include "Token.h"
#include "AST.h"
#include "Errors.h"

class Compiler;

struct SimpleVariableInfo;

template<typename t_varinfo>
class Environment
{
public:
	virtual ~Environment() {};

	static Environment make_orphaned();
	void spawn();
	void spawn_inside_function(const std::string& function);
	void pop();
	void pop_to_function();
	void kill();
	bool define(const std::string& name, t_varinfo var_info);
	bool inside_function();
	std::string* function_name();

	t_varinfo* resolve(const std::string& name);
protected:
	Environment(Environment* parent);
	std::shared_ptr<std::string> function;
	Environment* parent;
	Environment* child;
	std::unordered_map<std::string, t_varinfo> variables;
};

struct SimpleVariableInfo
{
	TypeName type;
	std::string name;
};

class Parser
{
public:
	Parser(Compiler& compiler, std::vector<Token> tokens);
	std::vector<std::unique_ptr<Stmt>> parse();
	void error(const Token& error_token, const std::string& message);

	Environment<SimpleVariableInfo> environment;
private:
	bool match(const std::vector<TokenType>& types);
	const TokenType* matching_type(const std::vector<TokenType>& types);
	bool current_token_is(TokenType type);
	const Token& advance();
	bool is_eof();
	const Token& peek();
	const Token& peek_ahead(size_t amount);
	const Token& peek_previous();
	const Token& consume(TokenType type, const std::string& error_message);
	void synchronize();
	bool peek_var_decl();

	TypeName parse_type();
	std::unique_ptr<Stmt> parse_statement();
	std::shared_ptr<Expr> parse_expression();
	std::shared_ptr<Expr> parse_assignment();
	std::shared_ptr<Expr> parse_or();
	std::shared_ptr<Expr> parse_and();
	std::shared_ptr<Expr> parse_equality();
	std::shared_ptr<Expr> parse_comparison();
	std::shared_ptr<Expr> parse_term();
	std::shared_ptr<Expr> parse_factor();
	std::shared_ptr<Expr> parse_unary();
	std::shared_ptr<Expr> parse_call();
	std::shared_ptr<Expr> parse_primary();

	std::shared_ptr<Expr> finish_parse_call(std::shared_ptr<Expr> expression);

	std::unique_ptr<Stmt> parse_if();
	std::unique_ptr<Stmt> parse_block();
	std::unique_ptr<Stmt> parse_variable_declaration();
	std::unique_ptr<Stmt> parse_function_declaration();
	std::unique_ptr<Stmt> parse_symbols();
	std::unique_ptr<Stmt> parse_declaration();
	std::unique_ptr<Stmt> parse_asm_statement();
	std::unique_ptr<Stmt> parse_print_statement();
	std::unique_ptr<Stmt> parse_while_statement();
	std::unique_ptr<Stmt> parse_for_statement();
	std::unique_ptr<Stmt> parse_return_statement();
	std::unique_ptr<Stmt> parse_expression_statement();

	Compiler& compiler;
	size_t current_token;
	std::vector<Token> tokens;
};

template<typename t_varinfo>
Environment<t_varinfo> Environment<t_varinfo>::make_orphaned()
{
	return Environment(nullptr);
}

template<typename t_varinfo>
Environment<t_varinfo>::Environment(Environment* parent)
	:parent(parent), child(nullptr)
{}

template<typename t_varinfo>
t_varinfo* Environment<t_varinfo>::resolve(const std::string& name)
{
	if (this->variables.count(name))
	{
		return &this->variables.at(name);
	}
	if (this->parent)
	{
		return this->parent->resolve(name);
	}
	return nullptr;
}

template<typename t_varinfo>
bool Environment<t_varinfo>::define(const std::string& name, t_varinfo val)
{
	if (this->variables.count(name))
	{
		return false;
	}
	this->variables.emplace(name, std::move(val));
	return true;
}

template<typename t_varinfo>
void Environment<t_varinfo>::spawn()
{
	if (this->child)
	{
		throw std::runtime_error("Attempted to create child of Environment which already has a child.");
	}
	this->child = new Environment(this);
	this->child->function = this->function;
	this = this->child;
}

template<typename t_varinfo>
void Environment<t_varinfo>::spawn_inside_function(const std::string& function)
{
	this->spawn();
	this->function = std::make_shared<std::string>(function);
}

template<typename t_varinfo>
bool Environment<t_varinfo>::inside_function()
{
	return this->function.get();
}

template<typename t_varinfo>
std::string* Environment<t_varinfo>::function_name()
{
	return this->function.get();
}

template<typename t_varinfo>
void Environment<t_varinfo>::kill()
{
	if (this->parent)
	{
		this->parent->child = nullptr;
	}
	delete this;
}

template<typename t_varinfo>
void Environment<t_varinfo>::pop()
{
	Environment* parent = this->parent;
	this->kill();
	*this = std::move(parent);
	delete parent;
}

template<typename t_varinfo>
void Environment<t_varinfo>::pop_to_function()
{
	if (!this->parent)
	{
		return this;
	}
	if (this->function)
	{
		return this;
	}
	Environment<t_varinfo>* env = this->parent->pop_to_function();
	if (env.,
}
