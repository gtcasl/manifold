#ifdef USE_QSIM

#ifndef __SPX_INSTRUCTION_H__
#define __SPX_INSTRUCTION_H__

#include <stdint.h>
#include <map>
#include <vector>
#include <cstddef>

namespace manifold {
namespace spx {

// forward declaration
class spx_core_t;
class pipeline_t;
class pipeline_config_t;
class pipeline_stats_t;

enum SPX_MEM_TYPE
{
    SPX_MEM_LD,
    SPX_MEM_ST,
    SPX_MEM_NONE,  
    SPX_NUM_MEM_TYPES
};

enum SPX_MEM_DISAMB_TYPE
{
    SPX_MEM_DISAMB_NONE,
    SPX_MEM_DISAMB_DETECTED,
    SPX_MEM_DISAMB_CLEAR,
    SPX_MEM_DISAMB_WAIT
};

class inst_t
{
public:
    inst_t(spx_core_t *spx_core, uint64_t Mop_seq, uint64_t uop_seq);
    inst_t(inst_t *inst, spx_core_t *spx_core, uint64_t Mop_seq, uint64_t uop_seq, int mem_code);
    ~inst_t();

    // IDs
    spx_core_t *core;
    uint64_t Mop_sequence;
    uint64_t uop_sequence;
  
    // inst addresses
    struct {
        uint64_t vaddr;
        uint64_t paddr;    
    }op;

    // data addresses
    struct {
        uint64_t vaddr;
        uint64_t paddr;    
    }data;
  
    uint64_t memory_request_time_stamp;

    // inst split after Qsim_cb
    bool split_store;
    bool split_load;

    // pipeline control
    int opcode; // instruction type
    int memcode; // memory instruction type
    int excode; // FU type (e.g., SPX_FU_INT)
    int port; // EX port

    bool is_long_inst;
    bool inflight;
    bool is_head;
    bool is_tail;
    bool is_idle;
    bool squashed;
    uint64_t squashed_cycle;
    uint64_t completed_cycle;

    // inst dependency
    uint64_t src_reg;
    uint8_t src_flag;
    uint16_t src_fpreg;

    uint64_t dest_reg;
    uint8_t dest_flag;
    uint16_t dest_fpreg;

    std::map<uint64_t,inst_t*> dest_dep; // dest dep; insts that depend on this inst
    std::map<uint64_t,inst_t*> src_dep; // src dep; insts that this inst depends on

    // memory order control
    std::vector<inst_t*> mem_disamb;
    int mem_disamb_status;  // wait until mem_disamb is cleared

    // Mop/uop information
    inst_t *prev_inst;  
    inst_t *next_inst;
    inst_t *Mop_head;
    int Mop_length;
};

class cache_request_t
{
public:
    cache_request_t() {}
    cache_request_t(inst_t *instruction, int rid, int sid, uint64_t paddr, int type);
    ~cache_request_t();
  
    unsigned long get_addr() { return addr; }
    bool is_read() { return op_type == SPX_MEM_LD; }

    inst_t *inst;
    uint64_t inst_id;
    int req_id;
    int source_id;
    uint64_t addr;
    int op_type;
};

} // namespace spx
} // namespace manifold

#endif

#endif // USE_QSIM
