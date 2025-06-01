#pragma once

#include <vector>
#include <stdexcept>
#include <unordered_map>

#include "Token.h"
#include "AST.h"
#include "Errors.h"

class Compiler;

class Environment
{
public:
	struct VariableInfo;

	static Environment make_orphaned();
	Environment& spawn();
	Environment& spawn_inside_function(const std::string& function);
	Environment* pop();
	void kill();
	bool define(const TypeName& type, const std::string& name);
	bool inside_function();
	std::string* function_name();

	VariableInfo* resolve(const std::string& name);
private:
	Environment(Environment* parent);
	std::shared_ptr<std::string> function;
	Environment* parent;
	Environment* child;
	std::unordered_map<std::string, VariableInfo> variables;
};

struct Environment::VariableInfo
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

	Environment environment;
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

