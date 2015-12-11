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
#include "kitfox-defs.h"
#include "kitfox-client.h"
#include "kitfox.h"
#endif

namespace manifold {
namespace spx {

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

#ifdef LIBKITFOX
class counter_t : public libKitFox::counter_t
{
public:
    counter_t() {}
    ~counter_t() {}
    
    void operator=(const libKitFox::counter_t &cnt)
    {
        switching = cnt.switching;
        read = cnt.read;
        read_tag = cnt.read_tag;
        write = cnt.write;
        write_tag = cnt.write_tag;
        search = cnt.search;
    }
    
    libKitFox::Comp_ID id;
};

class pipeline_counter_t
{
public:
    pipeline_counter_t() { clear(); }
    ~pipeline_counter_t() {}

    void clear()
    {
        l1_btb.clear(); l2_btb.clear();
        predictor_chooser.clear(); global_predictor.clear();
        l1_local_predictor.clear(); l2_local_predictor.clear(); ras.clear();
        inst_cache.clear(); inst_cache_miss_buffer.clear(); inst_cache_prefetch_buffer.clear(); inst_tlb.clear();
        latch_ic2ib.clear(); pc.clear(); inst_buffer.clear(); latch_ib2id.clear();
        inst_decoder.clear(); operand_decoder.clear(); uop_sequencer.clear(); frontend_undiff.clear();

        latch_id2uq.clear(); uop_queue.clear(); latch_uq2rr.clear(); 

        latch_id2iq.clear();
        
        dependency_check.clear(); reg_int.clear(); reg_fp.clear();
        alu.clear(); mul.clear(); int_bypass.clear();
        fpu.clear(); fp_bypass.clear();

        rat.clear(); freelist.clear(); latch_rr2rs.clear();
        rs.clear(); issue_select.clear(); latch_rs2ex.clear(); 
        rob.clear(); latch_rob2rs.clear(); latch_rob2reg.clear(); scheduler_undiff.clear();
        
        inst_queue.clear(); latch_iq2ex.clear(); latch_ex2reg.clear(); execute_undiff.clear();

        latch_ex_int2rob.clear(); ex_int_undiff.clear();
        
        latch_ex_fp2rob.clear(); ex_fp_undiff.clear();
        
        data_cache.clear(); data_cache_miss_buffer.clear(); data_cache_prefetch_buffer.clear();
        data_cache_writeback_buffer.clear(); data_tlb.clear(); l2_tlb.clear(); lsu_undiff.clear();
        
        stq.clear(); latch_stq2dcache.clear(); latch_stq2ldq.clear();
        ldq.clear(); latch_ldq2dcache.clear(); latch_ldq2rs.clear();
        
        lsq.clear(); latch_lsq2dcache.clear(); latch_lsq2reg.clear();
        
        package.clear(); core.clear(); frontend.clear(); lsu.clear();
        scheduler.clear(); ex_int.clear(); ex_fp.clear();
        execute.clear(); undiff.clear();
  }

