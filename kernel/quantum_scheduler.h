#ifndef MANIFOLD_KERNEL_QUANTUM_SCHEDULER_H
#define MANIFOLD_KERNEL_QUANTUM_SCHEDULER_H

#include "scheduler.h"

#include <list>

namespace manifold {
namespace kernel {



//####################################################################
//  Quantum schedulers
//####################################################################

class Quantum_Scheduler : public Scheduler {
public:
    Quantum_Scheduler();

    //! give quantum an initial value
    void init_quantum(Ticks_t qtm)
    {
        m_init_quantum = qtm;
	m_quantum_inited = true;
    }

    void print_stats(std::ostream&);

    void Run();
    bool isTimed() { return false; }

#ifdef KERNEL_UTEST
public:
#else
private:
#endif
    bool m_quantum_inited; //is quantum value initialized

#ifdef KERNEL_UTEST
public:
#else
protected:
#endif
    Ticks_t m_init_quantum; //init quantum value

    enum {MSG_NOTIFY_EXIT, // rank i (i != 0) notifies rank 0 it's exited main loop
          MSG_EXIT, //rank 0 to others: exit the main loop
	  MSG_END, //rank 0 to others: you can terminate now
	  MSG_BARRIER_ENTER, // rank i (i != 0) to rank0: i have entered barrier
	  MSG_BARRIER_PROCEED // rank 0 to others: you can get out of barrier now
	  };
    bool m_end;
    int m_num_exited; //num of nodes that have exited main loop
    void quantum_handle_incoming_messages();
    void quantum_recv_incoming_msg_after_mainloop();
    void handle_proto1(const Message_s& msg);
    void handle_proto1_for_termination(const Message_s& msg);
    //! called after main loop to properly terminate the program
    void post_main_loop();

    int m_barrier_count; //num of ranks that have entered barrier
    bool m_barrier; //true if I'm in the barrier
    std::list<Message_s> m_pending_msg_list;
    void processPendingMsg();
    void enterBarrier();

    //stats
    unsigned stats_num_exited; //number of NOTIFY_EXIT messages received
    unsigned stats_num_exit; //number of EXIT message received
    unsigned stats_num_end; //number of END message received
    unsigned stats_num_timestamp_violation;
    unsigned stats_max_num_pending_msgs;
    unsigned stats_total_pending_msgs; //to compute average # of pending msgs
    unsigned stats_num_barrier;

};


} //kernel
} //manifold

#endif // MANIFOLD_KERNEL_QUANTUM_SCHEDULER_H
