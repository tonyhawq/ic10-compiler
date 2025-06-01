#include "AST.h"

void TypeName::make_pointer_type(bool constant)
{
	TypeName pointer_type;
	pointer_type.constant = constant;
	pointer_type.pointer = true;
	pointer_type.m_pointed_to_type = std::make_unique<TypeName>(std::move(*this));
	*this = std::move(pointer_type);
}
TypeName::TypeName(const TypeName& other)
	:constant(other.constant), pointer(other.pointer), m_type_name(other.m_type_name ? std::make_unique<std::string>(*other.m_type_name) : nullptr),
	m_pointed_to_type(other.m_pointed_to_type ? std::make_unique<TypeName>(*other.m_pointed_to_type) : nullptr)
{}

TypeName& TypeName::operator=(const TypeName& other)
{
	if (this != &other)
	{
		constant = other.constant;
		pointer = other.pointer;
		m_type_name = other.m_type_name ? std::make_unique<std::string>(*other.m_type_name) : nullptr;
		m_pointed_to_type = other.m_pointed_to_type ? std::make_unique<TypeName>(*other.m_pointed_to_type) : nullptr;
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

std::string TypeName::type_name() const
{
	if (this->pointer)
	{
		std::string dtype_name = this->m_pointed_to_type->type_name();
		if (this->constant)
		{
			dtype_name += " const";
		}
		return dtype_name + " *";
	}
	return std::string(*this->m_type_name) + (this->constant ? " const" : "");
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
	if (this->constant != other.constant)
	{
		return false;
	}
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

bool TypeName::operator!=(const TypeName& other) const
{
	return !(*this == other);
}
