#ifdef USE_QSIM

#ifndef __SPX_CORE_H__
#define __SPX_CORE_H__

#include <mpi.h>
#include "qsim.h"
#include "kernel/component.h"
#include "kernel/clock.h"
#include "pipeline.h"
#include "qsim-proxy.h"

//#define DEBUG_NEW_QSIM_1 1

namespace manifold {
namespace spx {

enum QSIM_OSD_STATE { 
    QSIM_OSD_TERMINATED = 0,
    QSIM_OSD_IDLE,
    QSIM_OSD_ACTIVE
};

class spx_core_t : public manifold::kernel::Component
{
public:
    spx_core_t(const int nodeID, const char *configFileName, const int coreID);
    ~spx_core_t();

    enum { IN_FROM_DATA_CACHE = 0, IN_FROM_INST_CACHE = 1, IN_FROM_QSIM = 2, IN_FROM_KITFOX = 3 };
    enum { OUT_TO_DATA_CACHE = 0, OUT_TO_INST_CACHE = 1, OUT_TO_QSIM = 2, OUT_TO_KITFOX  = 3 };

    // manifold component functions
    void tick();
    int get_qsim_osd_state() const;
    void handle_cache_response(int temp, cache_request_t *cache_request);
    void handle_qsim_response(int temp, qsim_proxy_request_t *qsim_proxy_request);
    void print_stats(uint64_t sampling_period, FILE *LogFile = stdout);
    void reset_interval_stats();
    void turn_on();
    void turn_off();

    void send_qsim_proxy_request();

    int node_id; // manifold node ID
    int core_id; // core ID
    uint64_t clock_cycle;
    bool active;
    
private:
    pipeline_t *pipeline; // base class of pipeline models
    spx_qsim_proxy_t *qsim_proxy;
    bool qsim_proxy_request_sent;
};

} // namespace spx
} // namespace manifold

#endif

#endif // USE_QSIM
