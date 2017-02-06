#ifdef USE_KITFOX
#include "kitfox_proxy.h"

using namespace std;
using namespace libKitFox;
using namespace manifold;
using namespace manifold::kernel;
using namespace manifold::kitfox_proxy;
using namespace manifold::uarch;

kitfox_proxy_t::kitfox_proxy_t(const char *ConfigFile,
                               uint64_t SamplingFreq) :
    comp_num(0)
{
    kitfox = new kitfox_t(/* Intra (local) MPI Comm */ NULL,
                          /* Inter MPI Comm */ NULL);

    cout << "Initializing KitFox ..." << endl;
    kitfox->configure(ConfigFile);

    cout << "Registering clock ( " << SamplingFreq / 1e6  << " MHz) to KitFox Proxy ..." << endl;
    Clock* clk = new Clock(SamplingFreq);
    manifold::kernel::Clock :: Register(*clk, (kitfox_proxy_t*)this, &kitfox_proxy_t::tick, (void(kitfox_proxy_t::*)(void))0);

    /* Get the pair of <first ID, last ID> of KitFox pseudo components */
    pair<Comp_ID, Comp_ID> kitfox_component_id =
    kitfox->component_id_range;

#if 0
    /* Check which components are associated with which libraries. */
    for(Comp_ID pseudo_component_id = kitfox_component_id.first;
        pseudo_component_id <= kitfox_component_id.second;
        pseudo_component_id++) {
        /* Pseudo component is associated with energy library. */
        if(kitfox->get_energy_library(pseudo_component_id)) {
            add_kitfox_power_component(pseudo_component_id);
        }
        /* Pseudo component is associated with thermal library. */
        if(kitfox->get_thermal_library(pseudo_component_id)) {
            add_kitfox_thermal_component(pseudo_component_id);
        }
        /* Pseudo component is associated with reliability library. */
        if(kitfox->get_reliability_library(pseudo_component_id)) {
            add_kitfox_reliability_component(pseudo_component_id);
        }
    }
#endif

    cout << "Finished initializing KitFox" << endl;
}


kitfox_proxy_t::~kitfox_proxy_t()
{
    cout << "Terminating KitFox" << endl;
    delete kitfox;
    delete m_clk;
}


void kitfox_proxy_t::tick()
{
    if (m_clk->NowTicks() == 0)
        return ;

    int core_id = 0;
    int l1cache_id = 0;
    int l2cache_id = 0;
    for (auto id: manifold_node) {
        // core_model only, FIXME add test function for other models here
        // difference between manifold_node id and core_id???

        switch (id.second) {
            case KitFoxType::core_type: {
                kitfox_proxy_request_t<pipeline_counter_t> *req;
                req = new kitfox_proxy_request_t<pipeline_counter_t> (core_id, id.second, m_clk->NowTicks() * m_clk->period);
                Send(id.first, req);
                core_id++;
                break;
            }
            case KitFoxType::l1cache_type: {
                kitfox_proxy_request_t<cache_counter_t> *req;
                req = new kitfox_proxy_request_t<cache_counter_t> (l1cache_id, id.second, m_clk->NowTicks() * m_clk->period);
                Send(id.first, req);
                l1cache_id++;
                break;
            }
            case KitFoxType::l2cache_type: {
                kitfox_proxy_request_t<cache_counter_t> *req;
                req = new kitfox_proxy_request_t<cache_counter_t> (l2cache_id, id.second, m_clk->NowTicks() * m_clk->period);
                Send(id.first, req);
                l2cache_id++;
                break;
            }
            default:
                cerr << "error manifold_node type!" << endl;
                exit(1);
        }

    }
#if 0
    uint64_t clock_cycle = m_clk->NowTicks();

    /* Invoke KitFox calculations */
    if(clock_cycle % sampling_interval == 0) {
        /* Collect counters from Manifold nodes. */
        for(vector<pair<libKitFox::Comp_ID, libKitFox::counter_t*> >::iterator it
                = kitfox_power_component.begin();
            it != kitfox_power_component.end(); it++) {
        }

        /* Invoke calculate_power() */
        for(vector<pair<libKitFox::Comp_ID, libKitFox::counter_t*> >::iterator it
                = kitfox_power_component.begin();
            it != kitfox_power_component.end(); it++) {
        }

        /* Invoke calculate_temperature() */
        for(vector<Comp_ID>::iterator it = kitfox_thermal_component.begin();
            it != kitfox_thermal_component.end(); it++) {
        }

        /* Invoke calculate_failure_rate() */
        for(vector<Comp_ID>::iterator it = kitfox_reliability_component.begin();
            it != kitfox_reliability_component.end(); it++) {
        }
    }
#endif
}


