/** @file manifold-decl.h
 *  Manifold header
 *  This file contains the declarations (not the implementation)
 *  of the Manifold class.  The actual implementation is in 
 *  manifold.h. We split them apart since we have circular dependencies
 *  with component requiring manifold, and manifold requiring component.
 */
 
// George F. Riley, (and others) Georgia Tech, Fall 2010


#ifndef MANIFOLD_KERNEL_MANIFOLD_DECL_H
#define MANIFOLD_KERNEL_MANIFOLD_DECL_H

#include <iostream>
#include <map>
#include "common-defs.h"
#include "component-decl.h"
#include "scheduler.h"
#include "quantum_scheduler.h"

namespace manifold {
namespace kernel {

class Clock;




/** \class Manifold 
 *  The Manifold class is implemented with only static functions.
 */
class Manifold
{
public:

    enum SchedulerType { TICKED=0, TIMED, MIXED };
    static void Init(SchedulerType=TICKED);
    #ifndef NO_MPI
    static void Init(int argc, char** argv, SchedulerType=TICKED,SyncAlg::SyncAlgType_t syncAlgType=SyncAlg::SA_CMB_OPT_TICK, Lookahead::LookaheadType_t lookaheadType=Lookahead::LA_GLOBAL);
    #endif
    static void Finalize();



    //! Run the simulation
    static void    Run();                 
  
    //! Stop the simulation
    static void    Stop(); 
  
    //! Terminate the simulation. In a parallel simulation, this means
    //! terminating all processes.
    static void    Terminate(); 
  
    //! Stop at the specified time
    static void    StopAtTime(Time_t);
  
    //! Stop at specified tick (default clk)
    static void    StopAt(Ticks_t);  
  
    //! Stop at ticks, based on specified clk
    //!  @arg A tick count.
    //!  @arg A reference clock for tick count.         
    static void    StopAtClock(Ticks_t, Clock&);
  
    //! Return the current simulation time
    static double  Now();                 
  
    //! Current simtime in ticks, default clk
    static Ticks_t NowTicks();            
  
    //! Current simtime in half ticks, default clk
    static Ticks_t NowHalfTicks();            
  
    //! Current simtime in ticks, given clk
    static Ticks_t NowTicks(Clock&);            
  
    //! Current simtime in half ticks, given clk
    static Ticks_t NowHalfTicks(Clock&);            
  
    //! Cancel previously scheduled event
    //!  @arg Unique time event id number.
    static bool    Cancel(EventId&);
  
    //! Cancel prev scheduled Tick event
    //!  @arg Unique tick event id number.
    static bool    Cancel(TickEventId&);
  
    //! Peek (doesn't remove) earliest event from the even list
    static EventId Peek();

    //! Return earliest event
    static EventBase* GetEarliestEvent();
  
    //! Returns MPI Rank if distributed, zero if not
    static LpId_t GetRank();
  
    //! Start the MPI processing             
    //static void   EnableDistributed();


#ifdef KERNEL_UTEST
    #ifdef NO_MPI
    static void Reset(SchedulerType t);
    #else
    static void Reset(SchedulerType t, SyncAlg::SyncAlgType_t syncAlgType,
                      Lookahead::LookaheadType_t lookaheadType=Lookahead::LA_GLOBAL);
    #endif
#endif

    static void print_stats(std::ostream& out);

  // There are some variations of the required API.
  // First specifies latency in units of clock ticks for the default clock
  // This the common case and should be used most of the time.

  // The next two specify latency in units of clock ticks for the default
  // clock                          

