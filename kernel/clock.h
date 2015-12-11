/** @file clock.h
 *  Contains classes and functions related to clocks.
 */

// George F. Riley, (and others) Georgia Tech, Fall 2010

#ifndef MANIFOLD_KERNEL_CLOCK_H
#define MANIFOLD_KERNEL_CLOCK_H

#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <assert.h>

#include "common-defs.h"
#include "manifold-decl.h"

namespace manifold {
namespace kernel {

/** Base class for objects keeping track of tick handlers.
 */
class tickObjBase
{
 public:

 //! By default tick handlers are enabled
 tickObjBase() : enabled(true) {}

 //! Virtual rising tick handler
 virtual void CallRisingTick() = 0;

 //! Virtual falling tick handler
 virtual void CallFallingTick() = 0;

 //! Enables tick handlers
 void         Enable() {enabled = true;}

 //! Disables tick handlers.
 void         Disable() {enabled = false;}

 bool         enabled;
};



/** Object to keep track of tick handlers.
 */
template <typename OBJ>
class tickObj : public tickObjBase {
 public:

 //! @arg \c o Object registered with the clock.
 tickObj(OBJ* o) : obj(o), risingFunct(0), fallingFunct(0) {}

 //! @arg \c o Object registered with the clock.
 //! @arg \c r Pointer to rising function that will be called on tick.
 //! @arg \c f Pointer to falling function that will be called on tick.
 tickObj(OBJ* o, void (OBJ::*r)(void), void (OBJ::*f)(void))
  : obj(o), risingFunct(r), fallingFunct(f)
  {}

 //! Calls rising clk callback function on object registered with clock.
 void CallRisingTick()
  {
    if (risingFunct) {
      (obj->*risingFunct)();
    }
  }

 //! Calls falling clk callback function on object registered with clock.
 void CallFallingTick()
  {
    if (fallingFunct)
      {
        (obj->*fallingFunct)();
      }
  }

 //! Object registered with clock.
 OBJ* obj;

 //! Pointer to rising function that will be called on tick.
 void (OBJ::*risingFunct)(void);

 //! Pointer to falling function that will be called on tick.
 void (OBJ::*fallingFunct)(void);
};

#define CLOCK_CALENDAR_LENGTH 128

// Stats engine
class Clock_stat_engine;


/** \class Clock A clock object that components can register to.
 *
 *  The clock object contains:
 *  1) List of objects requiring the Rising and Falling callbacks
 *  2) List of future events that are scheduled using the "ticks"
 *     time instead of floating point time.  This is implemented
 *     using a calendar queue for any event less than
 *     CLOCK_CALENDER_LENGTH ticks in the future, and a simple sorted
 *     list (map) of events scheduled more than that length in the future.
 */
class Clock
{
 public:

  //! @arg Frequency(hz) of the clock
  Clock(double);

  virtual ~Clock();

  //! Processes all rising edge callbacks that have been registered with the clock.
  void Rising();

  //! Processes all falling edge callbacks that have been registered with the clock.
  void Falling();

  //! Inserts an event to be processed
  //!  @arg The time specified in the TickEvent is ticks in the future
  //!  @return The Id of the event is returned.
  TickEventId Insert(TickEventBase*);

  //! Inserts an event to be processed
  //!  @arg The time specified in the TickEvent is half-ticks in the future
  //!  @return The Id of the event is returned.
  TickEventId InsertHalf(TickEventBase*);

  //! Cancels an event
  //!  @arg The event with the specified Id is cancelled.
  void        Cancel(TickEventId);

  //! Returns floating point time of the next tick.
  virtual Time_t  NextTickTime() const;

  //! Returns the current tick counter
  Ticks_t     NowTicks() const;

  //! Returns the current tick counter in half ticks
  Ticks_t     NowHalfTicks() const;

  //! Processes all events in the current tick
  void        ProcessThisTick();


  void print_stats(std::ostream& out);

  void terminate();

  template <typename O>
  void register_output_predictor(O* obj, void(O::*rising)(void));

  void do_output_prediction();


  typedef std::vector<Clock*> ClockVec_t;


  //! Returns a reference to the master clock
  static  Clock& Master();

  //! Returns the time for the master clock
  static  Ticks_t Now();

  //! Returns the time for the master clock
  static  Ticks_t NowHalf();

  //! Returns the time from the specified clock
  static  Ticks_t Now(Clock&);

  //! Returns the time from the specified clock
  static  Ticks_t NowHalf(Clock&);

  //! Returns the vector of clock objects
  static ClockVec_t& GetClocks();

