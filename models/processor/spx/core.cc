#ifdef USE_QSIM

#include <string.h>
#include <libconfig.h++>
#include <sys/time.h>
#include <sstream>

#include "core.h"
#include "pipeline.h"
#include "outorder.h"
#include "inorder.h"

using namespace std;
using namespace libconfig;
using namespace manifold;
using namespace manifold::kernel;
using namespace manifold::spx;

spx_core_t::spx_core_t(const int nodeID, const char *configFileName, const int coreID) :
    node_id(nodeID),
    core_id(coreID),
    clock_cycle(0),
    active(true),
    qsim_proxy_request_sent(false)
{
    Config parser;
    try {
        parser.readFile(configFileName);
        parser.setAutoConvert(true);
    }
    catch(FileIOException e) {
        fprintf(stdout,"cannot read configuration file %s\n",configFileName);
        exit(EXIT_FAILURE);
    }
    catch(ParseException e) {
        fprintf(stdout,"cannot parse configuration file %s\n",configFileName);
        exit(EXIT_FAILURE);
    }

    try {
        // Pipeline model
        const char *pipeline_model = parser.lookup("pipeline");
        if(!strcasecmp(pipeline_model,"outorder"))
            pipeline = new outorder_t(this,&parser);
        else if(!strcasecmp(pipeline_model,"inorder"))
            pipeline = new inorder_t(this,&parser);
        else {
            fprintf(stdout,"unknown core type %s\n",pipeline_model);
            exit(EXIT_FAILURE);
        }

        // Qsim proxy
        qsim_proxy = new spx_qsim_proxy_t(pipeline);
        pipeline->set_qsim_proxy(qsim_proxy);
    }
    catch(SettingNotFoundException e) {
        fprintf(stdout,"%s not defined\n",e.getPath());
        exit(EXIT_FAILURE);
    }
    catch(SettingTypeException e) {
        fprintf(stdout,"%s has incorrect type\n",e.getPath());
        exit(EXIT_FAILURE);
    }
}

spx_core_t::~spx_core_t() {
    delete qsim_proxy;
    delete pipeline;
}

int spx_core_t::get_qsim_osd_state() const {
    return pipeline->Qsim_osd_state;
}

void spx_core_t::turn_on() {
    active = true;
}

void spx_core_t::turn_off() {
    active = false;
}

void spx_core_t::tick()
{
    clock_cycle = m_clk->NowTicks();
    if(clock_cycle) pipeline->stats.interval.clock_cycle++;

    pipeline->commit();
    pipeline->memory();
    pipeline->execute();
    pipeline->allocate();
    if(active) pipeline->frontend();

#ifdef LIBKITFOX
    pipeline->counter.frontend_undiff.switching++;
    pipeline->counter.scheduler_undiff.switching++;
    pipeline->counter.ex_int_undiff.switching++;
    pipeline->counter.ex_fp_undiff.switching++;
    pipeline->counter.lsu_undiff.switching++;
    pipeline->counter.undiff.switching++;

    print_stats(100000, stderr);
#else
    // This will periodically print the stats to show the progress of simulation -- for debugging
    print_stats(100000, stdout);
#endif

    if (get_qsim_osd_state() == QSIM_OSD_TERMINATED) {
        fprintf(stdout, "SPX core %d out of instructions", core_id);
        manifold::kernel::Manifold::Terminate();
    }
}

void spx_core_t::print_stats(uint64_t sampling_period, FILE *LogFile)
{
    if(clock_cycle&&((clock_cycle%sampling_period) == 0)) {
        fprintf(LogFile,"clk_cycle= %3.1lfM | core%d | \
                         IPC= %lf ( %lu / %lu ), \
                         avgIPC= %lf ( %lu / %lu )\n",
                         (double)clock_cycle/1e6, core_id,
                         (double)pipeline->stats.interval.uop_count / (double)pipeline->stats.interval.clock_cycle,
                         pipeline->stats.interval.uop_count,
                         pipeline->stats.interval.clock_cycle,
                         (double)pipeline->stats.uop_count / (double)clock_cycle,
                         pipeline->stats.uop_count,
                         clock_cycle);
        reset_interval_stats();
    }
}

void spx_core_t::reset_interval_stats()
{
    pipeline->stats.interval.clock_cycle = 0;
    pipeline->stats.interval.Mop_count = 0;
    pipeline->stats.interval.uop_count = 0;
}

void spx_core_t::print_stats(std::ostream& out)
{
    out << "************ SPX Core " << core_id << " [node " << node_id << "] stats *************" << endl;
    out << "  Total clock cycles: " << (double)clock_cycle/1e6 << "M" << endl;
    out << "  avgIPC = " << (double)pipeline->stats.uop_count / (double) clock_cycle << endl; }


void spx_core_t::handle_cache_response(int temp, cache_request_t *cache_request)
{
    pipeline->handle_cache_response(temp,cache_request);
}

void spx_core_t::send_qsim_proxy_request()
{
    if(!qsim_proxy_request_sent) { /* Do not send multiple requests */
        qsim_proxy_request_sent = true;
        qsim_proxy_request_t *qsim_proxy_request = new qsim_proxy_request_t(core_id, getComponentId());
#ifdef DEBUG_NEW_QSIM_1
        std::cerr << "( Core " << std::dec << core_id << " ) [send request to qsim] @ " << std::dec << clock_cycle << std::endl << std::flush;
#endif
        Send(OUT_TO_QSIM, qsim_proxy_request);
    }
}

void spx_core_t::handle_qsim_response(int temp, qsim_proxy_request_t *qsim_proxy_request)
{
    assert(qsim_proxy_request->get_core_id() == core_id);
    if (qsim_proxy_request->is_extended() == false) {
        assert(qsim_proxy_request_sent);
        qsim_proxy_request_sent = false;
    }

    /* qsim_proxy_request is deleted here */
    qsim_proxy->handle_qsim_response(qsim_proxy_request);
}

#endif // USE_QSIM
