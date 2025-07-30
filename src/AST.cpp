#include "AST.h"

std::vector<std::shared_ptr<Expr>> Expr::clone_vec(const std::vector<std::shared_ptr<Expr>>& exprs)
{
	std::vector<std::shared_ptr<Expr>> cloned;
	cloned.reserve(exprs.size());
	for (const auto& val : exprs)
	{
		cloned.push_back(val->clone());
	}
	return cloned;
}

std::vector<std::unique_ptr<Stmt>> Stmt::clone_vec(const std::vector<std::unique_ptr<Stmt>>& exprs)
{
	std::vector<std::unique_ptr<Stmt>> cloned;
	cloned.reserve(exprs.size());
	for (const auto& val : exprs)
	{
		cloned.push_back(val->clone());
	}
	return cloned;
}