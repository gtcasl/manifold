#ifndef MANIFOLD_KERNEL_SCHEDULER_H
#define MANIFOLD_KERNEL_SCHEDULER_H

#include <set>
#include <vector>

#include "manifold-event.h"
#include "stat_engine.h"
#include "stat.h"
#include "lookahead.h"
#include "syncalg.h"
#include "message.h"

namespace manifold {
namespace kernel {


class event_less
{
public:
  event_less() { }
  inline bool operator()(EventBase* const & l, const EventBase* const & r) const {
    if(l->time < r->time) return true;
    if (l->time == r->time) return l->uid < r->uid;
    return false;
  }
};

class Scheduler_stat_engine;




class Scheduler {
public:
    Scheduler();
    virtual ~Scheduler();

    virtual void Run() = 0;
    void Stop() { m_halted = true; }
    void terminate();

    virtual bool isTimed() = 0;
    void scheduleTimedEvent(EventBase*);
    bool cancelTimedEvent(EventId&);
    EventId peek(); //return ID of 1st timed event.
    EventBase* GetEarliestEvent(); //return 1st timed event
    Time_t get_simTime() { return m_simTime; }

    virtual void print_stats(std::ostream&);


#ifndef NO_MPI
    std::vector<LpId_t>& get_predecessors() { return predecessors; }
    std::vector<LpId_t>& get_successors() { return successors; }
    const SyncAlg* const get_syncAlg() { return m_syncAlg; }
#endif
    /**
     * Takes the given LP and appends it to the list of neighbors that might send message
     * to Manifold::GetRank().
     * @param lp An LP that might send messages to Manifold::GetRank()
     */
    void AddPredecessorLp(const LpId_t lp);

    /**
     * Takes the given LP and appends it to the list of neighbors that Manifold::GetRank()
     * might send messages to.
     * @param lp An LP that Manifold::GetRank() might send messages to
     */
    void AddSuccessorLp(const LpId_t lp);

#ifndef NO_MPI
    /**
     * Updates the static lookahead. Currently we support only global lookahead.
     * Pairwise lookahead is planned (therefore we added the parameters src and dst).
     * @param delay The lookahead will be decreased down to delay if not lower.
     * @param src For pairwise lookahead: source LP
     * @param dst For pairwise lookahead: destination LP
     */
    void UpdateLookahead(Time_t delay, LpId_t src=-1, LpId_t dst=-1);

    //! Allows a component to call this to specify when it will send the next output.
    //! This info can be used by the sync algorithm to optimize performance.
    void updateOutputTick(LpId_t src, LpId_t dst, Ticks_t when, Clock* c);

void print_lookahead() {m_syncAlg->print_lookahead(); }
#endif

#ifdef KERNEL_UTEST
public:
#else
protected:
#endif
    #ifndef NO_MPI
    void handle_incoming_messages();
    #endif

    bool m_halted;
    Time_t m_simTime; //current simulation time
    bool m_terminate_initiated; //termination has been initiated by this LP.


    typedef std::set<EventBase*, event_less> EventSet_t;

    #ifndef NO_MPI
    SyncAlg* m_syncAlg;
    #endif
    EventSet_t m_timedEvents;

private:
    #ifndef NO_MPI
    // Lists of neighboring LPs, and version number for fast update detection:
    std::vector<LpId_t> predecessors;
    std::vector<LpId_t> successors;
    //unsigned int m_neighborVersion;
    #endif


    Scheduler_stat_engine* stats;

};






class Scheduler_stat_engine : public Stat_engine
{
public:
    Scheduler_stat_engine();
    ~Scheduler_stat_engine();

    void global_stat_merge(Stat_engine *);
    void print_stats(std::ostream & out);
    void clear_stats();

    void start_warmup();
    void end_warmup();
    void save_samples();
};




//####################################################################
//  sequential schedulers
//####################################################################
class Seq_TickedScheduler : public Scheduler {
public:
    Seq_TickedScheduler() {}
    ~Seq_TickedScheduler() {}