void kitfox_proxy_t::tock()
{
    /* Nothing to do */
}

/* Add Manifold components to withdraw counters. */
void kitfox_proxy_t::add_manifold_node(CompId_t CompId, KitFoxType t)
{
    /* Check if there already exists the same node ID. */
    for(auto comp: manifold_node) {
        if(comp.first == CompId) { return; }
    }

    manifold_node.push_back(make_pair(CompId, t));
}

#if 0
/* Add KitFox components to calculate power. */
void kitfox_proxy_t::add_kitfox_power_component(Comp_ID ComponentID)
{
    /* Check if there already exists the same component ID. */
    for(vector<pair<libKitFox::Comp_ID, libKitFox::counter_t*> >::iterator it =
            kitfox_power_component.begin();
        it != kitfox_power_component.end(); it++) {
        if(it->first == ComponentID) { return; }
    }

    kitfox_power_component.push_back(make_pair(ComponentID, (libKitFox::counter_t*)NULL));
}

/* Add KitFox components to calculate thermal. */
void kitfox_proxy_t::add_kitfox_thermal_component(Comp_ID ComponentID)
{
    /* Check if there already exists the same component ID. */
    for(vector<Comp_ID>::iterator it = kitfox_thermal_component.begin();
        it != kitfox_thermal_component.end(); it++) {
        if((*it) == ComponentID) { return; }
    }

    kitfox_thermal_component.push_back(ComponentID);
}

/* Add KitFox components to calculate reliability. */

void kitfox_proxy_t::add_kitfox_reliability_component(Comp_ID ComponentID)
{
    /* Check if there already exists the same component ID. */
    for(vector<Comp_ID>::iterator it = kitfox_reliability_component.begin();
        it != kitfox_reliability_component.end(); it++) {
        if((*it) == ComponentID) { return; }
    }

    kitfox_reliability_component.push_back(ComponentID);
}
#endif