  /** Connect two components in both ways.
   *  @arg \c componentA The 1st component's unique id.
   *  @arg \c index1 The index(port) number of the 1st component.
   *  @arg \c handler1 First component's callback function pointer that expects a arbitrary type data.
   *  @arg \c component2 The 2nd component's unique id.
   *  @arg \c index2 The index(port) number of the 2nd component.
   *  @arg \c handler2 Second component's callback function pointer that expects a arbitrary type data.
   *  @arg \c clk1 Clock used by 1st component.
   *  @arg \c clk2 Clock used by 2nd component.
   *  @arg \c tickLatency1 Link latency in ticks from 1st component to 2nd.
   *  @arg \c tickLatency2 Link latency in ticks from 2nd component to 1st.
   */                          
  template <typename T, typename T2, typename U, typename U2>
  static void Connect(CompId_t component1, int index1,
                      void (T::*handler1)(int, T2),
                      CompId_t component2, int index2,
                      void (U::*handler2)(int, U2),
		      Clock& clk1, Clock& clk2,
                      Ticks_t tickLatency1, Ticks_t tickLatency2) throw (LinkTypeMismatchException);

  /** Connects an output of a component to an input on a second component.
   *  There are either one or two template parameters.  The first is always
   *  the type of the receiving component.  This is needed to insure the
   *  callback function is of the right type.  The second is optional
   *  and is the type of the data being sent on the link.
   *  If not specified, uint64_t is assumed.
   *  The handlers MUST all have the first parameter as an int, which
   *  indicates the input number the arrival is for.  The second is
   *  an arbitrary type T. Latency is in reference to the default clock.
   *  @arg \c sourceComponent The source component's unique id.
   *  @arg \c sourceIndex The source index(port) number.
   *  @arg \c dstComponent The destination component's unique id.
   *  @arg \c dstIndex The destination index(port) number.
   *  @arg \c handler Callback function pointer that expects a arbitrary type data.
   *  @arg \c tickLatency Link latency in ticks.
   */                          
  template <typename T, typename T2>
  static void Connect(CompId_t sourceComponent, int sourceIndex,
                          CompId_t dstComponent,    int dstIndex,
                          void (T::*handler)(int, T2),
                          Ticks_t tickLatency) throw (LinkTypeMismatchException);

  /** Connects an output of a component to an input on a second component.
   *  There are either one or two template parameters.  The first is always
   *  the type of the receiving component.  This is needed to insure the
   *  callback function is of the right type.  The second is optional
   *  and is the type of the data being sent on the link.
   *  If not specified, uint64_t is assumed.
   *  The handlers MUST all have the first parameter as an int, which
   *  indicates the input number the arrival is for.  The second is
   *  an arbitrary type T. Latency is in reference to the default clock.
   *  @arg \c sourceComponent The source component's unique id.
   *  @arg \c sourceIndex The source index(port) number.
   *  @arg \c dstComponent The destination component's unique id.
   *  @arg \c dstIndex The destination index(port) number.
   *  @arg \c handler Callback function pointer that expects a arbitrary type data.
   *  @arg \c tickLatency Link latency in half ticks.
   */                              
  template <typename T, typename T2>
  static void ConnectHalf(CompId_t sourceComponent, int sourceIndex,
                              CompId_t dstComponent,    int dstIndex,
                              void (T::*handler)(int, T2),
                              Ticks_t tickLatency) throw (LinkTypeMismatchException);
                              
  // The next two specify latency in units of clock ticks for the specified
  // clock
  
  /** Connects an output of a component to an input on a second component.
   *  There are either one or two template parameters.  The first is always
   *  the type of the receiving component.  This is needed to insure the
   *  callback function is of the right type.  The second is optional
   *  and is the type of the data being sent on the link.
   *  If not specified, uint64_t is assumed.
   *  The handlers MUST all have the first parameter as an int, which
   *  indicates the input number the arrival is for.  The second is
   *  an arbitrary type T. Latency is in reference to the specified clock.
   *  @arg \c sourceComponent The source component's unique id.
   *  @arg \c sourceIndex The source index(port) number.
   *  @arg \c dstComponent The destination component's unique id.
   *  @arg \c dstIndex The destination index(port) number.
   *  @arg \c c The specified reference clock for clock ticks.
   *  @arg \c handler Callback function pointer that expects a arbitrary type data.
   *  @arg \c tickLatency Link latency in ticks.
   */                                                                
  template <typename T, typename T2>
  static void ConnectClock(CompId_t sourceComponent, int sourceIndex,
                               CompId_t dstComponent,    int dstIndex,
                               Clock& c,
                               void (T::*handler)(int, T2),
                               Ticks_t tickLatency) throw (LinkTypeMismatchException);

