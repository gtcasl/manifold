/** @file syncalg.h
 * Contains classes and subclasses to perform synchronization
 * in parallel and distributed simulation.
 */

// Mirko Stoffers, (and others) Georgia Tech, 2012
#ifndef NO_MPI

#pragma once

#include <tr1/unordered_map>

#include "common-defs.h"
#include "messenger.h"
#include "lookahead.h"

namespace manifold {
namespace kernel {

//! @class OutputTS syncalg.h
//! @brief Used by components to inform synchronization algorithm about the time-stamp of future
//! output events. This is used to improve simulation efficiency.
struct OutputTS {
    OutputTS(unsigned sz) : m_times(sz), m_ticks(sz), m_prev_ticks(sz) {}
    std::vector<std::map<int, Time_t> > m_times;
    std::vector<std::map<int, Ticks_t> > m_ticks;
    std::vector<std::map<int, Ticks_t> > m_prev_ticks;
};



class SyncAlg {
public:
    enum SyncAlgType_t {
        SA_CMB,
        SA_CMB_OPT_TICK,
        SA_CMB_TICK_FORECAST,
        SA_LBTS,
	SA_QUANTUM
    };

    SyncAlg(Lookahead::LookaheadType_t laType);
    virtual ~SyncAlg() {}

    //! Print statistical data.
    virtual void PrintStats(std::ostream& out) = 0;

    //! Updates lookahead.
    //! @param delay The lookahead will be decreased down to delay if not lower.
    //! @param src For pairwise lookahead: source LP
    //! @param dst For pairwise lookahead: destination LP
    void UpdateLookahead(Time_t delay, LpId_t src = -1, LpId_t dst = -1);

    void updateOutputTick(LpId_t src, LpId_t dst, Ticks_t when, Clock* clk);

    //! Called by scheduler when termination is initiated.
    virtual void terminateInitiated() {}

    void print_lookahead()
    {
        m_lookahead->print();
    }

protected:
    Lookahead* m_lookahead;
    OutputTS* m_outputTS;
};



class LbtsSyncAlg: public SyncAlg {
public:
    LbtsSyncAlg(Lookahead::LookaheadType_t laType);

    //! checks whether it is safe to process and event at the given
    //! timestamp requestTime.
    //! @param requestTime the time of the next event.
    //! @return true iff it is safe to process the event
    bool isSafeToProcess(double requestTime);

    void terminateInitiated();

    //! Print statistical data.
    virtual void PrintStats(std::ostream& out);

#ifdef KERNEL_UTEST
public:
#else
private:
#endif
    struct LBTS_Msg {
        int epoch;
        int tx_count;
        int rx_count;
        int myId;
        Time_t smallest_time;
    };

    Time_t m_grantedTime;
    unsigned long m_stats_LBTS_sync;
};



class CmbSyncAlg: public SyncAlg {
public:
    CmbSyncAlg(Lookahead::LookaheadType_t laType);

    //! checks whether it is safe to process and event at the given
    //! timestamp requestTime.
    //! @param requestTime the time of the next event.
    //! @return true iff it is safe to process the event
    bool isSafeToProcess(double requestTime);

    void send_null_msgs();
    bool send_null_msgs(Clock*);
    void send_null_msgs_with_forecast(Clock* clk);
    void send_null_msgs_with_forecast_2(Clock* clk);

    bool isSafeToProcess_send_null(double requestTime);

    bool isSafeToProcess_send_null_if_safe(double requestTime);

    //! Print statistical data.
    virtual void PrintStats(std::ostream& out);

#ifdef KERNEL_UTEST
public:
#else
private:
#endif
    bool m_initialized;

    typedef std::tr1::unordered_map<LpId_t, Time_t> ts_t;
    ts_t m_eits;
    ts_t m_eots;
    std::tr1::unordered_map<LpId_t, Ticks_t> m_in_forecast_ticks;

    unsigned int m_neighborVersion;
    //Time_t m_earliestLocalEvent;
    Time_t m_min_null; //min of input null messages

    void UpdateEitSet();
    void UpdateEotSet();

    //stats
    std::vector<unsigned> stats_received_null;
    std::vector<unsigned> stats_sent_null;

    double null_msg_send_time;
    double null_msg_recv_time;
    double null_msg_wasted_send_time;
    double null_msg_wasted_recv_time;
};


//! For Quantum_Scheduler, but most functionality is in Quantum_Scheduler.
class QtmSyncAlg: public SyncAlg {
public:
    QtmSyncAlg() : SyncAlg(Lookahead::LA_GLOBAL) {}
    void PrintStats(std::ostream& out) {}
};



} //kernel
} //manifold

#endif //!NO_MPI
