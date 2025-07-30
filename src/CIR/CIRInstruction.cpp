#include "CIRInstruction.h"

cir::Instruction::ref cir::Instruction::from(Instruction* instruction)
{
	return cir::Instruction::ref(instruction);
}