  // The next two specify latency in units of half clock ticks for the specified
  // clock
  
  /** Connects an output of a component to an input on a second component.
   *  There are either one or two template parameters.  The first is always
   *  the type of the receiving component.  This is needed to insure the
   *  callback function is of the right type.  The second is optional
   *  and is the type of the data being sent on the link.
   *  If not specified, uint64_t is assumed.
   *  The handlers MUST all have the first parameter as an int, which
   *  indicates the input number the arrival is for.  The second is
   *  an arbitrary type T. Latency is in reference to the specified clock.
   *  @arg \c sourceComponent The source component's unique id.
   *  @arg \c sourceIndex The source index(port) number.
   *  @arg \c dstComponent The destination component's unique id.
   *  @arg \c dstIndex The destination index(port) number.
   *  @arg \c c The specified reference clock for clock ticks.
   *  @arg \c handler Callback function pointer that expects a arbitrary type data.
   *  @arg \c tickLatency Link latency in half ticks.
   */                                    
  template <typename T, typename T2>
  static void ConnectClockHalf(CompId_t sourceComponent, int sourceIndex,
                                   CompId_t dstComponent,    int dstIndex,
                                   Clock& c,
                                   void (T::*handler)(int, T2),
                                   Ticks_t tickLatency) throw (LinkTypeMismatchException);

  // The final two specify latency in units seconds
  // clock
  
  /** Connects an output of a component to an input on a second component.
   *  There are either one or two template parameters.  The first is always
   *  the type of the receiving component.  This is needed to insure the
   *  callback function is of the right type.  The second is optional
   *  and is the type of the data being sent on the link.
   *  If not specified, uint64_t is assumed.
   *  The handlers MUST all have the first parameter as an int, which
   *  indicates the input number the arrival is for.  The second is
   *  an arbitrary type T. Latency is in reference to the default clock.
   *  @arg \c sourceComponent The source component's unique id.
   *  @arg \c sourceIndex The source index(port) number.
   *  @arg \c dstComponent The destination component's unique id.
   *  @arg \c dstIndex The destination index(port) number.
   *  @arg \c handler Callback function pointer that expects a arbitrary type data.
   *  @arg \c tickLatency Link latency in time.
   */                                  
  template <typename T, typename T2>
  static void ConnectTime(CompId_t sourceComponent, int sourceIndex,
                              CompId_t dstComponent,    int dstIndex,
                              void (T::*handler)(int, T2),
                              Time_t latency) throw (LinkTypeMismatchException);

