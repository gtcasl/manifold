#ifndef __QSIM_PROXY_H__
#define __QSIM_PROXY_H__

#include <assert.h>
#include "qsim.h"
#include "kernel/component.h"
#include "kernel/clock.h"

#define QSIM_RUN_GRANULARITY   200
#define QSIM_OVERHEAD   0.2
#define QSIM_PROXY_QUEUE_SIZE (int)(5 * QSIM_RUN_GRANULARITY * (1 + QSIM_OVERHEAD))

//#define DEBUG_NEW_QSIM 1 
//#define DEBUG_NEW_QSIM_1 1 
//#define DEBUG_NEW_QSIM_2 1 

namespace manifold {
namespace qsim_proxy {

class qsim_proxy_t : public manifold::kernel::Component
{
public:
    qsim_proxy_t(char *StateName, char *AppName, uint64_t InterruptInterval);
    ~qsim_proxy_t();

    /* Tick function periodically invokes Qsim timer interrupt */
    void tick();
    void tock();

    /* Qsim instruction callbacks */
    void inst_cb(int core_id, uint64_t vaddr, uint64_t paddr, uint8_t len, const uint8_t *bytes, enum inst_type type);
    void mem_cb(int core_id, uint64_t vaddr, uint64_t paddr, uint8_t size, int type);
    void reg_cb(int core_id, int reg, uint8_t size, int type);

    /* Function that processes fetch requests from cores */
    template <typename T> void handle_core_request(int temp, T *CoreRequest);
    
private:
    Qsim::OSDomain *qsim_osd;
    std::vector<Qsim::QueueItem> buffer;
    uint64_t interrupt_interval;
};

template <typename T>
void qsim_proxy_t::handle_core_request(int temp, T *CoreRequest)
{
    int core_id = CoreRequest->get_core_id();

    /* x86 instruction length is variable.
       Utilize only a fraction of queue space to avoid possible overflow. */
    //while(CoreRequest->get_queue_size() < QSIM_PROXY_QUEUE_SIZE*0.8) {
    do {
        //buffer.clear();
        // run will execute at least a translation-block in qim-0.2
        int rc = qsim_osd->run(core_id, QSIM_RUN_GRANULARITY);
#if  defined(DEBUG_NEW_QSIM_2) || defined(DEBUG_NEW_QSIM_1)
        std::cerr << "( core: " << std::dec << core_id << " ) | inst " << std::dec << rc << " tid "  << qsim_osd->get_tid(core_id) << " idle " << qsim_osd->idle(core_id) << " buffer " << buffer.size() << std::endl << std::flush;
#endif
 
        if(!rc) {
            Qsim::QueueItem queue_item;
            assert(buffer.size() == 0);
            if(!qsim_osd->booted(core_id)) {
                queue_item.cb_type = Qsim::QueueItem::TERMINATED;
                queue_item.id = core_id;

                CoreRequest->push_back(queue_item);
            }
            else {
                //assert(qsim_osd->idle(core_id));
                //assert(qsim_osd.get_tid(i) != qsim_osd.get_bench_pid());
                queue_item.cb_type = Qsim::QueueItem::IDLE;
                queue_item.id = core_id;

                for(int i = 0; i < QSIM_RUN_GRANULARITY; i++) {
                    CoreRequest->push_back(queue_item);
                }
            }
            break;
        }
        else {
/*            while(buffer.size()) {*/
                //CoreRequest->push_back(*(buffer.begin()));
                //buffer.erase(buffer.begin());
            //}
            
            if (qsim_osd->idle(core_id) || (buffer.size() == 0)) {
                //assert(buffer.size() == 0);
                Qsim::QueueItem queue_item;
                queue_item.cb_type = Qsim::QueueItem::IDLE;
                queue_item.id = core_id;

                for(int i = 0; i < QSIM_RUN_GRANULARITY; i++) {
                    CoreRequest->push_back(queue_item);
                }
                break;
            } else {
#ifdef DEBUG_NEW_QSIM
                std::cerr << "( core: " << std::dec << core_id << " ) cbs: " << buffer.size() << " | " << std::flush;
#endif
                
                std::vector<Qsim::QueueItem>::size_type sz = buffer.size();
                std::vector<Qsim::QueueItem>::size_type offset = 0;
                while ( sz > QSIM_PROXY_QUEUE_SIZE ) {
                    T *req = new T(CoreRequest->get_core_id(), CoreRequest->get_port_id(), true);
                    std::vector<Qsim::QueueItem>::iterator it = buffer.begin() + offset;

                    for(int i = 0; i < QSIM_PROXY_QUEUE_SIZE; i++) {
                        Qsim::QueueItem qi = *it;
                        req->push_back(qi);
                        it++;
                    }

#ifdef DEBUG_NEW_QSIM_1
                    std::cerr << "( Core " << std::dec << req->get_core_id() << " ) [receive request from qsim*] | " << std::dec << req->get_queue_size() << std::endl << std::flush;
#endif

                    Send(req->get_port_id(), req);
                    
                    sz -= QSIM_PROXY_QUEUE_SIZE;
                    offset += QSIM_PROXY_QUEUE_SIZE;
                }

                //assert( buffer.size() < QSIM_PROXY_QUEUE_SIZE );
                for(std::vector<Qsim::QueueItem>::iterator it = buffer.begin() + offset; it != buffer.end(); it++) {
                    Qsim::QueueItem qi = *it;
                    CoreRequest->push_back(qi);
                }

#ifdef DEBUG_NEW_QSIM
                std::cerr << CoreRequest->get_queue_size() << std::endl << std::flush;
#endif
            }
        }
    } while(0);

    buffer.clear();
#ifdef DEBUG_NEW_QSIM_1
    std::cerr << "( Core " << std::dec << CoreRequest->get_core_id() << " ) [receive request from qsim] | " << std::dec << CoreRequest->get_queue_size() << std::endl << std::flush;
#endif
    Send(CoreRequest->get_port_id(), CoreRequest);
}

} // namespace qsim_proxy
} // namespace manifold

#endif