void kitfox_proxy_t::calculate_power(manifold::uarch::pipeline_counter_t c, manifold::kernel::Time_t t, const string prefix)
{
        string comp;
        libKitFox::Comp_ID comp_id;

        // l1_btb
        comp = prefix + ".l1_btb";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.l1_btb);

        // l2_btb
        comp = prefix + ".l2_btb";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.l2_btb);

        // predictor_chooser
        comp = prefix + ".predictor_chooser";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.predictor_chooser);

        // global_predictor
        comp = prefix + ".global_predictor";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.global_predictor);

        // l1_local_predictor
        comp = prefix + ".l1_local_predictor";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.l1_local_predictor);

        // l2_local_predictor
        comp = prefix + ".l2_local_predictor";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.l2_local_predictor);

        // ras
        comp = prefix + ".ras";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.ras);

        // inst_cache
        comp = prefix + ".inst_cache";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.inst_cache);

        // inst_cache_miss_buffer
        comp = prefix + ".inst_cache_miss_buffer";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.inst_cache_miss_buffer);

        // inst_tlb
        comp = prefix + ".inst_tlb";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.inst_tlb);

        // latch_ic2ib
        comp = prefix + ".latch_ic2ib";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.latch_ic2ib);

        // pc
        comp = prefix + ".pc";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.pc);

        // inst_buffer
        comp = prefix + ".inst_buffer";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.inst_buffer);

        // latch_ib2id
        comp = prefix + ".latch_ib2id";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.latch_ib2id);

        // inst_decoder
        comp = prefix + ".inst_decoder";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.inst_decoder);

        // operand_decoder
        comp = prefix + ".operand_decoder";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.operand_decoder);

        // uop_sequencer
        comp = prefix + ".uop_sequencer";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.uop_sequencer);

        // latch_id2uq
        comp = prefix + ".latch_id2uq";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.latch_id2uq);

        // uop_queue
        comp = prefix + ".uop_queue";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.uop_queue);

        // latch_uq2rr
        comp = prefix + ".latch_uq2rr";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.latch_uq2rr);

        // rat
        comp = prefix + ".rat";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.rat);

        // freelist
        comp = prefix + ".freelist";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.freelist);

        // dependency_check
        comp = prefix + ".dependency_check";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.dependency_check);

        // latch_id2iq (in-order)
        // comp = prefix + ".latch_id2iq";
        // comp_id = kitfox->get_component_id(comp);
        // assert(comp_id != INVALID_COMP_ID);
        // kitfox->calculate_power(comp_id, t, m_clk->period, c.latch_id2iq);

        // latch_rr2rs
        comp = prefix + ".latch_rr2rs";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.latch_rr2rs);

        // rs
        comp = prefix + ".rs";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.rs);

        // issue_select
        comp = prefix + ".issue_select";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.issue_select);

        // latch_rs2ex
        comp = prefix + ".latch_rs2ex";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.latch_rs2ex);

        // reg_int
        comp = prefix + ".reg_int";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.reg_int);

        // reg_fp
        comp = prefix + ".reg_fp";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.reg_fp);

        // rob
        comp = prefix + ".rob";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.rob);

        // latch_rob2rs
        comp = prefix + ".latch_rob2rs";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.latch_rob2rs);

        // latch_rob2reg
        comp = prefix + ".latch_rob2reg";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.latch_rob2reg);

        // alu
        comp = prefix + ".alu";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.alu);

        // mul
        comp = prefix + ".mul";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.mul);

        // int_bypass
        comp = prefix + ".int_bypass";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.int_bypass);

        // latch_ex_int2rob
        comp = prefix + ".latch_ex_int2rob";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.latch_ex_int2rob);

        // fpu
        comp = prefix + ".fpu";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.fpu);

        // fp_bypass
        comp = prefix + ".fp_bypass";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.fp_bypass);

        // latch_ex_fp2rob
        comp = prefix + ".latch_ex_fp2rob";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.latch_ex_fp2rob);

        // stq
        comp = prefix + ".stq";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.stq);

        // latch_stq2dcache
        comp = prefix + ".latch_stq2dcache";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.latch_stq2dcache);

        // latch_stq2ldq
        comp = prefix + ".latch_stq2ldq";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.latch_stq2ldq);

        // ldq
        comp = prefix + ".ldq";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.ldq);

        // latch_ldq2dcache
        comp = prefix + ".latch_ldq2dcache";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.latch_ldq2dcache);

        // latch_ldq2rs
        comp = prefix + ".latch_ldq2rs";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.latch_ldq2rs);

        // lsq (in-order)
        // comp = prefix + ".lsq";
        // comp_id = kitfox->get_component_id(comp);
        // assert(comp_id != INVALID_COMP_ID);
        // kitfox->calculate_power(comp_id, t, m_clk->period, c.lsq);

        // latch_lsq2dcache (in-order)
        // comp = prefix + ".latch_lsq2dcache";
        // comp_id = kitfox->get_component_id(comp);
        // assert(comp_id != INVALID_COMP_ID);
        // kitfox->calculate_power(comp_id, t, m_clk->period, c.latch_lsq2dcache);

        // latch_lsq2reg (in-order)
        // comp = prefix + ".latch_lsq2reg";
        // comp_id = kitfox->get_component_id(comp);
        // assert(comp_id != INVALID_COMP_ID);
        // kitfox->calculate_power(comp_id, t, m_clk->period, c.latch_lsq2reg);

        // data_cache
        comp = prefix + ".data_cache";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.data_cache);

        // data_cache_miss_buffer
        comp = prefix + ".data_cache_miss_buffer";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.data_cache_miss_buffer);

        // data_cache_prefetch_buffer
        comp = prefix + ".data_cache_prefetch_buffer";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.data_cache_prefetch_buffer);

        // data_cache_writeback_buffer
        comp = prefix + ".data_cache_writeback_buffer";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.data_cache_writeback_buffer);

        // data_tlb
        comp = prefix + ".data_tlb";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.data_tlb);

        // l2_tlb
        comp = prefix + ".l2_tlb";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.l2_tlb);

        // undiff
        comp = prefix + ".undiff";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.undiff);