  // Define the templated schedule functions
  // There are variations for up to four parameters,
  // and another set for non-member (static) function callbacks.
  // These are the definitions of the functions, the implementations are
  // in manifold.h
  // --------------------------------------------------------------------- //
  // This set are for "tick" events, that  are scheduled on integral ticks
  // with respect to the "master" (first defined) clock

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the master clock.
   *  @arg \c handler 0 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  \return Unique tick event id
   */
  template <typename T, typename OBJ>
    static TickEventId Schedule(Ticks_t t, void(T::*handler)(void), OBJ* obj);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the master clock.
   *  @arg \c handler 1 Parameter callback function pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list
   *  \return Unique tick event id
   */
  template <typename T, typename OBJ,
    typename U1, typename T1>
    static TickEventId Schedule(Ticks_t t, void(T::*handler)(U1), OBJ* obj, T1 t1);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the master clock.
   *  @arg \c handler 2 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list    
   *  \return Unique tick event id
   */
  template <typename T, typename OBJ,
    typename U1, typename T1,
    typename U2, typename T2>
    static TickEventId Schedule(Ticks_t t, void(T::*handler)(U1, U2), OBJ* obj, T1 t1, T2 t2);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the master clock.
   *  @arg \c handler 3 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list   
   *  \return Unique tick event id
   */
  template <typename T, typename OBJ,
    typename U1, typename T1,
    typename U2, typename T2,
    typename U3, typename T3>
    static TickEventId Schedule(Ticks_t t, void(T::*handler)(U1, U2, U3), OBJ* obj, T1 t1, T2 t2, T3 t3);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the master clock.
   *  @arg \c handler 4 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list
   *  @arg \c t4 4th parameter in callback function list   
   *  \return Unique tick event id
   */
  template <typename T, typename OBJ,
    typename U1, typename T1,
    typename U2, typename T2,
    typename U3, typename T3,
    typename U4, typename T4>
    static TickEventId Schedule(Ticks_t t, void(T::*handler)(U1, U2, U3, U4), OBJ* obj, T1 t1, T2 t2, T3 t3, T4 t4);

  // --------------------------------------------------------------------- //
  // This set are for "tick" events, that  are scheduled on integral ticks
  // with respect to the "master" (first defined) clock.  The units are
  // half-ticks. If presently on a rising edge, a value of 3 (for example)
  // results in a falling edge event 1.5 cycles in the future
  
  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the master clock.
   *  @arg \c handler 0 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  \return Unique tick event id
   */  
  template <typename T, typename OBJ>
    static TickEventId ScheduleHalf(Ticks_t t, void(T::*handler)(void), OBJ* obj);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the master clock.
   *  @arg \c handler 1 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list   
   *  \return Unique tick event id
   */
  template <typename T, typename OBJ,
    typename U1, typename T1>
    static TickEventId ScheduleHalf(Ticks_t t, void(T::*handler)(U1), OBJ* obj, T1 t1);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the master clock.
   *  @arg \c handler 2 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list    
   *  \return Unique tick event id
   */
  template <typename T, typename OBJ,
    typename U1, typename T1,
    typename U2, typename T2>
    static TickEventId ScheduleHalf(Ticks_t t, void(T::*handler)(U1, U2), OBJ* obj, T1 t1, T2 t2);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the master clock.
   *  @arg \c handler 3 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list   
   *  \return Unique tick event id
   */
  template <typename T, typename OBJ,
    typename U1, typename T1,
    typename U2, typename T2,
    typename U3, typename T3>
    static TickEventId ScheduleHalf(Ticks_t t, void(T::*handler)(U1, U2, U3), OBJ* obj, T1 t1, T2 t2, T3 t3);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the master clock.
   *  @arg \c handler 4 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list
   *  @arg \c t4 4th parameter in callback function list
   *  \return Unique tick event id
   */
  template <typename T, typename OBJ,
    typename U1, typename T1,
    typename U2, typename T2,
    typename U3, typename T3,
    typename U4, typename T4>
    static TickEventId ScheduleHalf(Ticks_t t, void(T::*handler)(U1, U2, U3, U4), OBJ* obj, T1 t1, T2 t2, T3 t3, T4 t4);

