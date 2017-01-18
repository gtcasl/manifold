#ifndef __KITFOX_PROXY_H__
#define __KITFOX_PROXY_H__
#ifdef USE_KITFOX

#include <assert.h>
#include <string>
#include "kitfox.h"
#include "kernel/component.h"
#include "kernel/clock.h"
#include "uarch/kitfoxCounter.h"

namespace manifold {
namespace kitfox_proxy {

template <typename T>
class kitfox_proxy_request_t
{
public:
    kitfox_proxy_request_t() {}
    kitfox_proxy_request_t(int CoreID, manifold::uarch::KitFoxType s, manifold::kernel::Time_t t) : core_id(CoreID), type(s), time(t) {  counter.clear(); }
    ~kitfox_proxy_request_t() {}
    int get_id() const { return core_id; }
    manifold::uarch::KitFoxType get_type()  const { return type; }
    manifold::kernel::Time_t get_time() const { return time; }
    void set_counter (const T c) { counter = c; }
    T get_counter (void) { return counter;}

    // kitfox_proxy_request_t operator=(const kitfox_proxy_request_t &_)
    // {
    //     core_id = _.core_id;
    //     time = _.time;
    //     counter = _.counter;
    //     name = _.name;

    //     return *this;
    // }

private:
    int core_id;
    manifold::kernel::Time_t time;
    manifold::uarch::KitFoxType type;
    T counter;
};

class kitfox_proxy_t : public manifold::kernel::Component
{
public:
    kitfox_proxy_t(const char *ConfigFile,
                   uint64_t SamplingFreq);
    ~kitfox_proxy_t();

    void tick();
    void tock();

    /* Add Manifold components to calculate power. */
    void add_manifold_node(manifold::kernel::CompId_t);
    void pair_counter_to_power_component(manifold::kernel::CompId_t);

    /* Add KitFox components to invoke calculations. */
    void add_kitfox_power_component(libKitFox::Comp_ID);
    void add_kitfox_thermal_component(libKitFox::Comp_ID);
#if 0
    void add_kitfox_reliability_component(libKitFox::Comp_ID);
#endif

    template <typename T> void handle_kitfox_proxy_response(int temp, kitfox_proxy_request_t<T> *Req);

private:
    libKitFox::kitfox_t *kitfox;
    size_t comp_num;
    //int comp_num;

    std::vector<manifold::kernel::CompId_t> manifold_node;
    std::vector<std::pair<libKitFox::Comp_ID, libKitFox::counter_t*> > kitfox_power_component;
    std::vector<libKitFox::Comp_ID> kitfox_thermal_component;
#if 0
    std::vector<libKitFox::Comp_ID> kitfox_reliability_component;
#endif
};

template <typename T>
void kitfox_proxy_t::handle_kitfox_proxy_response(int temp, kitfox_proxy_request_t<T> *Req)
{
    assert(Req != NULL);
    T c = Req->get_counter();

    //FIXME: core model only, add support for other models later
    assert(Req->get_type() == manifold::uarch::KitFoxType::core_type);

    if (Req->get_type() == manifold::uarch::KitFoxType::core_type) {
        string prefix = "package.core_die.core" + std::to_string(Req->get_id());
        string comp;
        libKitFox::Comp_ID comp_id;

        // l1_btb
        comp = prefix + ".l1_btb";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.l1_btb);

        // l2_btb
        comp = prefix + ".l2_btb";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.l2_btb);

        // predictor_chooser
        comp = prefix + ".predictor_chooser";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.predictor_chooser);

