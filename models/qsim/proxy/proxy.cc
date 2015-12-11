#include "proxy.h"
#include "qsim.h"
#include "qsim-load.h"

using namespace std;
using namespace Qsim;
using namespace manifold;
using namespace manifold::kernel;
using namespace manifold::qsim_proxy;

qsim_proxy_t::qsim_proxy_t(char *StateName, char *AppName, uint64_t InterruptInterval) :
    qsim_osd(NULL),
    interrupt_interval(InterruptInterval)
{
    cout << "Loading Qsim state file ..." << endl;
    qsim_osd = new Qsim::OSDomain(StateName);

    cout << "Loading application ..." << endl;
    load_file(*qsim_osd, AppName);

    cout << "Finished initializing Qsim" << endl;

    /* Set Qsim callback functions */
    qsim_osd->set_inst_cb(this, &qsim_proxy_t::inst_cb);
    qsim_osd->set_mem_cb(this, &qsim_proxy_t::mem_cb);
    qsim_osd->set_reg_cb(this, &qsim_proxy_t::reg_cb);

    buffer.reserve(128);
}


qsim_proxy_t::~qsim_proxy_t()
{
    cout << "Terminating Qsim" << endl;
    delete qsim_osd;
}


void qsim_proxy_t::tick()
{
    uint64_t clock_cycle = m_clk->NowTicks();

    /* Invoke Qsim timer interrupt */
    if(clock_cycle % interrupt_interval == 0) {
        qsim_osd->timer_interrupt();
    }
}


void qsim_proxy_t::tock()
{
    /* Nothing to do */
}


void qsim_proxy_t::inst_cb(int core_id, uint64_t vaddr, uint64_t paddr, uint8_t len, const uint8_t *bytes, enum inst_type type)
{
    buffer.push_back(QueueItem(vaddr, paddr, len, bytes, type));
}


void qsim_proxy_t::mem_cb(int core_id, uint64_t vaddr, uint64_t paddr, uint8_t size, int type)
{
    buffer.push_back(QueueItem(vaddr, paddr, size, type));
}


void qsim_proxy_t::reg_cb(int core_id, int reg, uint8_t size, int type)
{
    buffer.push_back(QueueItem(reg, size, type));
}

