#include "CIRProgram.h"

CIRProgram::CIRProgram(std::vector<cir::Instruction::ref> instructions, TypeCheckedProgram program)
	:instructions(std::move(instructions)), program(std::move(program))
{}