  // This set are for "tick" events, that  are scheduled on integral ticks
  // with respect to the specified clock
  
  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the specified clock.
   *  @arg \c c Reference clock for latency.
   *  @arg \c handler 0 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  \return Unique tick event id
   */  
  template <typename T, typename OBJ>
    static TickEventId ScheduleClock(Ticks_t t, Clock& c, void(T::*handler)(void), OBJ* obj);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the specified clock.
   *  @arg \c c Reference clock for latency.
   *  @arg \c handler 1 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list   
   *  \return Unique tick event id
   */  
  template <typename T, typename OBJ,
    typename U1, typename T1>
    static TickEventId ScheduleClock(Ticks_t t, Clock& c, void(T::*handler)(U1), OBJ* obj, T1 t1);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the specified clock.
   *  @arg \c c Reference clock for latency.
   *  @arg \c handler 2 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list    
   *  \return Unique tick event id
   */      
  template <typename T, typename OBJ,
    typename U1, typename T1,
    typename U2, typename T2>
    static TickEventId ScheduleClock(Ticks_t t, Clock& c, void(T::*handler)(U1, U2), OBJ* obj, T1 t1, T2 t2);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the specified clock.
   *  @arg \c c Reference clock for latency.
   *  @arg \c handler 3 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list   
   *  \return Unique tick event id
   */      
  template <typename T, typename OBJ,
    typename U1, typename T1,
    typename U2, typename T2,
    typename U3, typename T3>
    static TickEventId ScheduleClock(Ticks_t t, Clock& c, void(T::*handler)(U1, U2, U3), OBJ* obj, T1 t1, T2 t2, T3 t3);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the specified clock.
   *  @arg \c c Reference clock for latency.
   *  @arg \c handler 4 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list
   *  @arg \c t4 4th parameter in callback function list   
   *  \return Unique tick event id
   */      
  template <typename T, typename OBJ,
    typename U1, typename T1,
    typename U2, typename T2,
    typename U3, typename T3,
    typename U4, typename T4>
    static TickEventId ScheduleClock(Ticks_t t, Clock& c, void(T::*handler)(U1, U2, U3, U4), OBJ* obj, T1 t1, T2 t2, T3 t3, T4 t4);

  // This set are for "tick" events, that  are scheduled on integral half-ticks
  // with respect to the specified clock
  
  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the specified clock.
   *  @arg \c c Reference clock for latency.
   *  @arg \c handler 0 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  \return Unique tick event id
   */      
  template <typename T, typename OBJ>
    static TickEventId ScheduleClockHalf(Ticks_t t, Clock& c, void(T::*handler)(void), OBJ* obj);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the specified clock.
   *  @arg \c c Reference clock for latency.
   *  @arg \c handler 1 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list   
   *  \return Unique tick event id
   */
  template <typename T, typename OBJ,
    typename U1, typename T1>
    static TickEventId ScheduleClockHalf(Ticks_t t, Clock& c, void(T::*handler)(U1), OBJ* obj, T1 t1);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the specified clock.
   *  @arg \c c Reference clock for latency.
   *  @arg \c handler 2 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list    
   *  \return Unique tick event id
   */
  template <typename T, typename OBJ,
    typename U1, typename T1,
    typename U2, typename T2>
    static TickEventId ScheduleClockHalf(Ticks_t t, Clock& c, void(T::*handler)(U1, U2), OBJ* obj, T1 t1, T2 t2);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the specified clock.
   *  @arg \c c Reference clock for latency.
   *  @arg \c handler 3 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list   
   *  \return Unique tick event id
   */
  template <typename T, typename OBJ,
    typename U1, typename T1,
    typename U2, typename T2,
    typename U3, typename T3>
    static TickEventId ScheduleClockHalf(Ticks_t t, Clock& c, void(T::*handler)(U1, U2, U3), OBJ* obj, T1 t1, T2 t2, T3 t3);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the specified clock.
   *  @arg \c c Reference clock for latency.
   *  @arg \c handler 4 Parameter member function callback pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list
   *  @arg \c t4 4th parameter in callback function list   
   *  \return Unique tick event id
   */
  template <typename T, typename OBJ,
    typename U1, typename T1,
    typename U2, typename T2,
    typename U3, typename T3,
    typename U4, typename T4>
    static TickEventId ScheduleClockHalf(Ticks_t t, Clock& c, void(T::*handler)(U1, U2, U3, U4), OBJ* obj, T1 t1, T2 t2, T3 t3, T4 t4);

