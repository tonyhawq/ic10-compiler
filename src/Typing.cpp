#include "Typing.h"

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
{
}

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

#define CHECK_IS_FUNCTION_OR_ERROR(reason) do {if (!this->is_function) {throw std::logic_error(std::string("TYPECHECK ERROR: !ATTEMPTED TO GET ") + std::string(reason) + " OF NON FUNCTION TYPE " + this->type.type_name());}} while(0)

TypeName& TypeID::return_type()
{
	CHECK_IS_FUNCTION_OR_ERROR("RETURN TYPE");
	return this->function_type->return_type;
}

std::string& TypeID::unmangled_name()
{
	CHECK_IS_FUNCTION_OR_ERROR("UNMANGLED_NAME");
	return this->function_type->unmangled_name;
}

std::string& TypeID::mangled_name()
{
	CHECK_IS_FUNCTION_OR_ERROR("MANGLED_NAME");
	return this->function_type->mangled_name;
}

std::vector<FunctionParam>& TypeID::arguments()
{
	CHECK_IS_FUNCTION_OR_ERROR("ARGUMENTS");
	return this->function_type->arguments;
}

const TypeName& TypeID::return_type() const
{
	CHECK_IS_FUNCTION_OR_ERROR("RETURN TYPE");
	return this->function_type->return_type;
}

const std::string& TypeID::unmangled_name() const
{
	CHECK_IS_FUNCTION_OR_ERROR("UNMANGLED_NAME");
	return this->function_type->unmangled_name;
}

const std::string& TypeID::mangled_name() const
{
	CHECK_IS_FUNCTION_OR_ERROR("MANGLED_NAME");
	return this->function_type->mangled_name;
}

const std::vector<FunctionParam>& TypeID::arguments() const
{
	CHECK_IS_FUNCTION_OR_ERROR("ARGUMENTS");
	return this->function_type->arguments;
}

TypeID::TypeID(const TypeName& type)
	:type(type), is_function(false), function_type(nullptr)
{
}

TypeID::TypeID(const TypeName& type, const TypeName& return_type, const std::string& m_unmangled_name, const std::vector<FunctionParam>& m_arguments)
	:type(type), is_function(true), function_type(std::make_unique<FunctionTypeID>(return_type, "", m_unmangled_name, m_arguments))
{
	this->function_type->mangled_name = types::get_mangled_function_name(this->unmangled_name(), this->arguments(), this->return_type());
}

FunctionTypeID::FunctionTypeID(const TypeName& return_type, const std::string& mangled_name, const std::string& unmangled_name, const std::vector<FunctionParam> arguments)
	:return_type(return_type), mangled_name(mangled_name), unmangled_name(unmangled_name), arguments(arguments)
{
}

TypeID::TypeID(const TypeID& other)
	:type(other.type), is_function(other.is_function),
	function_type(other.function_type ? std::make_unique<FunctionTypeID>(*other.function_type) : nullptr)
{
}

namespace types
{
	std::string get_mangled_function_name(const std::string& name, const std::vector<FunctionParam> params, const TypeName& return_type)
	{
		std::string type_name = std::string("@function_") + name + "_return_" + return_type.type_name() + "_params_";
		for (int i = 0; i < params.size(); i++)
		{
			const FunctionParam& param = params[i];
			type_name += '@';
			type_name += param.type.type_name();
			type_name += '_';
			type_name += param.name;
		}
		return type_name;
	}

	TypeName get_function_signature(const std::vector<FunctionParam>& params, const TypeName& return_type)
	{
		std::string signature = "@fun(";
		for (size_t i = 0; i < params.size(); i++)
		{
			signature += params[i].type.type_name();
			if (i < params.size() - 1)
			{
				signature += ",";
			}
		}
		signature += ") -> ";
		signature += return_type.type_name();
		return TypeName(std::move(signature));
	}
}