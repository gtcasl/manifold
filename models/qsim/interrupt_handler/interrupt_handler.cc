#include "interrupt_handler.h"

using namespace std;
using namespace manifold;
using namespace manifold::kernel;
using namespace manifold::qsim_interrupt_handler;

interrupt_handler_t::interrupt_handler_t(Qsim::OSDomain *osd, manifold::kernel::Clock* clk) :
    Qsim_osd(osd),
    clock_cycle(0),
    interval(1)
{
   clock = clk;
   Clock :: Register(*clock, this, &interrupt_handler_t::tick, (void(interrupt_handler_t::*)(void))0);
}

interrupt_handler_t::~interrupt_handler_t()
{
}

void interrupt_handler_t::set_interval(uint64_t Interval)
{
    interval = Interval;   
}

void interrupt_handler_t::tick()
{
    clock_cycle = clock->NowTicks();
    if((clock_cycle % interval == 0) && (clock_cycle > 0)) { 
        Qsim_osd->timer_interrupt();
        cout << "Qsim Interrupt Handler: " << clock_cycle << endl; 
    }
}

