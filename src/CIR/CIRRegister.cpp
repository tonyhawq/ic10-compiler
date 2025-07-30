#include "CIRRegister.h"

cir::Temporary::Temporary(size_t i)
	:i(i)
{}

std::string cir::Temporary::to_string() const
{
	return std::string("t") + std::to_string(this->i);
}

size_t cir::Temporary::num() const
{
	return this->i;
}

cir::Number::Number(double val)
	:val(val)
{}

std::string cir::Number::to_string() const
{
	return std::to_string(this->val);
}

double cir::Number::value() const
{
	return this->val;
}