    void operator*=(const libKitFox::Count &c)
    {
        l1_btb = l1_btb*c; l2_btb = l2_btb*c;
        predictor_chooser = predictor_chooser*c; global_predictor = global_predictor*c;
        l1_local_predictor = l1_local_predictor*c; l2_local_predictor = l2_local_predictor*c; ras = ras*c;
        inst_cache = inst_cache*c; inst_cache_miss_buffer = inst_cache_miss_buffer*c; 
        inst_cache_prefetch_buffer = inst_cache_prefetch_buffer*c; inst_tlb = inst_tlb*c;
        latch_ic2ib = latch_ic2ib*c; pc = pc*c; inst_buffer = inst_buffer*c; latch_ib2id = latch_ib2id*c;
        inst_decoder = inst_decoder*c; operand_decoder = operand_decoder*c; 
        uop_sequencer = uop_sequencer*c; frontend_undiff = frontend_undiff*c;
        
        latch_id2uq = latch_id2uq*c; uop_queue = uop_queue*c; latch_uq2rr = latch_uq2rr*c;

        latch_id2iq = latch_id2iq*c;

        dependency_check = dependency_check*c; reg_int = reg_int*c; reg_fp = reg_fp*c;
        alu = alu*c; mul = mul *c; int_bypass = int_bypass*c;
        fpu = fpu*c; fp_bypass = fp_bypass*c;

        rat = rat*c; freelist = freelist*c; latch_rr2rs = latch_rr2rs*c;
        rs = rs*c; issue_select = issue_select*c; latch_rs2ex = latch_rs2ex*c;
        rob = rob*c; latch_rob2rs = latch_rob2rs*c; latch_rob2reg = latch_rob2reg*c; 
        scheduler_undiff = scheduler_undiff*c;

        inst_queue = inst_queue*c; latch_iq2ex = latch_iq2ex*c; 
        latch_ex2reg = latch_ex2reg*c; execute_undiff = execute_undiff*c;

        latch_ex_int2rob = latch_ex_int2rob*c; ex_int_undiff = ex_int_undiff*c;

        latch_ex_fp2rob = latch_ex_fp2rob*c; ex_fp_undiff = ex_fp_undiff*c;

        data_cache = data_cache*c; data_cache_miss_buffer = data_cache_miss_buffer*c;
        data_cache_prefetch_buffer = data_cache_prefetch_buffer*c; data_cache_writeback_buffer = data_cache_writeback_buffer*c;
        data_tlb = data_tlb*c; l2_tlb = l2_tlb*c; lsu_undiff = lsu_undiff*c;

        stq = stq*c; latch_stq2dcache = latch_stq2dcache*c; latch_stq2ldq = latch_stq2ldq*c;
        ldq = ldq*c; latch_ldq2dcache = latch_ldq2dcache*c; latch_ldq2rs = latch_ldq2rs*c;

        lsq = lsq*c; latch_lsq2dcache*c; latch_lsq2reg*c;

        package = package*c; core = core*c; frontend = frontend*c; lsu = lsu*c;
        scheduler = scheduler*c; ex_int = ex_int*c; ex_fp = ex_fp*c;
        execute = execute*c; undiff = undiff*c;
    }
  
    // Frontend components (including inst cache)
    counter_t l1_btb, l2_btb;
    counter_t predictor_chooser, global_predictor;
    counter_t l1_local_predictor, l2_local_predictor, ras;
    counter_t inst_cache, inst_cache_miss_buffer, inst_cache_prefetch_buffer, inst_tlb;
    counter_t latch_ic2ib, pc, inst_buffer, latch_ib2id;
    counter_t inst_decoder, operand_decoder, uop_sequencer, frontend_undiff;
    // Outorder Frontend components
    counter_t latch_id2uq, uop_queue, latch_uq2rr;
    // Inorder Frontend components
    counter_t latch_id2iq;
    
    // Scheduler/Execute components
    counter_t dependency_check, reg_int, reg_fp;
    counter_t alu, mul, int_bypass;
    counter_t fpu, fp_bypass;
      
    // Outorder Scheduler components
    counter_t rat, freelist, latch_rr2rs;
    counter_t rs, issue_select, latch_rs2ex;
    counter_t rob, latch_rob2rs, latch_rob2reg, scheduler_undiff;
    
    // Inorder Execute components
    counter_t inst_queue, latch_iq2ex, latch_ex2reg, execute_undiff;
      
    // Outorder Integer execution unit components
    counter_t latch_ex_int2rob, ex_int_undiff;

    // Outorder Floating-point execution unit components
    counter_t latch_ex_fp2rob, ex_fp_undiff;

    // Load store unit components (including data cache not used)
    counter_t data_cache, data_cache_miss_buffer, data_cache_prefetch_buffer;
    counter_t data_cache_writeback_buffer, data_tlb, l2_tlb, lsu_undiff;
    
    // Outroder Load store unit components
    counter_t stq, latch_stq2dcache, latch_stq2ldq;
    counter_t ldq, latch_ldq2dcache, latch_ldq2rs;
    
    // Inorder Load store unit components
    counter_t lsq, latch_lsq2dcache, latch_lsq2reg;
    
    // Package, core, and pipeline
    counter_t package, core, frontend, lsu;
    counter_t scheduler, ex_int, ex_fp;
    counter_t execute, undiff;
};
#endif

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
    pipeline_counter_t counter;
#endif

    spx_core_t *core;
};

} //namespace manifold
} //namespace spx
#endif

#endif // USE_QSIM
