#ifdef USE_QSIM

#include <string>
#include "qsim.h"
#include "instruction.h"

using namespace std;
using namespace manifold;
using namespace manifold::spx;

inst_t::inst_t(spx_core_t *spx_core, uint64_t Mop_seq, uint64_t uop_seq) :
core(spx_core),
Mop_sequence(Mop_seq),
uop_sequence(uop_seq),
memory_request_time_stamp(0),
split_store(false),
split_load(false),
opcode(QSIM_INST_INTBASIC),
memcode(SPX_MEM_NONE),
excode(-1),
port(-1),
is_long_inst(false),
inflight(false),
is_head(true),
is_tail(true),
is_idle(false),
squashed(false),
squashed_cycle(0),
completed_cycle(0),
src_reg(0),
src_flag(0),
src_fpreg(0),
dest_reg(0),
dest_flag(0),
dest_fpreg(0),
mem_disamb_status(SPX_MEM_DISAMB_NONE),
prev_inst(NULL),
next_inst(NULL),
Mop_head(this),
Mop_length(1)
{
    op.vaddr = 0;
    op.paddr = 0;
    data.vaddr = 0;
    data.paddr = 0;
}

inst_t::inst_t(inst_t *inst, spx_core_t *spx_core, uint64_t Mop_seq, uint64_t uop_seq, int mem_code) :
core(spx_core),
Mop_sequence(Mop_seq),
uop_sequence(uop_seq),
memory_request_time_stamp(0),
split_store(false),
split_load(false),
opcode(inst->opcode),
memcode(mem_code),
excode(-1),
port(-1),
is_long_inst(false),
inflight(false),
is_head(false),
is_tail(true),
is_idle(false),
squashed(false),
squashed_cycle(0),
completed_cycle(0),
src_reg(0),
src_flag(0),
src_fpreg(0),
dest_reg(0),
dest_flag(0),
dest_fpreg(0),
mem_disamb_status(SPX_MEM_DISAMB_NONE),
prev_inst(inst),
next_inst(NULL),
Mop_head(inst->Mop_head),
Mop_length(0)
{
    op.vaddr = inst->op.vaddr;
    op.paddr = inst->op.paddr;
    data.vaddr = 0;
    data.paddr = 0;

    if(inst->memcode == SPX_MEM_LD) {
        inst->dest_dep.insert(pair<uint64_t,inst_t*>(uop_sequence,this));
        src_dep.insert(pair<uint64_t,inst_t*>(inst->uop_sequence,inst));
    }
  
    inst->next_inst = this;
    inst->is_tail = false;
    Mop_head->Mop_length++;
}

inst_t::~inst_t()
{
    dest_dep.clear();
    src_dep.clear();
    mem_disamb.clear();
}

cache_request_t::cache_request_t(inst_t *instruction, int rid, int sid, uint64_t paddr, int type) :
inst(instruction),
inst_id(instruction->uop_sequence),
req_id(rid),
source_id(sid),
addr(paddr),
op_type(type)
{
}

cache_request_t::~cache_request_t()
{
}

#endif // USE_QSIM
