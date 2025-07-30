#pragma once

#include "../TypeChecker.h"
#include "CIRInstruction.h"

class CIRProgram
{
public:
	CIRProgram(std::vector<cir::Instruction::ref> instructions, TypeCheckedProgram program);
	std::vector<cir::Instruction::ref> instructions;
	TypeCheckedProgram program;
private:
};

