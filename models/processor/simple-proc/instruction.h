/*
 * =====================================================================================
 *
 *       Filename:  intruction.h
 *
 *    Description:  Contains the list of supported instruction types
 *
 *        Version:  1.0
 *        Created:  06/26/2011 09:39:49 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Mitchelle Rasquinha 
 *        Company: Georgia Institute of Technology 
 *
 * =====================================================================================
 */
#ifndef  INTRUCTION_H_INC
#define  INTRUCTION_H_INC

#include <stdint.h>

namespace manifold {
namespace simple_proc {

typedef uint64_t paddr_t;

class Instruction {
public:
    typedef enum {
	// Main instruction opcode
	OpMask      = 0xF00,        // Mask for main insn class
	OpMem       = 0x400,
	OpNop       = 0x000,

	// Memory subtypes
	OpMemLd     = 0x00 + OpMem,
	OpMemSt     = 0x10 + OpMem
    } opcode_t;

    opcode_t opcode;
    paddr_t  addr;

public:
    Instruction (opcode_t opcode = OpNop, paddr_t addr = 0x0);
    ~Instruction (void);

    bool is_memop (void);
};

}//namespace simple_proc
}//namespace manifold


#endif   /* ----- #ifndef INTRUCTION_H_INC  ----- */
