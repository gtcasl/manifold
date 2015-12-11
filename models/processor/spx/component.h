#ifdef USE_QSIM

#ifndef __SPX_COMPONENT_H__
#define __SPX_COMPONENT_H__

#include <vector>
#include <map>
#include <inttypes.h>
#include "instruction.h"
#include "qsim-regs.h"

namespace manifold {
namespace spx {

enum SPX_FU 
{ 
    SPX_FU_INT, SPX_FU_MUL, SPX_FU_FP, 
    SPX_FU_MOV, SPX_FU_BR, SPX_FU_LD, 
    SPX_FU_ST, SPX_NUM_FU_TYPES 
};
  
enum SPX_FP_REGS
{
    SPX_FP_REG_ST0 = 0, SPX_FP_REG_ST1,
    SPX_FP_REG_ST2, SPX_FP_REG_ST3,
    SPX_FP_REG_ST4, SPX_FP_REG_ST5,
    SPX_FP_REG_ST6, SPX_FP_REG_ST7,
    SPX_NUM_FP_REGS
};

class instQ_t
{
public:
    instQ_t(pipeline_t *pl, int instQ_size, int cache_line_width);
    ~instQ_t();

    void push_back(inst_t *inst);
    inst_t* get_front();
    void pop_front();
    bool is_available();

private:
    std::vector<inst_t*> queue;
    int size;
    int occupancy;
    int fetch_mask;
    uint64_t fetch_addr;
    pipeline_t *pipeline;
};

class ROB_t
{
public:
    ROB_t(pipeline_t *pl, int ROB_size);
    ~ROB_t();

    void push_back(inst_t *inst);
    inst_t* get_front();
    inst_t* get_head();
    void pop_front();
    bool is_available();
    void update(inst_t *inst);

private:
    std::vector<inst_t*> queue;
    int size;
    //int occupancy;
    pipeline_t *pipeline;
};

class RS_t
{
public:
    RS_t(pipeline_t *pl, int RS_size, int exec_port, std::vector<int> *FU_port_binding);
    ~RS_t();

    void push_back(inst_t *inst);
    inst_t* get_front(int port);
    void pop_front(int port);
    bool is_available();
    void update(inst_t *inst);
    void port_binding(inst_t *inst);

private:
    std::vector<inst_t*> *ready; // ready queues to each exec port

    int size;
    int occupancy;
    int port; // total number of ports
    int *port_loads; // array of inst counts assigned to each port
    uint16_t FU_port[SPX_NUM_FU_TYPES]; // array of possible ports to each FU type
    pipeline_t *pipeline;
};

#define SPX_N_FLAGS 6
#define SPX_N_FPREGS 8
#define IS_IREG(i) ((i<QSIM_FP0)||(i>QSIM_FPSP))

class RF_t
{
public:
    RF_t(pipeline_t *pl);
    ~RF_t();

    void resolve_dependency(inst_t *inst);
    void writeback(inst_t *inst);

private:
    inst_t *regs[QSIM_N_REGS]; // regs table of latest producers of regs
    inst_t *flags[SPX_N_FLAGS]; // flag table of latest producers of flags
    inst_t *fpregs[SPX_N_FPREGS]; // fpregs table of latest producers of fpregs
    int fpregs_stack_ptr;
    pipeline_t *pipeline;
};

class FU_t
{
public:
    FU_t(pipeline_t *pl, int FU_delay, int FU_issue_rate);
    ~FU_t();

    void push_back(inst_t *inst);
    inst_t* get_front();
    void pop_front();
    bool is_available();
    void stall();

    int delay;
    int issue_rate;
    
private:
    std::vector<inst_t*> queue;
    pipeline_t *pipeline;
};

class EX_t
{
public:
    EX_t(pipeline_t *pl, int FU_port, std::vector<int> *FU_port_binding, int *FU_delay, int *FU_issue_rate);
    ~EX_t();

    void push_back(inst_t *inst);
    inst_t* get_front();
    void pop_front(int port);
    bool is_available(int port);

private:
    FU_t *FUs[SPX_NUM_FU_TYPES];
    int FU_index[SPX_NUM_FU_TYPES];
    int num_FUs;
    pipeline_t *pipeline;
};

class LDQ_t
{
public:
    LDQ_t(pipeline_t *pl, int LDQ_size);
    ~LDQ_t();

    void push_back(inst_t *inst);
    void pop(inst_t *inst);
    void schedule(inst_t *inst);
    void handle_cache();
    bool is_available();
    inst_t* handle_deadlock();

private:
    std::vector<inst_t*> outgoing;
    std::map<uint64_t,inst_t*> outstanding;
    int size;
    int occupancy;
    pipeline_t *pipeline;
};

class STQ_t
{
public:
    STQ_t(pipeline_t *pl, int STQ_size);
    ~STQ_t();

    void push_back(inst_t *inst);
    void pop(inst_t *inst);
    void schedule(inst_t *inst);
    void handle_cache();
    bool is_available();
    void store_forward(inst_t *inst);
    void mem_disamb_check(inst_t *inst);
    inst_t* handle_deadlock();

private:
    std::map<uint64_t,inst_t*> mem_disamb;
    std::vector<inst_t*> outgoing;
    std::map<uint64_t,inst_t*> outstanding;
    int size;
    int occupancy;
    pipeline_t *pipeline;
};

} // namespace spx
} // namespace manifold

#endif

#endif // USE_QSIM
