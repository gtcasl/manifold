#ifndef __SPX_QSIM_PROXY_H__
#define __SPX_QSIM_PROXY_H__

#include "qsim.h"
#include "qsim-regs.h"
#include "pipeline.h"

/* Refer to ../../qsim/proxy/proxy.h:QSIM_PROXY_QUEUE_SIZE */
#define SPX_QSIM_PROXY_QUEUE_SIZE 256

namespace manifold {
namespace spx {

class qsim_proxy_request_t
{
public:
    qsim_proxy_request_t() {}
    qsim_proxy_request_t(int CoreID, int PortID) : core_id(CoreID),
                                                   port_id(PortID),
                                                   num_queue_items(0) {}
    ~qsim_proxy_request_t() {}

    int get_core_id() const { return core_id; }
    int get_port_id() const { return port_id; }
    Qsim::QueueItem get_queue_item(unsigned idx) const { return queue[idx]; }
    void push_back(Qsim::QueueItem queue_item) { queue[num_queue_items++] = queue_item; }
    void reset() { num_queue_items = 0; }
    unsigned get_queue_size() const { return num_queue_items; }

private:
    int core_id;
    int port_id;
    Qsim::QueueItem queue[SPX_QSIM_PROXY_QUEUE_SIZE];
    unsigned num_queue_items;
};



class spx_qsim_proxy_t {
public:
    spx_qsim_proxy_t(pipeline_t *Pipeline);
    ~spx_qsim_proxy_t();

    int run(int CoreID, unsigned InstCounts);
    void handle_qsim_response(qsim_proxy_request_t *QsimProxyRequest);

private:
    pipeline_t *pipeline;
    std::vector<Qsim::QueueItem> queue;
};

} // namespace spx
} // namespace manifold

#endif