  // -------------------------------------------------------------------- //
  // This set is for member function callbacks with floating point time
  
  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in time with respect to the default clock.
   *  @arg \c handler 0 Parameter member function callback function pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  \return Unique time event id
   */  
  template <typename T, typename OBJ>
    static EventId ScheduleTime(double t, void(T::*handler)(void), OBJ* obj);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in time with respect to the default clock.
   *  @arg \c handler 1 Parameter member function callback function pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list
   *  \return Unique time event id
   */
  template <typename T, typename OBJ,
    typename U1, typename T1>
    static EventId ScheduleTime(double t, void(T::*handler)(U1), OBJ* obj, T1 t1);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in time with respect to the default clock.
   *  @arg \c handler 2 Parameter member function callback function pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  \return Unique time event id
   */
  template <typename T, typename OBJ,
    typename U1, typename T1,
    typename U2, typename T2>
    static EventId ScheduleTime(double t, void(T::*handler)(U1, U2), OBJ* obj, T1 t1, T2 t2);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in time with respect to the default clock.
   *  @arg \c handler 3 Parameter member function callback function pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list  
   *  \return Unique time event id
   */
  template <typename T, typename OBJ,
    typename U1, typename T1,
    typename U2, typename T2,
    typename U3, typename T3>
    static EventId ScheduleTime(double t, void(T::*handler)(U1, U2, U3), OBJ* obj, T1 t1, T2 t2, T3 t3);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in time with respect to the default clock.
   *  @arg \c handler 4 Parameter member function callback function pointer.
   *  @arg \c obj Object the callback function will be called on.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list
   *  @arg \c t4 4th parameter in callback function list    
   *  \return Unique time event id
   */
  template <typename T, typename OBJ,
    typename U1, typename T1,
    typename U2, typename T2,
    typename U3, typename T3,
    typename U4, typename T4>
    static EventId ScheduleTime(double t, void(T::*handler)(U1, U2, U3, U4), OBJ* obj, T1 t1, T2 t2, T3 t3, T4 t4);

  // --------------------------------------------------------------------- //
  // Schedulers for static callback functions with integral (ticks)  time
  // using the default clock

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the default clock.
   *  @arg \c handler 0 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list
   *  @arg \c t4 4th parameter in callback function list    
   *  \return Unique tick event id
   */  
  static TickEventId Schedule(Ticks_t t, void(*handler)(void));

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the default clock.
   *  @arg \c handler 1 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  \return Unique tick event id
   */      
  template <typename U1, typename T1>
    static TickEventId Schedule(Ticks_t t, void(*handler)(U1), T1 t1);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the default clock.
   *  @arg \c handler 2 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  \return Unique tick event id
   */      
  template <typename U1, typename T1,
            typename U2, typename T2>
    static TickEventId Schedule(Ticks_t t, void(*handler)(U1, U2), T1 t1, T2 t2);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the default clock.
   *  @arg \c handler 3 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list   
   *  \return Unique tick event id
   */      
  template <typename U1, typename T1,
            typename U2, typename T2,
            typename U3, typename T3>
    static TickEventId Schedule(Ticks_t t, void(*handler)(U1, U2, U3), T1 t1, T2 t2, T3 t3);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the default clock.
   *  @arg \c handler 4 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list
   *  @arg \c t4 4th parameter in callback function list    
   *  \return Unique tick event id
   */      
  template <typename U1, typename T1,
            typename U2, typename T2,
            typename U3, typename T3,
            typename U4, typename T4>
    static TickEventId Schedule(Ticks_t t, void(*handler)(U1, U2, U3, U4), T1 t1, T2 t2, T3 t3, T4 t4);