#if 0
        // frontend_undiff
        comp = prefix + "frontend_undiff";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.frontend_undiff);

        // scheduler_undiff
        comp = prefix + "scheduler_undiff";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.scheduler_undiff);

        // ex_int_undiff
        comp = prefix + "ex_int_undiff";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.ex_int_undiff);

        // ex_fp_undiff
        comp = prefix + "ex_fp_undiff";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.ex_fp_undiff);

        // lsu_undiff
        comp = prefix + "lsu_undiff";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.lsu_undiff);

        // package
        comp = prefix + "package";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.package);

        // core
        comp = prefix + "core";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.core);

        // frontend
        comp = prefix + "frontend";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.frontend);

        // lsu
        comp = prefix + "lsu";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.lsu);

        // scheduler
        comp = prefix + "scheduler";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.scheduler);

        // ex_int
        comp = prefix + "ex_int";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.ex_int);

        // ex_fp
        comp = prefix + "ex_fp";
        comp_id = kitfox->get_component_id(comp);
        assert(comp_id != INVALID_COMP_ID);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.ex_fp);

        // execute
        comp = prefix + "execute";
        assert(comp_id != INVALID_COMP_ID);
        comp_id = kitfox->get_component_id(comp);
        kitfox->calculate_power(comp_id, t, m_clk->period, c.execute);
#endif

        // synchronize core power
        comp = prefix;
        comp_id = kitfox->get_component_id(comp);
        kitfox->synchronize_data(comp_id, t, m_clk->period, libKitFox::KITFOX_DATA_POWER);

        // print core power
        libKitFox::power_t power;
        assert(kitfox->pull_data(comp_id, t, m_clk->period, libKitFox::KITFOX_DATA_POWER, &power) == libKitFox::KITFOX_QUEUE_ERROR_NONE);
        cerr << prefix + ".power = " << power.get_total() << "W (dynamic = " << power.dynamic << "W, leakage = " << power.leakage << "W) @" << t << endl;

}

void kitfox_proxy_t::calculate_power(manifold::uarch::cache_counter_t c, manifold::kernel::Time_t t, const string prefix)
{
    string comp;
    libKitFox::Comp_ID comp_id;

    // cache
    comp = prefix + ".cache";
    comp_id = kitfox->get_component_id(comp);
    assert(comp_id != INVALID_COMP_ID);
    kitfox->calculate_power(comp_id, t, m_clk->period, c.cache);

    // tlb
    comp = prefix + ".tlb";
    comp_id = kitfox->get_component_id(comp);
    assert(comp_id != INVALID_COMP_ID);
    kitfox->calculate_power(comp_id, t, m_clk->period, c.cache);

    // prefetch
    comp = prefix + ".prefetch";
    comp_id = kitfox->get_component_id(comp);
    assert(comp_id != INVALID_COMP_ID);
    kitfox->calculate_power(comp_id, t, m_clk->period, c.prefetch);

    // missbuf
    comp = prefix + ".missbuf";
    comp_id = kitfox->get_component_id(comp);
    assert(comp_id != INVALID_COMP_ID);
    kitfox->calculate_power(comp_id, t, m_clk->period, c.missbuf);

    // linefill
    comp = prefix + ".linefill";
    comp_id = kitfox->get_component_id(comp);
    assert(comp_id != INVALID_COMP_ID);
    kitfox->calculate_power(comp_id, t, m_clk->period, c.linefill);

    // writeback
    comp = prefix + ".writeback";
    comp_id = kitfox->get_component_id(comp);
    assert(comp_id != INVALID_COMP_ID);
    kitfox->calculate_power(comp_id, t, m_clk->period, c.writeback);

    // undiff
    comp = prefix + ".undiff";
    comp_id = kitfox->get_component_id(comp);
    assert(comp_id != INVALID_COMP_ID);
    kitfox->calculate_power(comp_id, t, m_clk->period, c.undiff);

    // synchronize core power
    comp = prefix;
    comp_id = kitfox->get_component_id(comp);
    kitfox->synchronize_data(comp_id, t, m_clk->period, libKitFox::KITFOX_DATA_POWER);

    // print core power
    libKitFox::power_t power;
    assert(kitfox->pull_data(comp_id, t, m_clk->period, libKitFox::KITFOX_DATA_POWER, &power) == libKitFox::KITFOX_QUEUE_ERROR_NONE);
    cerr << prefix + ".power = " << power.get_total() << "W (dynamic = " << power.dynamic << "W, leakage = " << power.leakage << "W) @" << t << endl;
}

#endif //USE_KITFOX
