#ifndef __QSIM_INTERRUPT_HANDLER_H__
#define __QSIM_INTERRUPT_HANDLER_H__

#include "qsim.h"
#include "kernel/component.h"
#include "kernel/clock.h"

namespace manifold {
namespace qsim_interrupt_handler {

class interrupt_handler_t : public manifold::kernel::Component
{
public:
    interrupt_handler_t(Qsim::OSDomain *osd, manifold::kernel::Clock* clk);
    ~interrupt_handler_t();

    void tick();
    void set_interval(uint64_t Interval);
    
private:
    Qsim::OSDomain *Qsim_osd;
    manifold::kernel::Clock* clock;
    uint64_t clock_cycle;
    uint64_t interval;
};

} // namespace qsim_interrupt_handler
} // namespace manifold

#endif