  // --------------------------------------------------------------------- //
  // Schedulers for static callback functions with half time
  // using the default clock
  
  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the default clock.
   *  @arg \c handler 0 Parameter static callback function pointer.
   *  \return Unique tick event id
   */      
  static TickEventId ScheduleHalf(Ticks_t t, void(*handler)(void));

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the default clock.
   *  @arg \c handler 1 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  \return Unique tick event id
   */
  template <typename U1, typename T1>
    static TickEventId ScheduleHalf(Ticks_t t, void(*handler)(U1), T1 t1);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the default clock.
   *  @arg \c handler 2 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  \return Unique tick event id
   */    
  template <typename U1, typename T1,
            typename U2, typename T2>
    static TickEventId ScheduleHalf(Ticks_t t, void(*handler)(U1, U2), T1 t1, T2 t2);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the default clock.
   *  @arg \c handler 3 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list
   *  \return Unique tick event id
   */    
  template <typename U1, typename T1,
            typename U2, typename T2,
            typename U3, typename T3>
    static TickEventId ScheduleHalf(Ticks_t t, void(*handler)(U1, U2, U3), T1 t1, T2 t2, T3 t3);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the default clock.
   *  @arg \c handler 4 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list
   *  @arg \c t4 4th parameter in callback function list    
   *  \return Unique tick event id
   */    
  template <typename U1, typename T1,
            typename U2, typename T2,
            typename U3, typename T3,
            typename U4, typename T4>
    static TickEventId ScheduleHalf(Ticks_t t, void(*handler)(U1, U2, U3, U4), T1 t1, T2 t2, T3 t3, T4 t4);

  // Schedulers for static callback functions with integral (ticks)  time
  // using the specified clock

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the specified clock.
   *  @arg The reference clock the latency is based on.
   *  @arg \c handler 0 Parameter static callback function pointer.
   *  \return Unique tick event id
   */    
  static TickEventId ScheduleClock(Ticks_t t, Clock&, void(*handler)(void));

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the specified clock.
   *  @arg The reference clock the latency is based on.
   *  @arg \c handler 1 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  \return Unique tick event id
   */
  template <typename U1, typename T1>
    static TickEventId ScheduleClock(Ticks_t t, Clock&, void(*handler)(U1), T1 t1);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the specified clock.
   *  @arg The reference clock the latency is based on.
   *  @arg \c handler 2 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  \return Unique tick event id
   */
  template <typename U1, typename T1,
            typename U2, typename T2>
    static TickEventId ScheduleClock(Ticks_t t, Clock&, void(*handler)(U1, U2), T1 t1, T2 t2);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the specified clock.
   *  @arg The reference clock the latency is based on.
   *  @arg \c handler 3 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list
   *  \return Unique tick event id
   */
  template <typename U1, typename T1,
            typename U2, typename T2,
            typename U3, typename T3>
    static TickEventId ScheduleClock(Ticks_t t, Clock&, void(*handler)(U1, U2, U3), T1 t1, T2 t2, T3 t3);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in ticks with respect to the specified clock.
   *  @arg The reference clock the latency is based on.
   *  @arg \c handler 4 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list
   *  @arg \c t4 4th parameter in callback function list    
   *  \return Unique tick event id
   */
  template <typename U1, typename T1,
            typename U2, typename T2,
            typename U3, typename T3,
            typename U4, typename T4>
    static TickEventId ScheduleClock(Ticks_t t, Clock&, void(*handler)(U1, U2, U3, U4), T1 t1, T2 t2, T3 t3, T4 t4);

