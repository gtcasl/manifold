#include <stdio.h>
#include <assert.h>
#include "qsim-proxy.h"
#include "core.h"

using namespace manifold;
using namespace spx;
using namespace Qsim;

spx_qsim_proxy_t::spx_qsim_proxy_t(pipeline_t *Pipeline) :
    pipeline(Pipeline)
{
    /* Reserve fixed queue length. */
    queue.reserve(QSIM_PROXY_QUEUE_SIZE);
    queue.clear();
}

spx_qsim_proxy_t::~spx_qsim_proxy_t()
{
    queue.clear();
}

void spx_qsim_proxy_t::handle_qsim_response(qsim_proxy_request_t *QsimProxyRequest)
{
#ifdef DEBUG_NEW_QSIM_1
    std::cerr << "******************" << std::endl << std::flush;
    std::cerr << std::dec << pipeline->core->core_id << " recv: " << QsimProxyRequest->get_queue_size() << " @ " << pipeline->core->clock_cycle << std::endl << std::flush;
    std::vector<Qsim::QueueItem>::size_type sz = queue.size();
    //QsimProxyRequest->dump_queue();
#endif
    QsimProxyRequest->append_to(queue);
#ifdef DEBUG_NEW_QSIM_1
    std::cerr << "copied: " << queue.size() - sz << std::endl << std::flush;
#endif
    delete QsimProxyRequest;
#ifdef DEBUG_NEW_QSIM_1
    std::cerr << "remain: " << queue.size() << std::endl << std::flush;
    std::cerr << "******************" << std::endl << std::flush;
#endif
}

int spx_qsim_proxy_t::run(int CoreID, unsigned InstCount)
{
    unsigned inst_count = InstCount;
    std::vector<QueueItem>::iterator it;


    for(it = queue.begin(); it != queue.end(); it++) {
        QueueItem queue_item = *it;

        assert (CoreID == queue_item.id);

        if(queue_item.cb_type == QueueItem::INST) {
            if(inst_count == 0) { break; } /* loop exit */
            pipeline->Qsim_osd_state = QSIM_OSD_ACTIVE; /* Qsim core is active */

#ifdef DEBUG_NEW_QSIM
    std::cerr << "*( core " << std::dec << queue_item.id << " ): INST" << " | v: 0x" << std::hex << queue_item.data.inst.vaddr <<" p: 0x" << std::hex << queue_item.data.inst.paddr << std::endl << std::flush;
#endif
            pipeline->Qsim_inst_cb(CoreID,
                                   queue_item.data.inst.vaddr,
                                   queue_item.data.inst.paddr,
                                   queue_item.data.inst.len,
                                   (const uint8_t*)&queue_item.data.inst.bytes,
                                   (enum inst_type)queue_item.data.inst.type);
            inst_count--;
        }
        else if(queue_item.cb_type == QueueItem::MEM) {
            pipeline->Qsim_osd_state = QSIM_OSD_ACTIVE; /* Qsim core is active */

#ifdef DEBUG_NEW_QSIM
    std::cerr << "*( core " << std::dec << queue_item.id << " ): MEM" << " | v: 0x" << std::hex << queue_item.data.mem.vaddr <<" p: 0x" << std::hex << queue_item.data.mem.paddr << (queue_item.data.mem.type == 0 ? " RD" : " WR") << std::endl << std::flush;
#endif

            pipeline->Qsim_mem_cb(CoreID,
                                  queue_item.data.mem.vaddr,
                                  queue_item.data.mem.paddr,
                                  queue_item.data.mem.size,
                                  queue_item.data.mem.type);
        }
        else if(queue_item.cb_type == QueueItem::REG) {
            pipeline->Qsim_osd_state = QSIM_OSD_ACTIVE; /* Qsim core is active */
#ifdef DEBUG_NEW_QSIM
    std::cerr << "*( core " << std::dec << queue_item.id << " ): REG" << " | regid: " << std::dec << queue_item.data.reg.reg <<"  size: " << std::dec << static_cast<uint16_t>(queue_item.data.reg.size) << (queue_item.data.reg.type == 0 ? " SRC" : " DST") << std::endl << std::flush;
#endif

            pipeline->Qsim_reg_cb(CoreID,
                                  queue_item.data.reg.reg,
                                  queue_item.data.reg.size,
                                  queue_item.data.reg.type);
        }
        else if(queue_item.cb_type == QueueItem::IDLE) {
            if(inst_count != InstCount) { break; } /* loop exit */
#ifdef DEBUG_NEW_QSIM
    std::cerr << "*( core " << std::dec << queue_item.id << " ): IDLE" << std::endl << std::flush;
#endif
            pipeline->Qsim_osd_state = QSIM_OSD_IDLE;
            it++;
            break;
        }
        else if(queue_item.cb_type == QueueItem::TERMINATED) {
            if(inst_count != InstCount) { break; } /* loop exit */
#ifdef DEBUG_NEW_QSIM
    std::cerr << "*( core " << std::dec << queue_item.id << " ): TERMINATED" << std::endl << std::flush;
#endif
            pipeline->Qsim_osd_state = QSIM_OSD_TERMINATED;
            it++;
            break;
        }
        else { /* Unknown callback type. Something's wrong. */
            fprintf(stdout,"SPX core%d received unknown type of Qsim QueueItem.\n\
                            Aborting simulation ...\n", CoreID);
            exit(EXIT_FAILURE);
        }

    }

    queue.erase(queue.begin(), it);

    //if(queue.size() < SPX_QSIM_PROXY_QUEUE_SIZE*0.2) {
    if(queue.size() < 5) {
        pipeline->core->send_qsim_proxy_request();
    }

    return (InstCount-inst_count); /* Return the number of processed instructions. */
}
