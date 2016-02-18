#ifndef __SPX_QSIM_PROXY_H__
#define __SPX_QSIM_PROXY_H__

#include "qsim.h"
#include "qsim-regs.h"
#include "pipeline.h"

/* Refer to ../../qsim/proxy/proxy.h:QSIM_PROXY_QUEUE_SIZE */
#define SPX_QSIM_PROXY_QUEUE_SIZE 256

//#define DEBUG_NEW_QSIM 1
//#define DEBUG_NEW_QSIM_1 1

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
    void push_back(Qsim::QueueItem queue_item) { queue.push_back(queue_item); }
    void reset() { num_queue_items = 0; }
    unsigned get_queue_size() const { return queue.size(); }
    void append_to (std::vector<Qsim::QueueItem> &q) { 
#ifdef DEBUG_NEW_QSIM
        std::cerr << "in append_to -> q: " << q.size() << " queue: " << queue.size() << std::endl << std::flush; 
#endif
        q.insert(q.end(), queue.begin(), queue.end()); 
#ifdef DEBUG_NEW_QSIM
        std::cerr << "in append_to[af] -> q: " << q.size() << " queue: " << queue.size() << std::endl << std::flush; 
#endif
    }
    void dump_queue () {
        for(std::vector<Qsim::QueueItem>::iterator it = queue.begin(); it != queue.end(); it++) {
           Qsim::QueueItem qi = *it;
           switch (qi.cb_type) {
               case Qsim::QueueItem::INST:
                   std::cerr << "(core " << std::dec << qi.id << " ) | INST" << std::endl << std::flush;
                   break;
               case Qsim::QueueItem::MEM:
                   std::cerr << "(core " << std::dec << qi.id << " ) | MEM" << std::endl << std::flush;
                   break;
               case Qsim::QueueItem::REG:
                   std::cerr << "(core " << std::dec << qi.id << " ) | REG" << std::endl << std::flush;
                   break;
               case Qsim::QueueItem::IDLE:
                   std::cerr << "(core " << std::dec << qi.id << " ) | IDLE" << std::endl << std::flush;
                   break;
               case Qsim::QueueItem::TERMINATED:
                   std::cerr << "(core " << std::dec << qi.id << " ) | TERMINATED" << std::endl << std::flush;
                   break;
                default:
                   std::cerr << " (core " << std::dec << qi.id << ") | Wrong Type!" << std::endl << std::flush;
           } 
        } 
    }

private:
    int core_id;
    int port_id;
    std::vector<Qsim::QueueItem> queue;
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
