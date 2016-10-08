#ifdef USE_QSIM

#ifndef __SPX_PIPELINE_H__
#define __SPX_PIPELINE_H__

#include <libconfig.h++>
#include "qsim.h"
#ifdef QSIM_CLIENT
#include "qsim-client.h"
#endif
#include "instruction.h"
#include "component.h"
#include "cache/cache.h"

#ifdef LIBKITFOX
#include "uarch/kitfoxCounter.h"
#endif

namespace manifold {
namespace spx {

constexpr unsigned int str2int(const char* str, int h = 0)
{
    return !str[h] ? 5381 : (str2int(str, h+1)*33) ^ str[h];
}

enum SPX_ARCH_TYPES
{
    SPX_X86 = 0,
    SPX_X64,
    SPX_A64, // a64 stands for arm64
    SPX_NUM_ARCH_TYPES
};

enum SPX_PIPELINE_TYPES
{
    SPX_PIPELINE_OUTORDER = 0,
    SPX_PIPELINE_INORDER,
    SPX_NUM_PIPELINE_TYPES
};

enum SPX_QSIM_CB_TYPES
{
    SPX_QSIM_INST_CB = 0,
    SPX_QSIM_SRC_REG_CB,
    SPX_QSIM_SRC_FLAG_CB,
    SPX_QSIM_MEM_CB,
    SPX_QSIM_LD_CB,
    SPX_QSIM_ST_CB,
    SPX_QSIM_DEST_REG_CB,
    SPX_QSIM_DEST_FLAG_CB,
    NUM_SPX_QSIM_CB_TYPES
};

class pipeline_config_t
{
public:
    pipeline_config_t() {}
    ~pipeline_config_t() {}

    bool memory_deadlock_avoidance;
    uint64_t memory_deadlock_threshold;

    int fetch_width;
    int alloc_width;
    int exec_width;
    int commit_width;

    int instQ_size;
    int RS_size;
    int LDQ_size;
    int STQ_size;
    int ROB_size;

    int FU_delay[SPX_NUM_FU_TYPES]; // delays for each FU
    int FU_issue_rate[SPX_NUM_FU_TYPES]; // issue rate for each FU
    std::vector<int> FU_port[SPX_NUM_FU_TYPES]; // ports for each FU

    uint64_t cache_line_width;
    uint64_t mem_addr_mask;

    enum SPX_ARCH_TYPES arch_type;
};

class pipeline_stats_t
{
public:
    pipeline_stats_t()
    {
        uop_count = 0;
        Mop_count = 0;
        last_commit_cycle = 0;
        core_time = 0.0;
        total_time = 0.0;

        interval.clock_cycle = 0;
        interval.uop_count = 0;
        interval.Mop_count = 0;
        interval.core_time = 0.0;
    }
    ~pipeline_stats_t() {}

    uint64_t uop_count;
    uint64_t Mop_count;
    uint64_t last_commit_cycle;
    double core_time;
    double total_time;

    struct {
        uint64_t clock_cycle;
        uint64_t uop_count;
        uint64_t Mop_count;
        double core_time;
        timeval start_time;
    }interval;
};

class spx_qsim_proxy_t;

class pipeline_t
{
public:
    pipeline_t();
    virtual ~pipeline_t();

    // pipeline functions
    virtual void commit() = 0;
    virtual void memory() = 0;
    virtual void execute() = 0;
    virtual void allocate() = 0;
    virtual void frontend() = 0;
    virtual void fetch(inst_t *inst) = 0;
    virtual void handle_cache_response(int temp, cache_request_t *cache_request) = 0;
    virtual void set_config(libconfig::Config *config) = 0;
    void handle_long_inst(inst_t *inst);
    void handle_deadlock_inst(inst_t *inst);
    void debug_deadlock_inst(inst_t *inst);
    bool is_nop(inst_t *inst);

    // Qsim callback functions
    void Qsim_inst_cb(int core_id, uint64_t vaddr, uint64_t paddr, uint8_t len, const uint8_t *bytes, enum inst_type type);
    void Qsim_mem_cb(int core_id, uint64_t vaddr, uint64_t paddr, uint8_t size, int type);
    void Qsim_reg_cb(int core_id, int reg, uint8_t size, int type);
    void Qsim_post_cb(inst_t *inst); // modify inst information after Qsim callbacks are completed
    void set_qsim_proxy(spx_qsim_proxy_t *QsimProxy);

protected:
    inst_t *next_inst; // next_inst from Qsim_cb
    spx_qsim_proxy_t *qsim_proxy;

    uint64_t Mop_count;
    uint64_t uop_count;
    int Qsim_cb_status;

public:
    int Qsim_osd_state;

    int type;
    pipeline_config_t config;
    pipeline_stats_t stats;

    cache_table_t *inst_cache;
    cache_table_t *inst_tlb;
    cache_table_t *data_tlb;
    cache_table_t *l2_tlb;

#ifdef LIBKITFOX
    manifold::uarch::pipeline_counter_t counter;
#endif

    spx_core_t *core;
};

} //namespace manifold
} //namespace spx
#endif

#endif // USE_QSIM