    void Run();
    bool isTimed() { return false; }
};



class Seq_TimedScheduler : public Scheduler {
public:
    Seq_TimedScheduler() {}
    ~Seq_TimedScheduler() {}
    void Run();
    bool isTimed() { return true; }
};



class Seq_MixedScheduler : public Scheduler {
public:
    Seq_MixedScheduler() {}
    ~Seq_MixedScheduler() {}
    void Run();
    bool isTimed() { return true; }
};






#ifndef NO_MPI
//####################################################################
//  LBTS schedulers
//####################################################################
//! When all the events in the system are clock tick based, this
//! scheduler should be used.
class LBTS_TickedScheduler : public Scheduler {
public:
    LBTS_TickedScheduler(Lookahead::LookaheadType_t lookaheadType);
    ~LBTS_TickedScheduler() {}

    void Run();
    bool isTimed() { return false; }
    //?????????????????????????????????? should clocks be member of the scheduler?

};




//! When none of the events in the system are clock tick based, this
//! scheduler should be used.
class LBTS_TimedScheduler : public Scheduler {
public:
    LBTS_TimedScheduler(Lookahead::LookaheadType_t lookaheadType);
    ~LBTS_TimedScheduler() {}
    void Run();
    bool isTimed() { return true; }
};




//! When events in the system are a mixture of tick based and non-tick-based,
//! This scheduler should be used.
class LBTS_MixedScheduler : public Scheduler {
public:
    LBTS_MixedScheduler(Lookahead::LookaheadType_t lookaheadType);
    ~LBTS_MixedScheduler() {}
    void Run();
    bool isTimed() { return true; }
};






//####################################################################
//  CMB schedulers
//####################################################################

//This class contains variables and functions that are common to all its
//subclasses
class CMB_Scheduler : public Scheduler {
public:
    CMB_Scheduler();
    void print_stats(std::ostream&);

protected:
    enum {MSG_FINI, MSG_STOP, MSG_END};
    bool m_end;
    int m_num_fini; //num of nodes that have exited main loop
    void CMB_handle_incoming_messages();
    void CMB_recv_incoming_messages();
    void handle_proto1(Message_s& msg);

    //! called after main loop to properly terminate the program
    void post_main_loop();

    //stats
    unsigned stats_num_fini; //number of FINI messages
    unsigned stats_num_stop; //number of STOP messages
    unsigned stats_num_end; //number of END messages
    unsigned long stats_blocking_time; //time (usec) in blocking state
};


class CMB_TickedScheduler : public CMB_Scheduler {
public:
    CMB_TickedScheduler(Lookahead::LookaheadType_t lookaheadType);
    ~CMB_TickedScheduler() {}

    void Run();
    bool isTimed() { return false; }
};


class CMB_TimedScheduler : public CMB_Scheduler {
public:
    CMB_TimedScheduler(Lookahead::LookaheadType_t lookaheadType);
    ~CMB_TimedScheduler() {}
    void Run();
    bool isTimed() { return true; }
};



class CMB_MixedScheduler : public CMB_Scheduler {
public:
    CMB_MixedScheduler(Lookahead::LookaheadType_t lookaheadType);
    ~CMB_MixedScheduler() {}
    void Run();
    bool isTimed() { return true; }
};


class CMB_TickedOptScheduler : public CMB_Scheduler {
public:
    CMB_TickedOptScheduler(Lookahead::LookaheadType_t lookaheadType);
    ~CMB_TickedOptScheduler() {}

    void Run();
    bool isTimed() { return false; }
};


class CMB_TickedForecastScheduler : public CMB_Scheduler {
public:
    CMB_TickedForecastScheduler(Lookahead::LookaheadType_t lookaheadType);
    ~CMB_TickedForecastScheduler() {}

    void Run();
    bool isTimed() { return false; }
};

#endif //#ifndef NO_MPI




} //kernel
} //manifold

#endif // MANIFOLD_KERNEL_SCHEDULER_H
