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
    queue.reserve(SPX_QSIM_PROXY_QUEUE_SIZE);
}

spx_qsim_proxy_t::~spx_qsim_proxy_t()
{
    queue.clear();
}

void spx_qsim_proxy_t::handle_qsim_response(qsim_proxy_request_t *QsimProxyRequest)
{
    unsigned queue_index = 0;
    while(queue_index < QsimProxyRequest->get_queue_size()) {
        queue.push_back(QsimProxyRequest->get_queue_item(queue_index));
        queue_index++;
    }

    delete QsimProxyRequest;
}

int spx_qsim_proxy_t::run(int CoreID, unsigned InstCount) 
{
    unsigned inst_count = InstCount;

    while(queue.size()) {
        QueueItem queue_item = *queue.begin();

        if(queue_item.cb_type == QueueItem::INST_CB) {
            if(inst_count == 0) { break; } /* While loop exit */
            pipeline->Qsim_osd_state = QSIM_OSD_ACTIVE; /* Qsim core is active */
            pipeline->Qsim_inst_cb(CoreID,
                                   queue_item.data.inst_cb.vaddr,
                                   queue_item.data.inst_cb.paddr,
                                   queue_item.data.inst_cb.len,
                                   (const uint8_t*)&queue_item.data.inst_cb.bytes,
                                   (enum inst_type)queue_item.data.inst_cb.type);
            inst_count--;
        }
        else if(queue_item.cb_type == QueueItem::MEM_CB) {
            pipeline->Qsim_osd_state = QSIM_OSD_ACTIVE; /* Qsim core is active */
            pipeline->Qsim_mem_cb(CoreID,
                                  queue_item.data.mem_cb.vaddr,
                                  queue_item.data.mem_cb.paddr,
                                  queue_item.data.mem_cb.size,
                                  queue_item.data.mem_cb.type);
        }
        else if(queue_item.cb_type == QueueItem::REG_CB) {
            pipeline->Qsim_osd_state = QSIM_OSD_ACTIVE; /* Qsim core is active */
            pipeline->Qsim_reg_cb(CoreID,
                                  queue_item.data.reg_cb.reg,
                                  queue_item.data.reg_cb.size,
                                  queue_item.data.reg_cb.type);
        }
        else if(queue_item.cb_type == QueueItem::IDLE) {
            pipeline->Qsim_osd_state = QSIM_OSD_IDLE;
            break;
        }
        else if(queue_item.cb_type == QueueItem::TERMINATED) { 
            pipeline->Qsim_osd_state = QSIM_OSD_TERMINATED;
            break;
        }
        else { /* Unknown callback type. Something's wrong. */
            fprintf(stdout,"SPX core%d received unknown type of Qsim QueueItem.\n\
                            Aborting simulation ...\n", CoreID);
            exit(EXIT_FAILURE);
        }

        queue.erase(queue.begin()); /* Dequeue the head of QueueItem */
    }

    if(queue.size() < SPX_QSIM_PROXY_QUEUE_SIZE*0.2) {
        pipeline->core->send_qsim_proxy_request();
    }

    return (InstCount-inst_count); /* Return the number of processed instructions. */
}

