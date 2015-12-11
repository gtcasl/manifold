#ifndef __QSIM_PROXY_H__
#define __QSIM_PROXY_H__

#include <assert.h>
#include "qsim.h"
#include "kernel/component.h"
#include "kernel/clock.h"

#define QSIM_PROXY_QUEUE_SIZE 256

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
    while(CoreRequest->get_queue_size() < QSIM_PROXY_QUEUE_SIZE*0.8) {
        buffer.clear();
        int rc = qsim_osd->run(core_id, 1);

        if(!rc) {
            Qsim::QueueItem queue_item;
            if(!qsim_osd->booted(core_id)) {
                queue_item.cb_type = Qsim::QueueItem::TERMINATED;
            }
            else {
                assert(qsim_osd->idle(core_id));
                queue_item.cb_type = Qsim::QueueItem::IDLE;
            }
            CoreRequest->push_back(queue_item);
        }
        else {
            while(buffer.size()) {
                CoreRequest->push_back(*(buffer.begin()));
                buffer.erase(buffer.begin());
            }
        }
    }

    Send(CoreRequest->get_port_id(), CoreRequest);
}

} // namespace qsim_proxy
} // namespace manifold

#endif

