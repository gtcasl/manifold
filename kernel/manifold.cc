// Manifold object implementation
// George F. Riley, (and others) Georgia Tech, Spring 2010

#include <stdlib.h>
#include "common-defs.h"
#include "manifold.h"
#include "component.h"
#ifndef NO_MPI
#include "messenger.h"
#endif
#include "clock.h"

namespace manifold {
namespace kernel {

// Static variables
int        EventBase::nextUID = 0;
int        TickEventBase::nextUID = 0;
Scheduler* Manifold::TheScheduler = 0;

// TickEvent0Stat is not a template, so we implement it here
void TickEvent0Stat::CallHandler()
{
    handler();
}

// Event0Stat is not a template, so we implement it here
void Event0Stat::CallHandler()
{
  handler();
}

// The static Schedule with no args is not a template, so implement here
   TickEventId Manifold::Schedule(Ticks_t t, void(*handler)(void))
  {
    Clock& c = Clock::Master();
    TickEventBase* ev = new TickEvent0Stat(t, c, handler);
    c.Insert(ev);
    return TickEventId(Manifold::NowTicks() + t, ev->uid, c);
  }

// The static Schedule with no args is not a template, so implement here
   TickEventId Manifold::ScheduleHalf(Ticks_t t, void(*handler)(void))
  {
    Clock& c = Clock::Master();
    TickEventBase* ev = new TickEvent0Stat(t, c, handler);
    c.InsertHalf(ev);
    return TickEventId(ev->time, ev->uid, c);
  }

// The static Schedule with no args is not a template, so implement here
TickEventId Manifold::ScheduleClock(Ticks_t t, Clock& c, void(*handler)(void))
  {
    TickEventBase* ev = new TickEvent0Stat(t, c, handler);
    c.Insert(ev);
    return TickEventId(ev->time, ev->uid, c);
  }

// The static Schedule with no args is not a template, so implement here
TickEventId Manifold::ScheduleClockHalf(Ticks_t t, Clock& c, void(*handler)(void))
  {
    TickEventBase* ev = new TickEvent0Stat(t, c, handler);
    c.InsertHalf(ev);
    return TickEventId(ev->time, ev->uid, c);
  }

// The static ScheduleTime with no args is not a template, so implement here
   EventId Manifold::ScheduleTime(double t, void(*handler)(void))
  {
    double future = t + Now();
    EventBase* ev = new Event0Stat(future, handler);
    assert(TheScheduler->isTimed());
    //events.insert(ev);
    TheScheduler->scheduleTimedEvent(ev);
    return EventId(future, ev->uid);
  }


// Static methods
LpId_t Manifold::GetRank()
{
#ifndef NO_MPI
  return TheMessenger.get_node_id();
#else
  return 0;
#endif
}

#if 0
void Manifold::EnableDistributed()
{
  isDistributed = true;
}
#endif


//====================================================================
//====================================================================
void Manifold::Init(SchedulerType t)
{
  if(TheScheduler)
      return;  //scheduler already created

  switch(t) {
      case TICKED:
	  TheScheduler = new Seq_TickedScheduler();
	  break;

      case TIMED:
	  TheScheduler = new Seq_TimedScheduler();
	  break;

      case MIXED:
	  TheScheduler = new Seq_MixedScheduler();
	  break;
  }
}

#ifndef NO_MPI
//====================================================================
//====================================================================
void Manifold::Init(int argc, char** argv, SchedulerType t,SyncAlg::SyncAlgType_t syncAlgType, Lookahead::LookaheadType_t lookaheadType)
{
  assert(TheScheduler == 0);

  TheMessenger.init(argc, argv);

  if(TheMessenger.get_node_size() == 1) {
      switch(t) {
	  case TICKED:
	      TheScheduler = new Seq_TickedScheduler();
	      break;

	  case TIMED:
	      TheScheduler = new Seq_TimedScheduler();
	      break;

	  case MIXED:
	      TheScheduler = new Seq_MixedScheduler();
	      break;
      }
      return;
  }

  switch(t) {
      case TICKED:
          switch(syncAlgType) {
	      case SyncAlg :: SA_CMB:
		  TheScheduler = new CMB_TickedScheduler(lookaheadType);
		  break;
	      case SyncAlg :: SA_CMB_OPT_TICK:
		  TheScheduler = new CMB_TickedOptScheduler(lookaheadType);
		  break;
              #ifdef FORECAST_NULL
	      case SyncAlg :: SA_CMB_TICK_FORECAST:
		  TheScheduler = new CMB_TickedForecastScheduler(lookaheadType);
		  break;
	      #endif
	      case SyncAlg :: SA_LBTS:
		  TheScheduler = new LBTS_TickedScheduler(lookaheadType);
		  break;
	      case SyncAlg :: SA_QUANTUM:
		  TheScheduler = new Quantum_Scheduler();
		  break;
	      default:
	          std::cerr << "Invalid synchronization algorithm\n";
		  exit(1);
	  }
	  break;

      case TIMED:
          switch(syncAlgType) {
	      case SyncAlg :: SA_LBTS:
		  TheScheduler = new LBTS_TimedScheduler(lookaheadType);
		  break;
	      case SyncAlg :: SA_CMB:
		  TheScheduler = new CMB_TimedScheduler(lookaheadType);
		  break;
              default:
	          assert(0);
	  }
	  break;

      case MIXED:
          switch(syncAlgType) {
	      case SyncAlg :: SA_LBTS:
		  TheScheduler = new LBTS_MixedScheduler(lookaheadType);
		  break;
	      case SyncAlg :: SA_CMB:
		  TheScheduler = new CMB_MixedScheduler(lookaheadType);
		  break;
              default:
	          assert(0);
	  }
	  break;
  }
}
#endif


//====================================================================
//====================================================================
void Manifold::Finalize()
{
#ifndef NO_MPI
  TheMessenger.finalize();
#endif
}


//====================================================================
//====================================================================
void Manifold::Run()
{
    TheScheduler->Run();
}


//====================================================================
//Terminate the simulation. In a parallel simulation, this means all
//processes should terminate.
//====================================================================
void Manifold :: Terminate()
{
    vector<Clock*> clocks = Clock :: GetClocks();
    for(int i=0; i<clocks.size(); i++) {
        clocks[i]->terminate();
    }
    TheScheduler->terminate();
}


void Manifold::Stop()
{
    TheScheduler->Stop();
}


void Manifold::StopAtTime(Time_t s)
{
  ScheduleTime(s, &Manifold::Stop);
}

void Manifold::StopAt(Ticks_t t)
{
  Schedule(t, &Manifold::Stop);
}

void Manifold::StopAtClock(Ticks_t t, Clock& c)
{
  ScheduleClock(t, c, &Manifold::Stop);
}

double Manifold::Now()
{
  return TheScheduler->get_simTime();
}

Ticks_t Manifold::NowTicks()
{
  return Clock::Now();
}

Ticks_t Manifold::NowHalfTicks()
{
  return Clock::NowHalf();
}

Ticks_t Manifold::NowTicks(Clock& c)
{
  return Clock::Now(c);
}

Ticks_t Manifold::NowHalfTicks(Clock& c)
{
  return Clock::NowHalf(c);
}

bool Manifold::Cancel(EventId& evid)
{
  assert(TheScheduler->isTimed());
  return TheScheduler->cancelTimedEvent(evid);
}

EventId Manifold::Peek()
{
  return TheScheduler->peek();
}

EventBase* Manifold::GetEarliestEvent()
{
  return TheScheduler->GetEarliestEvent();
}



void Manifold :: print_stats(std::ostream& out)
{
    out << "=== Scheduler ===\n";
    TheScheduler->print_stats(out);
    Clock::ClockVec_t& clocks = Clock :: GetClocks();
    for(unsigned i=0; i<clocks.size(); i++) {
	out << "=== Clock " << i << " ===\n";
        clocks[i]->print_stats(out);
    }
}



//##############################################################################
// Below are functions for unit test
//##############################################################################
#ifdef KERNEL_UTEST

#ifdef NO_MPI
void Manifold :: Reset(SchedulerType t)
{
    //1. delete old scheduler and create a new one.
    delete TheScheduler;
    switch(t) {
	case TICKED:
	    TheScheduler = new Seq_TickedScheduler();
	    break;

	case TIMED:
	    TheScheduler = new Seq_TimedScheduler();
	    break;

	case MIXED:
	    TheScheduler = new Seq_MixedScheduler();
	    break;
    }

    //2. reset all clocks
    Clock::Reset();
}

#else

void Manifold::Reset(SchedulerType t, SyncAlg::SyncAlgType_t syncAlgType,
                            Lookahead::LookaheadType_t lookaheadType)
{
    //1. delete old scheduler and create a new one
    //2. reset all clocks
    //3. anything to be done with the messenger???
    delete TheScheduler;
    switch(t) {
	case TICKED:
	    switch(syncAlgType) {
		case SyncAlg :: SA_CMB:
		    TheScheduler = new CMB_TickedScheduler(lookaheadType);
		    break;
		case SyncAlg :: SA_CMB_OPT_TICK:
		    TheScheduler = new CMB_TickedOptScheduler(lookaheadType);
		    break;
		case SyncAlg :: SA_LBTS:
		    TheScheduler = new LBTS_TickedScheduler(lookaheadType);
		    break;
		case SyncAlg :: SA_QUANTUM:
		    TheScheduler = new Quantum_Scheduler();
		    break;
		default:
		    assert(0);
	    }
	    break;

	case TIMED:
	    switch(syncAlgType) {
		case SyncAlg :: SA_LBTS:
		    TheScheduler = new LBTS_TimedScheduler(lookaheadType);
		    break;
		case SyncAlg :: SA_CMB:
		    TheScheduler = new CMB_TimedScheduler(lookaheadType);
		    break;
		default:
		    assert(0);
	    }
	    break;

	case MIXED:
	    switch(syncAlgType) {
		case SyncAlg :: SA_LBTS:
		    TheScheduler = new LBTS_MixedScheduler(lookaheadType);
		    break;
		case SyncAlg :: SA_CMB:
		    TheScheduler = new CMB_MixedScheduler(lookaheadType);
		    break;
		default:
		    assert(0);
	    }
	    break;
    }//switch

    //2. reset all clocks
    Clock::Reset();
}

#endif //#ifdef NO_MPI

#endif

} //namespace kernel
} //namespace manifold


