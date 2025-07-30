#include "AST.h"

void TypeName::make_pointer_type(bool constant, bool nullable)
{
	TypeName pointer_type;
	pointer_type.constant = constant;
	pointer_type.pointer = true;
	pointer_type.m_nullable = std::make_unique<bool>(nullable);
	pointer_type.m_pointed_to_type = std::make_unique<TypeName>(std::move(*this));
	*this = std::move(pointer_type);
}
TypeName::TypeName(const TypeName& other)
	:constant(other.constant), pointer(other.pointer), m_type_name(other.m_type_name ? std::make_unique<std::string>(*other.m_type_name) : nullptr),
	m_pointed_to_type(other.m_pointed_to_type ? std::make_unique<TypeName>(*other.m_pointed_to_type) : nullptr),
	m_nullable(other.m_nullable ? std::make_unique<bool>(*other.m_nullable) : nullptr),
	compile_time(other.compile_time)
{}

TypeName TypeName::create_compile_time(const std::string& type_name)
{
	TypeName type(type_name);
	type.compile_time = true;
	return type;
}

TypeName& TypeName::operator=(const TypeName& other)
{
	if (this != &other)
	{
		constant = other.constant;
		pointer = other.pointer;
		m_type_name = other.m_type_name ? std::make_unique<std::string>(*other.m_type_name) : nullptr;
		m_pointed_to_type = other.m_pointed_to_type ? std::make_unique<TypeName>(*other.m_pointed_to_type) : nullptr;
		m_nullable = other.m_nullable ? std::make_unique<bool>(*other.m_nullable) : nullptr;
	}
	return *this;
}

TypeName TypeName::underlying() const
{
	if (this->pointer)
	{
		return this->m_pointed_to_type->underlying();
	}
	return TypeName(*this->m_type_name);
}

bool TypeName::nullable() const
{
	if (this->pointer)
	{
		return *this->m_nullable;
	}
	return false;
}

std::string TypeName::type_name() const
{
	if (this->pointer)
	{
		std::string dtype_name = this->m_pointed_to_type->type_name();
		if (this->constant)
		{
			dtype_name += " const";
		}
		dtype_name += " *";
		if (this->nullable())
		{
			dtype_name += "?";
		}
		return dtype_name;
	}
	return (this->compile_time ? std::string("fixed ") : std::string("")) + (this->constant ? std::string("const ") : std::string("")) + *this->m_type_name;
}

int TypeName::height() const
{
	if (this->pointer)
	{
		return this->m_pointed_to_type->height() + 1;
	}
	return 0;
}

bool TypeName::const_unqualified_equals(const TypeName& other) const
{
	if (this->pointer && other.pointer)
	{
		return (*this->m_pointed_to_type) == (*other.m_pointed_to_type);
	}
	if (!this->pointer && !other.pointer)
	{
		return *this->m_type_name == *other.m_type_name;
	}
	return false;
}

bool TypeName::operator==(const TypeName& other) const
{
	if (this == &other)
	{
		return true;
	}
	if (this->constant != other.constant)
	{
		return false;
	}
	if (this->compile_time != other.compile_time)
	{
		return false;
	}
	if (this->pointer && other.pointer)
	{
		return (*this->m_pointed_to_type) == (*other.m_pointed_to_type);
	}
	if (!this->pointer && !other.pointer)
	{
		return (*this->m_type_name) == (*other.m_type_name);
	}
	return false;
}

bool TypeName::operator!=(const TypeName& other) const
{
	return !(*this == other);
}

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