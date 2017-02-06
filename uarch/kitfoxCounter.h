#ifndef MANIFOLD_UARCH_KITFOXCOUNTER_H
#define MANIFOLD_UARCH_KITFOXCOUNTER_H

#include "kitfox-defs.h"
#include "communicator/kitfox-client.h"
#include "kitfox.h"

namespace manifold {
namespace uarch {

enum KitFoxType {
    null_type = 0,
    core_type,
    l1cache_type,
    l2cache_type,
    network_type,
    dram_type,
    num_types
};


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

    void operator=(const pipeline_counter_t &c)
    {
        l1_btb = c.l1_btb; l2_btb = c.l2_btb;
        predictor_chooser = c.predictor_chooser; global_predictor = c.global_predictor;
        l1_local_predictor = c.l1_local_predictor; l2_local_predictor = c.l2_local_predictor; ras = c.ras;
        inst_cache = c.inst_cache; inst_cache_miss_buffer = c.inst_cache_miss_buffer;
        inst_cache_prefetch_buffer = c.inst_cache_prefetch_buffer; inst_tlb = c.inst_tlb;
        latch_ic2ib = c.latch_ic2ib; pc = c.pc; inst_buffer = c.inst_buffer; latch_ib2id = c.latch_ib2id;
        inst_decoder = c.inst_decoder; operand_decoder = c.operand_decoder;
        uop_sequencer = c.uop_sequencer; frontend_undiff = c.frontend_undiff;

        latch_id2uq = c.latch_id2uq; uop_queue = c.uop_queue; latch_uq2rr = c.latch_uq2rr;

        latch_id2iq = c.latch_id2iq;

        dependency_check = c.dependency_check; reg_int = c.reg_int; reg_fp = c.reg_fp;
        alu = c.alu; mul = c.mul ; int_bypass = c.int_bypass;
        fpu = c.fpu; fp_bypass = c.fp_bypass;

        rat = c.rat; freelist = c.freelist; latch_rr2rs = c.latch_rr2rs;
        rs = c.rs; issue_select = c.issue_select; latch_rs2ex = c.latch_rs2ex;
        rob = c.rob; latch_rob2rs = c.latch_rob2rs; latch_rob2reg = c.latch_rob2reg;
        scheduler_undiff = c.scheduler_undiff;

        inst_queue = c.inst_queue; latch_iq2ex = c.latch_iq2ex;
        latch_ex2reg = c.latch_ex2reg; execute_undiff = c.execute_undiff;

        latch_ex_int2rob = c.latch_ex_int2rob; ex_int_undiff = c.ex_int_undiff;

        latch_ex_fp2rob = c.latch_ex_fp2rob; ex_fp_undiff = c.ex_fp_undiff;

        data_cache = c.data_cache; data_cache_miss_buffer = c.data_cache_miss_buffer;
        data_cache_prefetch_buffer = c.data_cache_prefetch_buffer; data_cache_writeback_buffer = c.data_cache_writeback_buffer;
        data_tlb = c.data_tlb; l2_tlb = c.l2_tlb; lsu_undiff = c.lsu_undiff;

        stq = c.stq; latch_stq2dcache = c.latch_stq2dcache; latch_stq2ldq = c.latch_stq2ldq;
        ldq = c.ldq; latch_ldq2dcache = c.latch_ldq2dcache; latch_ldq2rs = c.latch_ldq2rs;

        lsq = c.lsq; latch_lsq2dcache = c.latch_lsq2reg;

        package = c.package; core = c.core; frontend = c.frontend; lsu = c.lsu;
        scheduler = c.scheduler; ex_int = c.ex_int; ex_fp = c.ex_fp;
        execute = c.execute; undiff = c.undiff;
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

        lsq = lsq*c; latch_lsq2dcache = latch_lsq2reg*c;

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

class cache_counter_t
{
public:
    cache_counter_t() { clear(); }
    ~cache_counter_t() {}

    void clear()
    {
        cache.clear();
        tlb.clear();
        missbuf.clear(); prefetch.clear();
        linefill.clear(); writeback.clear();
        undiff.clear();
    }

    void operator=(const cache_counter_t &c)
    {
        cache = c.cache;
        tlb = c.tlb;
        missbuf = c.missbuf; prefetch = c.prefetch;
        linefill = c.linefill; writeback = c.writeback;
        undiff = c.undiff;
    }

    void operator*=(const libKitFox::Count &c)
    {
        cache = cache*c;
        tlb = tlb*c;
        missbuf = missbuf*c; prefetch = prefetch*c;
        linefill = linefill*c; writeback = writeback*c;
        undiff = undiff*c;
    }

    // cache_banks
    counter_t cache;
    // tlb
    counter_t tlb;
    // cache queues
    counter_t missbuf, prefetch;
    counter_t linefill, writeback;
    // undiff
    counter_t undiff;
};

} // namespace uarch
} //namespace manifold

#endif // MANIFOLD_UARCH_KITFOXCOUNTER_H