  // Schedulers for static callback functions with half ticks time
  // using the specified clock

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the specified clock.
   *  @arg The reference clock the latency is based on.
   *  @arg \c handler 0 Parameter static callback function pointer.
   *  \return Unique tick event id
   */  
  static TickEventId ScheduleClockHalf(Ticks_t t, Clock&, void(*handler)(void));

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the specified clock.
   *  @arg The reference clock the latency is based on.
   *  @arg \c handler 1 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  \return Unique tick event id
   */      
  template <typename U1, typename T1>
    static TickEventId ScheduleClockHalf(Ticks_t t, Clock&, void(*handler)(U1), T1 t1);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the specified clock.
   *  @arg The reference clock the latency is based on.
   *  @arg \c handler 2 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  \return Unique tick event id
   */      
  template <typename U1, typename T1,
            typename U2, typename T2>
    static TickEventId ScheduleClockHalf(Ticks_t t, Clock&, void(*handler)(U1, U2), T1 t1, T2 t2);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the specified clock.
   *  @arg The reference clock the latency is based on.
   *  @arg \c handler 3 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list
   *  \return Unique tick event id
   */      
  template <typename U1, typename T1,
            typename U2, typename T2,
            typename U3, typename T3>
    static TickEventId ScheduleClockHalf(Ticks_t t, Clock&, void(*handler)(U1, U2, U3), T1 t1, T2 t2, T3 t3);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in half ticks with respect to the specified clock.
   *  @arg The reference clock the latency is based on.
   *  @arg \c handler 4 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list
   *  @arg \c t4 4th parameter in callback function list    
   *  \return Unique tick event id
   */      
  template <typename U1, typename T1,
            typename U2, typename T2,
            typename U3, typename T3,
            typename U4, typename T4>
    static TickEventId ScheduleClockHalf(Ticks_t t, Clock&, void(*handler)(U1, U2, U3, U4), T1 t1, T2 t2, T3 t3, T4 t4);

  // --------------------------------------------------------------------- //
  // Schedulers for static callback functions with floating point time
  
  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in time with respect to the default clock.
   *  @arg \c handler 0 Parameter static callback function pointer.
   *  \return Unique time event id
   */      
  static EventId ScheduleTime(double t, void(*handler)(void));

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in time with respect to the default clock.
   *  @arg \c handler 1 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  \return Unique time event id
   */
  template <typename U1, typename T1>
    static EventId ScheduleTime(double t, void(*handler)(U1), T1 t1);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in time with respect to the default clock.
   *  @arg \c handler 2 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  \return Unique time event id
   */
  template <typename U1, typename T1,
            typename U2, typename T2>
    static EventId ScheduleTime(double t, void(*handler)(U1, U2), T1 t1, T2 t2);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in time with respect to the default clock.
   *  @arg \c handler 3 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list
   *  \return Unique time event id
   */
  template <typename U1, typename T1,
            typename U2, typename T2,
            typename U3, typename T3>
    static EventId ScheduleTime(double t, void(*handler)(U1, U2, U3), T1 t1, T2 t2, T3 t3);

  /** To Schedule an event for a component without going through a link, this 
   *  function can only be called on components within a rank(logical process).
   *  @arg \c t Latency in time with respect to the default clock.
   *  @arg \c handler 4 Parameter static callback function pointer.
   *  @arg \c t1 1st parameter in callback function list
   *  @arg \c t2 2nd parameter in callback function list 
   *  @arg \c t3 3rd parameter in callback function list
   *  @arg \c t4 4th parameter in callback function list    
   *  \return Unique time event id
   */
  template <typename U1, typename T1,
            typename U2, typename T2,
            typename U3, typename T3,
            typename U4, typename T4>
    static EventId ScheduleTime(double t, void(*handler)(U1, U2, U3, U4), T1 t1, T2 t2, T3 t3, T4 t4);


  static Scheduler* get_scheduler() { return TheScheduler; }

  //static void print_lookahead() { TheScheduler->print_lookahead(); }

private:

  static Scheduler* TheScheduler;
  
   //! A flag set to TRUE if simulation is distributed.
   //static bool       isDistributed;
  
  template <typename T, typename T2>
  static void DoConnect(CompId_t sourceComponent, int sourceIndex,
      CompId_t dstComponent, int dstIndex,
      void (T::*handler)(int,  T2),
      Ticks_t latencyTicks,
      Time_t delay, bool isTimed, bool isHalf,
      Clock* c) throw (LinkTypeMismatchException);
};

} //namespace kernel
} //namespace manifold

#endif // MANIFOLD_KERNEL_MANIFOLD_DECL_H