        // global_predictor
        comp = prefix + ".global_predictor";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.global_predictor);

        // l1_local_predictor
        comp = prefix + ".l1_local_predictor";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.l1_local_predictor);

        // l2_local_predictor
        comp = prefix + ".l2_local_predictor";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.l2_local_predictor);

        // ras
        comp = prefix + ".ras";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.ras);

        // inst_cache
        comp = prefix + ".inst_cache";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.inst_cache);

        // inst_cache_miss_buffer
        comp = prefix + ".inst_cache_miss_buffer";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.inst_cache_miss_buffer);

        // inst_tlb
        comp = prefix + ".inst_tlb";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.inst_tlb);

        // latch_ic2ib
        comp = prefix + ".latch_ic2ib";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.latch_ic2ib);

        // pc
        comp = prefix + ".pc";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.pc);

        // inst_buffer
        comp = prefix + ".inst_buffer";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.inst_buffer);

        // latch_ib2id
        comp = prefix + ".latch_ib2id";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.latch_ib2id);

        // inst_decoder
        comp = prefix + ".inst_decoder";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.inst_decoder);

        // operand_decoder
        comp = prefix + ".operand_decoder";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.operand_decoder);

        // uop_sequencer
        comp = prefix + ".uop_sequencer";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.uop_sequencer);

        // latch_id2uq
        comp = prefix + ".latch_id2uq";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.latch_id2uq);

        // uop_queue
        comp = prefix + ".uop_queue";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.uop_queue);

        // latch_uq2rr
        comp = prefix + ".latch_uq2rr";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.latch_uq2rr);

        // rat
        comp = prefix + ".rat";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.rat);

        // freelist
        comp = prefix + ".freelist";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.freelist);

        // dependency_check
        comp = prefix + ".dependency_check";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.dependency_check);

        // latch_id2iq (in-order)
        // comp = prefix + ".latch_id2iq";
        // comp_id = kitfox->get_component_id(comp);
        // assert(comp_id != INVALID_COMP_ID);
        // kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.latch_id2iq);

        // latch_rr2rs
        comp = prefix + ".latch_rr2rs";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.latch_rr2rs);

        // rs
        comp = prefix + ".rs";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.rs);

        // issue_select
        comp = prefix + ".issue_select";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.issue_select);

        // latch_rs2ex
        comp = prefix + ".latch_rs2ex";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.latch_rs2ex);

        // reg_int
        comp = prefix + ".reg_int";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.reg_int);

        // reg_fp
        comp = prefix + ".reg_fp";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.reg_fp);

        // rob
        comp = prefix + ".rob";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.rob);

        // latch_rob2rs
        comp = prefix + ".latch_rob2rs";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.latch_rob2rs);

        // latch_rob2reg
        comp = prefix + ".latch_rob2reg";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.latch_rob2reg);

        // alu
        comp = prefix + ".alu";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.alu);

        // mul
        comp = prefix + ".mul";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.mul);

        // int_bypass
        comp = prefix + ".int_bypass";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.int_bypass);

        // latch_ex_int2rob
        comp = prefix + ".latch_ex_int2rob";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.latch_ex_int2rob);

        // fpu
        comp = prefix + ".fpu";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.fpu);

        // fp_bypass
        comp = prefix + ".fp_bypass";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.fp_bypass);

        // latch_ex_fp2rob
        comp = prefix + ".latch_ex_fp2rob";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.latch_ex_fp2rob);

        // stq
        comp = prefix + ".stq";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.stq);

        // latch_stq2dcache
        comp = prefix + ".latch_stq2dcache";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.latch_stq2dcache);

        // latch_stq2ldq
        comp = prefix + ".latch_stq2ldq";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.latch_stq2ldq);

        // ldq
        comp = prefix + ".ldq";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.ldq);

        // latch_ldq2dcache
        comp = prefix + ".latch_ldq2dcache";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.latch_ldq2dcache);

        // latch_ldq2rs
        comp = prefix + ".latch_ldq2rs";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.latch_ldq2rs);

        // lsq (in-order)
        // comp = prefix + ".lsq";
        // comp_id = kitfox->get_component_id(comp);
        // assert(comp_id != INVALID_COMP_ID);
        // kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.lsq);

        // latch_lsq2dcache (in-order)
        // comp = prefix + ".latch_lsq2dcache";
        // comp_id = kitfox->get_component_id(comp);
        // assert(comp_id != INVALID_COMP_ID);
        // kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.latch_lsq2dcache);

        // latch_lsq2reg (in-order)
        // comp = prefix + ".latch_lsq2reg";
        // comp_id = kitfox->get_component_id(comp);
        // assert(comp_id != INVALID_COMP_ID);
        // kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.latch_lsq2reg);

        // data_cache
        comp = prefix + ".data_cache";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.data_cache);

        // data_cache_miss_buffer
        comp = prefix + ".data_cache_miss_buffer";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.data_cache_miss_buffer);

        // data_cache_prefetch_buffer
        comp = prefix + ".data_cache_prefetch_buffer";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.data_cache_prefetch_buffer);

        // data_cache_writeback_buffer
        comp = prefix + ".data_cache_writeback_buffer";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.data_cache_writeback_buffer);

        // data_tlb
        comp = prefix + ".data_tlb";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.data_tlb);

        // l2_tlb
        comp = prefix + ".l2_tlb";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.l2_tlb);

        // undiff
        comp = prefix + ".undiff";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.undiff);

