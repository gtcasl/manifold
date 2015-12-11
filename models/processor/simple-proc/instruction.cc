#ifndef  INTRUCTION_CC_INC
#define  INTRUCTION_CC_INC

#include	"instruction.h"

namespace manifold {
namespace simple_proc {

Instruction::Instruction( opcode_t op, paddr_t addr)
{
    this->opcode = op;
    this->addr = addr;
}

Instruction::~Instruction()
{
}

bool Instruction::is_memop(void)
{
    return ((opcode & OpMask) == OpMem );
}

}//namespace simple_proc
}//namespace manifold

#endif   /* ----- #ifndef INTRUCTION_CC_INC  ----- */