  /** Register an object with the specified clock object
   *  Uses "master" clock
   * @arg \c obj A pointer to the component to register with the clock.
   * @arg \c rising A pointer to the member function that will be called on a
   * rising edge.  This function must not take any arguments or return any
   * values.
   * @arg \c falling A pointer to the member function that will be called on a
   * falling edge.  This function must not take any arguments or return any
   * values.
   * @return Returns a tickObjBase ptr used to disable/enable tick handlers.
   */
  template <typename O> static tickObjBase* Register(
                                                     O*          obj,
                                                     void(O::*rising)(void),
                                                     void(O::*falling)(void));
  /** Register an object with the specified clock object
   *  Uses specified clock
   * @arg \c whichClock Specified clock to register to.
   * @arg \c obj A pointer to the component to register with the clock.
   * @arg \c rising A pointer to the member function that will be called on a
   * rising edge.  This function must not take any arguments or return any
   * values.
   * @arg \c falling A pointer to the member function that will be called on a
   * falling edge.  This function must not take any arguments or return any
   * values.
   * @return Returns a tickObjBase ptr used to disable/enable tick handlers.
   */
  template <typename O> static tickObjBase* Register(
                                                     Clock&  whichClock,
                                                     O*      obj,
                                                     void(O::*rising)(void),
                                                     void(O::*falling)(void));
  template <typename O> static void Unregister( Clock& c, O* obj);


  //! Typedefs for the calendar queue
  typedef std::vector<TickEventBase*> EventVec_t;

  //! Typedefs for the calendar queue
  typedef std::vector<EventVec_t>     EventVecVec_t;

  //! To account for events "too far" in the future, we do need
  //! a sorted (multimap) for those
  typedef std::multimap<Ticks_t, TickEventBase*> EventMap_t;


#ifdef KERNEL_UTEST
public:
    static void Reset();

    std::list<tickObjBase*>& getRegistered() { return tickObjs; }
    void unregisterAll()
    {
	tickObjs.clear();
    }
    const EventVecVec_t& getCalendar() const { return calendar; }
    const EventMap_t& getEventMap() const { return events; }
#endif


public:

  //! Period of the clock (seconds)
  double  period;

  //! Frequency of the clock (hz)
  double  freq;

  //! The rising edge is the "tick", the falling is one-half tick later
  //!  True if next is rising edge
  bool    nextRising;

  //! The rising edge is the "tick", the falling is one-half tick later
  //!  Tick count of next tick
  Ticks_t nextTick;

private:

  //! Called at early termination. Disables all registered components.
  void disableAll();

  //! Stores the actual calendar queue
  EventVecVec_t calendar;

  //! Stores events outside the calendar queue
  EventMap_t    events;

  //! Holds the list of handlers that have been registered
  std::list<tickObjBase*> tickObjs;

  //list of components that are interested in predicting their output.
  std::list<tickObjBase*> output_predictors;

  //! Stores the vector of clock objects
  static ClockVec_t* clocks;

  Clock_stat_engine* stats;

};

class Clock_stat_engine : public Stat_engine
{
    public:
        Clock_stat_engine();
        ~Clock_stat_engine();
        manifold::kernel::Persistent_stat<manifold::kernel::counter_t> queued_events;
        manifold::kernel::Persistent_stat<manifold::kernel::counter_t> rising_calendar_events;
        manifold::kernel::Persistent_stat<manifold::kernel::counter_t> falling_calendar_events;
        manifold::kernel::Persistent_stat<manifold::kernel::counter_t> registered_events;

        void global_stat_merge(Stat_engine *);
        void print_stats(std::ostream & out);
        void clear_stats();

        void start_warmup();
        void end_warmup();
        void save_samples();
};





template <typename O> tickObjBase* Clock::Register(
                                                   O*          obj,
                                                   void(O::*rising)(void),
                                                   void(O::*falling)(void))
{
    // Get the clock registered for this period
    Clock& c = Master();
    return Register(c, obj, rising, falling);
}

template <typename O> tickObjBase* Clock::Register(
                                                   Clock&      c,
                                                   O*          obj,
                                                   void(O::*rising)(void),
                                                   void(O::*falling)(void))
{
    tickObj<O>* t = new tickObj<O>(obj, rising, falling);
    c.tickObjs.push_back(t);
    obj->set_clock(c);
    return t;
}

template <typename O> void Clock::Unregister(
                                                   Clock&      c,
                                                   O*          obj)
{
    std::list<tickObjBase*>::iterator it = c.tickObjs.begin();
    for(; it!=c.tickObjs.end(); ++it) {
        tickObj<O>* t = dynamic_cast<tickObj<O>*>(*it);
        if(t && t->obj == obj)
	    break;
    }
    assert(it != c.tickObjs.end());

    //we don't delete the registered object.
    c.tickObjs.erase(it);
}


template <typename O>
void Clock :: register_output_predictor(O* obj, void(O::*rising)(void))
{
    tickObj<O>* t = new tickObj<O>(obj, rising, 0);
    output_predictors.push_back(t);
}





class MultipleFreqChangeException //we do not allow a clock to change frequency multiple times in a single cycle
{ //empty class
};


class DVFSClock : public Clock {
public:
    DVFSClock(double f);

    Time_t NextTickTime() const; //overwrite base class

    void set_frequency(double f) throw (MultipleFreqChangeException);

private:
    Ticks_t m_lastChangeTick; //tick when frequency last changed.
    Time_t m_lastChangeTime; //time (seconds) of the rising edge when frequency last changed.
};








} //namespace kernel
} //namespace manifold


#endif //MANIFOLD_KERNEL_CLOCK_H