#if 0
        // frontend_undiff
        comp = prefix + "frontend_undiff";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.frontend_undiff);

        // scheduler_undiff
        comp = prefix + "scheduler_undiff";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.scheduler_undiff);

        // ex_int_undiff
        comp = prefix + "ex_int_undiff";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.ex_int_undiff);

        // ex_fp_undiff
        comp = prefix + "ex_fp_undiff";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.ex_fp_undiff);

        // lsu_undiff
        comp = prefix + "lsu_undiff";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.lsu_undiff);

        // package
        comp = prefix + "package";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.package);

        // core
        comp = prefix + "core";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.core);

        // frontend
        comp = prefix + "frontend";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.frontend);

        // lsu
        comp = prefix + "lsu";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.lsu);

        // scheduler
        comp = prefix + "scheduler";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.scheduler);

        // ex_int
        comp = prefix + "ex_int";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.ex_int);

        // ex_fp
        comp = prefix + "ex_fp";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.ex_fp);

        // execute
        comp = prefix + "execute";
        assert(comp_id != INVALID_COMP_ID);
        comp_id = kitfox->get_component_id(comp);
        kitfox->calculate_power(comp_id, Req->get_time(), m_clk->period, c.execute);
#endif

        // synchronize core power
        comp = prefix;
        comp_id = kitfox->get_component_id(comp);
        kitfox->synchronize_data(comp_id, Req->get_time(), m_clk->period, libKitFox::KITFOX_DATA_POWER);

        // print core power
        libKitFox::power_t power;
        assert(kitfox->pull_data(comp_id, Req->get_time(), m_clk->period, libKitFox::KITFOX_DATA_POWER, &power) == libKitFox::KITFOX_QUEUE_ERROR_NONE);
        cerr << prefix + ".power = " << power.get_total() << "W (dynamic = " << power.dynamic << "W, leakage = " << power.leakage << "W) @" << Req->get_time() << endl;

        comp_num += 1;
    }

    if (comp_num == manifold_node.size()) {
        libKitFox::Comp_ID package_id = kitfox->get_component_id("package");
        assert(package_id != INVALID_COMP_ID);
        kitfox->calculate_temperature(package_id, Req->get_time(), m_clk->period);

        for(int i = 0; i < manifold_node.size(); i++){
            libKitFox::Kelvin t;
            string partition = "package.core_die.core" + std::to_string(i);
            libKitFox::Comp_ID par_id = kitfox->get_component_id(partition);
            assert(kitfox->pull_data(par_id, Req->get_time(), m_clk->period, libKitFox::KITFOX_DATA_TEMPERATURE, &t) == libKitFox::KITFOX_QUEUE_ERROR_NONE);

            cerr << partition + ".temperature = " << t << "Kelvin @" << Req->get_time() << endl;
        }

        // Print thermal grid
#if 0
        libKitFox::grid_t<libKitFox::Kelvin> thermal_grid;
        assert(kitfox->pull_data(package_id, Req->get_time(), m_clk->period, libKitFox::KITFOX_DATA_THERMAL_GRID, &thermal_grid) == libKitFox::KITFOX_QUEUE_ERROR_NONE);

        for(unsigned z = 0; z < thermal_grid.dies(); z++) {
            printf("die %d:\n", z);
            for(unsigned x = 0; x < thermal_grid.rows(); x++) {
                for(unsigned y = 0; y < thermal_grid.cols(); y++) {
                    printf("%3.2lf ", thermal_grid(x, y, z));
                }
                printf("\n");
            }
            printf("\n");
        }
        printf("\n");
#endif

        // reset kitfox component counter
        comp_num = 0;
    }

    delete Req;
}


} // namespace kitfox_proxy
} // namespace manifold

#endif // USE_KITFOX
#endif
