#ifndef MANIFOLD_KERNEL_MANIFOLD_EVENT_H
#define MANIFOLD_KERNEL_MANIFOLD_EVENT_H
#include "common-defs.h"

namespace manifold {
namespace kernel {

class Clock;

/** Defines the base event class for tick events.
 */
class TickEventBase 
{
 public:
 
 /** Constructor
  *  @arg \c t Latency of event in ticks.
  *  @arg \c c Reference to the clock ticks are based on.
  */
 TickEventBase(Ticks_t t, Clock& c)
   : time(t), uid(nextUID++), clock(c), cancelled(false), rising(true) {}

 /** Constructor
  *  @arg \c t Latency of event in ticks
  *  @arg \c u Unique id of the event
  *  @arg \c c Reference to the clock ticks are based on.
  */  
 TickEventBase(Ticks_t t, int u, Clock& c) 
   : time(t), uid(u), clock(c), cancelled(false), rising(true) {}
   
  /** Virtual function, all subclasses must implement CallHandler
   */
  virtual void CallHandler() = 0;
  
 public:
 
  /** Timestamp (units of ticks) for the event
   */
  Ticks_t time;
  
  /** Each event has a unique identifier to break timestamp ties
   */
  int     uid;
  
  /** Clock reference for the tick event
   */
  Clock&  clock;
  
  /** True if cancelled before scheduling
   */
  bool    cancelled;
  
  /** True if rising edge event
   */
  bool    rising;     

 private:
 
  /** Holds the next unused unique event identifier
   */
  static int nextUID;
};

/** TickEventID class subclasses TickEventBase, and 
 *  is the return type from all schedule functions. 
 *  This is used to cancel the event.
 */
class TickEventId : public TickEventBase
{
public:
 
 /** Constructor
  *  @arg \c t Latency specified in ticks.
  *  @arg \c u Unique id for the event.
  *  @arg \c c Reference clock for ticks.
  */
 TickEventId(Ticks_t t, int u, Clock& c) : TickEventBase(t, u, c) {}
 
  /** Calls the callback function when the event is processed.
   */
  void CallHandler() {}
};

// ----------------------------------------------------------------------- //

/** TickEvent0 subclasses TickEventBase, it defines a 
 *  0 parameter member function CallHandler for a TickEvent
 */
template<typename T, typename OBJ>
class TickEvent0 : public TickEventBase
{
public:

  /** Constructor
   *  @arg \c t Latency in ticks.
   *  @arg \c c Reference clock used with tick latency.
   *  @arg \c f 0 Paramater call back function pointer for tick event
   *  @arg \c obj0 Object the callback is called on.
   */
  TickEvent0(Ticks_t t, Clock& c, void (T::*f)(void), OBJ* obj0)
    : TickEventBase(t,c), handler(f), obj(obj0){}
  
  /** Defines the callback handler function pointer
   */    
  void (T::*handler)(void);
  
  /** Object the CallHandler uses for the callback function
   */
  OBJ*      obj;
public:

  /** Calls the callback function when the event is processed.
   */
  void CallHandler();
};

template <typename T, typename OBJ>
void TickEvent0<T, OBJ>::CallHandler()
{
  (obj->*handler)();
}

/** TickEvent1 subclasses TickEventBase, it defines a 
 *  1 parameter member function CallHandler for a TickEvent
 */
template<typename T, typename OBJ, typename U1, typename T1>
class TickEvent1 : public TickEventBase
{
public:

  /** Constructor
   *  @arg \c t Latency in ticks.
   *  @arg \c c Reference clock used with tick latency.
   *  @arg \c f 1 Parameter callback function pointer for tick event
   *  @arg \c obj0 Object the callback is called on.
   *  @arg \c t1_0 1st parameter in callback function list
   */
  TickEvent1(Ticks_t t, Clock& c, void (T::*f)(U1), OBJ* obj0, T1 t1_0)
    : TickEventBase(t,c), handler(f), obj(obj0), t1(t1_0){}

  /** Defines the callback handler function pointer
   */        
  void (T::*handler)(U1);
  
  /** Object the CallHandler uses for the callback function
   */
  OBJ*      obj;
  
  /** 1st parameter in callback function list
   */
  T1        t1;
  
public:

  /** Calls the callback function when the event is processed.
   */
  void CallHandler();
};

template <typename T, typename OBJ, typename U1, typename T1>
void TickEvent1<T, OBJ, U1, T1>::CallHandler()
{
  (obj->*handler)(t1);
}

/** TickEvent2 subclasses TickEventBase, it defines a 
 *  2 parameter member function CallHandler for a TickEvent
 */
template<typename T, typename OBJ,
         typename U1, typename T1, 
         typename U2, typename T2>
class TickEvent2 : public TickEventBase
{
public:

  /** Constructor
   *  @arg \c t Latency in ticks.
   *  @arg \c c Reference clock used with tick latency.
   *  @arg \c f 2 Parameter callback function pointer for tick event
   *  @arg \c obj0 Object the callback is called on.
   *  @arg \c t1_0 1st parameter in callback function list
   *  @arg \c t2_0 2nd parameter in callback function list      
   */    
  TickEvent2(Ticks_t t, Clock& c, void (T::*f)(U1, U2), OBJ* obj0, T1 t1_0, T2 t2_0)
    : TickEventBase(t,c), handler(f), obj(obj0), t1(t1_0), t2(t2_0) {}

  /** Defines the callback handler function pointer
   */        
  void (T::*handler)(U1, U2);
  
  /** Object the CallHandler uses for the callback function
   */  
  OBJ*      obj;

  /** 1st parameter in callback function list
   */
  T1        t1;

  /** 2nd parameter in callback function list
   */  
  T2        t2;
  
public:

  /** Calls the callback function when the event is processed.
   */
  void CallHandler();
};

template <typename T, typename OBJ, 
          typename U1, typename T1,
          typename U2, typename T2>
void TickEvent2<T, OBJ, U1, T1, U2, T2>::CallHandler()
{
  (obj->*handler)(t1, t2);
}

/** TickEvent3 subclasses TickEventBase, it defines a 
 *  3 parameter member function CallHandler for a TickEvent
 */
template <typename T,  typename OBJ,
          typename U1, typename T1,
          typename U2, typename T2,
          typename U3, typename T3>
class TickEvent3 : public TickEventBase {
public:

   /** Constructor
    *  @arg \c t Latency in ticks.
    *  @arg \c c Reference clock used with tick latency.
    *  @arg \c f 3 Parameter callback function pointer for tick event
    *  @arg \c obj0 Object the callback is called on.
    *  @arg \c t1_0 1st parameter in callback function list
    *  @arg \c t2_0 2nd parameter in callback function list 
    *  @arg \c t3_0 3rd parameter in callback function list
    */    
   TickEvent3(Ticks_t t, Clock& c, void (T::*f)(U1, U2, U3), OBJ *obj0, T1 t1_0, T2 t2_0, T3 t3_0)  
     : TickEventBase(t,c), handler(f), obj(obj0), t1(t1_0), t2(t2_0), t3(t3_0) {}

   /** Defines the callback handler function pointer
    */      
   void (T::*handler)(U1, U2, U3);
   
   /** Object the CallHandler uses for the callback function
    */   
   OBJ* obj;

   /** 1st parameter in callback function list
    */   
   T1 t1;

   /** 2nd parameter in callback function list
    */   
   T2 t2;

   /** 3rd parameter in callback function list
    */   
   T3 t3;
   
public:

   /** Calls the callback function when the event is processed.
    */ 
   void CallHandler();
};

template <typename T,  typename OBJ,
          typename U1, typename T1,
          typename U2, typename T2,
          typename U3, typename T3> 
void TickEvent3<T,OBJ,U1,T1,U2,T2,U3,T3>::CallHandler() {
     (obj->*handler)(t1,t2,t3);
}

/** TickEvent4 subclasses TickEventBase, it defines a 
 *  4 parameter member function CallHandler for a TickEvent
 */
template <typename T,  typename OBJ,
          typename U1, typename T1,
          typename U2, typename T2,
          typename U3, typename T3,
          typename U4, typename T4>
class TickEvent4 : public TickEventBase {
public:

   /** Constructor
    *  @arg \c t Latency in ticks.
    *  @arg \c c Reference clock used with tick latency.
    *  @arg \c f 4 Parameter callback function pointer for tick event
    *  @arg \c obj0 Object the callback is called on.
    *  @arg \c t1_0 1st parameter in callback function list
    *  @arg \c t2_0 2nd parameter in callback function list 
    *  @arg \c t3_0 3rd parameter in callback function list
    *  @arg \c t4_0 4th parameter in callback function list            
    */    
   TickEvent4(Ticks_t t, Clock& c, void (T::*f)(U1, U2, U3, U4), OBJ *obj0, T1 t1_0, T2 t2_0, T3 t3_0, T4 t4_0)  
     : TickEventBase(t,c), handler(f), obj(obj0), t1(t1_0), t2(t2_0), t3(t3_0), t4(t4_0){}
   
   /** Defines the callback handler function pointer
    */    
   void (T::*handler)(U1, U2, U3, U4);
   
   /** Object the CallHandler uses for the callback function
    */
   OBJ* obj;

   /** 1st parameter in callback function list
    */   
   T1 t1;
   
   /** 2nd parameter in callback function list
    */   
   T2 t2;
   
   /** 3rd parameter in callback function list
    */   
   T3 t3;
   
   /** 4th parameter in callback function list
    */   
   T4 t4;
   
public:

   /** Calls the callback function when the event is processed.
    */  
   void CallHandler();
};

template <typename T,  typename OBJ,
          typename U1, typename T1,
          typename U2, typename T2,
          typename U3, typename T3,
          typename U4, typename T4> 
void TickEvent4<T,OBJ,U1,T1,U2,T2,U3,T3,U4,T4>::CallHandler() {
     (obj->*handler)(t1,t2,t3,t4);
}

// ----------------------------------------------------------------------- //
// Also need a variant of the Event0 that calls a static function,
// not a member function.

/** TickEvent0Stat subclasses TickEventBase, it defines a 
 *  tick event with a 0 parameter static callback function 
 *  rather than a object member function.
 */
class TickEvent0Stat : public TickEventBase
{
public:

  /** Constructor
   *  @arg \c t Latency in ticks.
   *  @arg \c c Reference clock for tick latency.
   *  @arg \c f 0 Parameter static callback function pointer.
   */
  TickEvent0Stat(Ticks_t t, Clock& c, void (*f)(void))
    : TickEventBase(t,c), handler(f){}
  
  /** Defines the static callback handler function pointer.
   */  
  void (*handler)(void);

public:

  /** Calls the static callback function when the event is processed.
   */  
  void CallHandler();
};

/** TickEvent1Stat subclasses TickEventBase, it defines a 
 *  tick event with a 1 parameter static callback function 
 *  rather than a object member function.
 */
template<typename U1, typename T1>
class TickEvent1Stat : public TickEventBase
{
public:

  /** Constructor
   *  @arg \c t Latency in ticks.
   *  @arg \c c Reference clock for tick latency.
   *  @arg \c f 1 Parameter static callback function pointer.
   *  @arg \c t1_0 1st parameter in callback function list   
   */
  TickEvent1Stat(Ticks_t t, Clock& c, void (*f)(U1), T1 t1_0)
    : TickEventBase(t,c), handler(f), t1(t1_0){}
    
  /** Defines the static callback handler function pointer.
   */     
  void (*handler)(U1);
  
  /** 1st parameter in callback function list
   */ 
  T1        t1;

public:

  /** Calls the static callback function when the event is processed.
   */ 
  void CallHandler();
};

template <typename U1, typename T1>
void TickEvent1Stat<U1, T1>::CallHandler()
{
  handler(t1);
}

/** TickEvent2Stat subclasses TickEventBase, it defines a 
 *  tick event with a 2 parameter static callback function 
 *  rather than a object member function.
 */
template<typename U1, typename T1, 
         typename U2, typename T2>
class TickEvent2Stat : public TickEventBase
{
public:

  /** Constructor
   *  @arg \c t Latency in ticks.
   *  @arg \c c Reference clock for tick latency.
   *  @arg \c f 2 Parameter static callback function pointer.
   *  @arg \c t1_0 1st parameter in callback function list
   *  @arg \c t2_0 2nd parameter in callback function list    
   */
  TickEvent2Stat(Ticks_t t, Clock& c, void (*f)(U1, U2), T1 t1_0, T2 t2_0)
    : TickEventBase(t,c), handler(f), t1(t1_0), t2(t2_0) {}
  
  /** Defines the static callback handler function pointer.
   */     
  void (*handler)(U1, U2);
  
  /** 1st parameter in static callback function list
   */   
  T1        t1;

  /** 2nd parameter in callback function list
   */   
  T2        t2;

public:

  /** Calls the static callback function when the event is processed.
   */ 
  void CallHandler();
};

template <typename U1, typename T1,
          typename U2, typename T2>
void TickEvent2Stat<U1, T1, U2, T2>::CallHandler()
{
  handler(t1, t2);
}

/** TickEvent3Stat subclasses TickEventBase, it defines a 
 *  tick event with a 3 parameter static callback function 
 *  rather than a object member function.
 */
template <typename U1, typename T1,
          typename U2, typename T2,
          typename U3, typename T3>
class TickEvent3Stat : public TickEventBase {
public:
  
   /** Constructor
    *  @arg \c t Latency in ticks.
    *  @arg \c c Reference clock for tick latency.
    *  @arg \c f 3 Parameter static callback function pointer.
    *  @arg \c t1_0 1st parameter in callback function list
    *  @arg \c t2_0 2nd parameter in callback function list 
    *  @arg \c t3_0 3rd parameter in callback function list    
    */
   TickEvent3Stat(Ticks_t t, Clock& c, void (*f)(U1, U2, U3), T1 t1_0, T2 t2_0, T3 t3_0)  
     : TickEventBase(t,c), handler(f), t1(t1_0), t2(t2_0), t3(t3_0) {}

   /** Defines the static callback handler function pointer.
    */      
   void (*handler)(U1, U2, U3);
   
   /** 1st parameter in static callback function list
    */    
   T1 t1;
   
   /** 2nd parameter in static callback function list
    */ 
   T2 t2;
   
   /** 3rd parameter in static callback function list
    */    
   T3 t3;
   
public:

   /** Calls the static callback function when the event is processed.
    */ 
   void CallHandler();
};

template <typename U1, typename T1,
          typename U2, typename T2,
          typename U3, typename T3> 
void TickEvent3Stat<U1,T1,U2,T2,U3,T3>::CallHandler()
{
  handler(t1,t2,t3);
}

/** TickEvent4Stat subclasses TickEventBase, it defines a 
 *  tick event with a 4 parameter static callback function 
 *  rather than a object member function.
 */
template <typename U1, typename T1,
          typename U2, typename T2,
          typename U3, typename T3,
          typename U4, typename T4>
class TickEvent4Stat : public TickEventBase {
public:

   /** Constructor
    *  @arg \c t Latency in ticks.
    *  @arg \c c Reference clock for tick latency.
    *  @arg \c f 4 Parameter static callback function pointer.
    *  @arg \c t1_0 1st parameter in callback function list
    *  @arg \c t2_0 2nd parameter in callback function list 
    *  @arg \c t3_0 3rd parameter in callback function list
    *  @arg \c t4_0 4th parameter in callback function list    
    */
   TickEvent4Stat(Ticks_t t, Clock& c, void (*f)(U1, U2, U3, U4), T1 t1_0, T2 t2_0, T3 t3_0, T4 t4_0)  
     : TickEventBase(t,c), handler(f), t1(t1_0), t2(t2_0), t3(t3_0), t4(t4_0){}

   /** Defines the static callback handler function pointer.
    */      
   void (*handler)(U1, U2, U3, U4);
   
   /** 1st parameter in static callback function list
    */    
   T1 t1;
   
   /** 2nd parameter in static callback function list
    */    
   T2 t2;
   
   /** 3rd parameter in static callback function list
    */    
   T3 t3;
   
   /** 4th parameter in static callback function list
    */    
   T4 t4;
   
public:

   /** Calls the static callback function when the event is processed.
    */ 
   void CallHandler();
};

template <typename U1, typename T1,
          typename U2, typename T2,
          typename U3, typename T3,
          typename U4, typename T4> 
void TickEvent4Stat<U1,T1,U2,T2,U3,T3,U4,T4>::CallHandler() {
     handler(t1,t2,t3,t4);
}

// ----------------------------------------------------------------------- //  

/** Defines the base event class for floating point events.
 */
class EventBase 
{
 public:
 
 /** Constructor
  *  @arg \c t Latency in time.
  */
 EventBase(double t) : time(t), uid(nextUID++) {}
 
 /** Constructor
  *  @arg \c t Latency in time.
  *  @arg \c u Unique id of the event. 
  */
 EventBase(double t, int u) : time(t), uid(u) {}
  
  /** Virtual function, all subclasses must implement CallHandler
   */   
  virtual void CallHandler() = 0;
  
 public:
 
      /** Timestamp for the event
       */
      double time;
      
      /** Each event has a uniques identifier to break timestamp ties
       */
      int    uid;
      
 private:
 
      /** Holds the next available unique id. 
       */
      static int nextUID;
};

/** EventId subclasses EventBase, and is the return type from 
 *  all schedule functions.  This is used to cancel the event.
 */
class EventId : public EventBase
{
public:

  /** Constructor
   *  @arg \c t Latency in time.
   *  @arg \c u Unique id of the event.
   */
  EventId(double t, int u) : EventBase(t, u) {}
  
  /** Calls the callback function when the event is processed.
   */ 
  void CallHandler() {}
};

/** Event0 defines an event object with a 
 *  0 parameter member function callback
 */
template<typename T, typename OBJ>
class Event0 : public EventBase
{
public:

  /** Constructor
   *  @arg \c t Latency in time.
   *  @arg \c f 0 Parameter member function callback pointer.
   *  @arg \c obj0 Object the callback function is called on.
   */
  Event0(double t, void (T::*f)(void), OBJ* obj0)
    : EventBase(t), handler(f), obj(obj0){}
  
  /** Defines the static callback handler function pointer.
   */        
  void (T::*handler)(void);
      
  /** Object the CallHandler uses for the callback function
   */
  OBJ*      obj;
  
public:

  /** Calls the callback function when the event is processed.
   */ 
  void CallHandler();
};

template <typename T, typename OBJ>
void Event0<T, OBJ>::CallHandler()
{
  (obj->*handler)();
}

/** Event1 defines an event object with a 
 *  1 parameter member function callback
 */
template<typename T, typename OBJ, typename U1, typename T1>
class Event1 : public EventBase
{
public:

  /** Constructor
   *  @arg \c t Latency in time.
   *  @arg \c f 1 Parameter member function callback pointer.
   *  @arg \c obj0 Object the callback function is called on.
   *  @arg \c t1_0 1st parameter in callback function list   
   */
  Event1(double t, void (T::*f)(U1), OBJ* obj0, T1 t1_0)
    : EventBase(t), handler(f), obj(obj0), t1(t1_0){}

  /** Defines the member function callback handler pointer.
   */     
  void (T::*handler)(U1);
  
  /** Object the CallHandler uses for the callback function
   */  
  OBJ*      obj;
  
  /** 1st parameter in callback function list  
   */  
  T1        t1;
  
public:

  /** Calls the callback function when the event is processed.
   */ 
  void CallHandler();
};

template <typename T, typename OBJ, typename U1, typename T1>
void Event1<T, OBJ, U1, T1>::CallHandler()
{
  (obj->*handler)(t1);
}

/** Event2 defines an event object with a 
 *  2 parameter member function callback
 */
template<typename T, typename OBJ,
         typename U1, typename T1, 
         typename U2, typename T2>
class Event2 : public EventBase
{
public:

  /** Constructor
   *  @arg \c t Latency in time.
   *  @arg \c f 2 Parameter member function callback pointer.
   *  @arg \c obj0 Object the callback function is called on.
   *  @arg \c t1_0 1st parameter in callback function list
   *  @arg \c t2_0 2nd parameter in callback function list    
   */
  Event2(double t, void (T::*f)(U1, U2), OBJ* obj0, T1 t1_0, T2 t2_0)
    : EventBase(t), handler(f), obj(obj0), t1(t1_0), t2(t2_0) {}

  /** Defines the member function callback handler pointer.
   */ 
  void (T::*handler)(U1, U2);
  
  /** Object the CallHandler uses for the callback function
   */   
  OBJ*      obj;

  /** 1st parameter in callback function list  
   */  
  T1        t1;
  
  /** 2nd parameter in callback function list  
   */  
  T2        t2;
  
public:

  /** Calls the callback function when the event is processed.
   */ 
  void CallHandler();
};

template <typename T, typename OBJ, 
          typename U1, typename T1,
          typename U2, typename T2>
void Event2<T, OBJ, U1, T1, U2, T2>::CallHandler()
{
  (obj->*handler)(t1, t2);
}

/** Event3 defines an event object with a 
 *  3 parameter member function callback
 */
template <typename T,  typename OBJ,
          typename U1, typename T1,
          typename U2, typename T2,
          typename U3, typename T3>
class Event3 : public EventBase {
public:

   /** Constructor
    *  @arg \c t Latency in time.
    *  @arg \c f 3 Parameter member function callback pointer.
    *  @arg \c obj0 Object the callback function is called on.
    *  @arg \c t1_0 1st parameter in callback function list
    *  @arg \c t2_0 2nd parameter in callback function list 
    *  @arg \c t3_0 3rd parameter in callback function list    
    */
   Event3(double t, void (T::*f)(U1, U2, U3), OBJ *obj0, T1 t1_0, T2 t2_0, T3 t3_0)  
     : EventBase(t), handler(f), obj(obj0), t1(t1_0), t2(t2_0), t3(t3_0) {}
     
   /** Defines the member function callback handler pointer.
    */      
   void (T::*handler)(U1, U2, U3);

   /** Object the CallHandler uses for the callback function
    */    
   OBJ* obj;
   
   /** 1st parameter in callback function list  
    */   
   T1 t1;
   
   /** 2nd parameter in callback function list  
    */   
   T2 t2;
   
   /** 3rd parameter in callback function list  
    */   
   T3 t3;
   
public:

   /** Calls the callback function when the event is processed.
    */ 
   void CallHandler();
};

template <typename T,  typename OBJ,
          typename U1, typename T1,
          typename U2, typename T2,
          typename U3, typename T3> 
void Event3<T,OBJ,U1,T1,U2,T2,U3,T3>::CallHandler() {
     (obj->*handler)(t1,t2,t3);
}

/** Event4 defines an event object with a 
 *  4 parameter member function callback
 */
template <typename T,  typename OBJ,
          typename U1, typename T1,
          typename U2, typename T2,
          typename U3, typename T3,
          typename U4, typename T4>
class Event4 : public EventBase {
public:

   /** Constructor
    *  @arg \c t Latency in time.
    *  @arg \c f 4 Parameter member function callback pointer.
    *  @arg \c obj0 Object the callback function is called on.
    *  @arg \c t1_0 1st parameter in callback function list
    *  @arg \c t2_0 2nd parameter in callback function list 
    *  @arg \c t3_0 3rd parameter in callback function list
    *  @arg \c t4_0 4th parameter in callback function list    
    */
   Event4(double t, void (T::*f)(U1, U2, U3, U4), OBJ *obj0, T1 t1_0, T2 t2_0, T3 t3_0, T4 t4_0)  
     : EventBase(t), handler(f), obj(obj0), t1(t1_0), t2(t2_0), t3(t3_0), t4(t4_0){}

   /** Defines the member function callback handler pointer.
    */      
   void (T::*handler)(U1, U2, U3, U4);
   
   /** Object the CallHandler uses for the callback function
    */    
   OBJ* obj;

   /** 1st parameter in callback function list  
    */     
   T1 t1;
   
   /** 2nd parameter in callback function list  
    */     
   T2 t2;
   
   /** 3rd parameter in callback function list  
    */     
   T3 t3;
   
   /** 4th parameter in callback function list  
    */     
   T4 t4;
   
public:

   /** Calls the callback function when the event is processed.
    */ 
   void CallHandler();
};

template <typename T,  typename OBJ,
          typename U1, typename T1,
          typename U2, typename T2,
          typename U3, typename T3,
          typename U4, typename T4> 
void Event4<T,OBJ,U1,T1,U2,T2,U3,T3,U4,T4>::CallHandler() {
     (obj->*handler)(t1,t2,t3,t4);
}

// Also need a variant of the Event0 that calls a static function,
// not a member function.

/** Event0Stat subclasses EventBase, it defines a 
 *  time event with a 0 parameter static callback function 
 *  rather than a object member function.
 */
class Event0Stat : public EventBase
{
public:

  /** Constructor
   *  @arg \c t Latency in time.
   *  @arg \c f 0 Parameter static callback function pointer.
   */
  Event0Stat(double t, void (*f)(void))
    : EventBase(t), handler(f){}

  /** Defines the static callback handler function pointer.
   */      
  void (*handler)(void);

public:

  /** Calls the callback function when the event is processed.
   */ 
  void CallHandler();
};

/** Event1Stat subclasses EventBase, it defines a 
 *  time event with a 1 parameter static callback function 
 *  rather than a object member function.
 */
template<typename U1, typename T1>
class Event1Stat : public EventBase
{
public:

  /** Constructor
   *  @arg \c t Latency in time.
   *  @arg \c f 1 Parameter static callback function pointer.
   *  @arg \c t1_0 1st parameter in callback function list   
   */
  Event1Stat(double t, void (*f)(U1), T1 t1_0)
    : EventBase(t), handler(f), t1(t1_0){}

  /** Defines the static callback handler function pointer.
   */          
  void (*handler)(U1);
  
  /** 1st parameter in callback function list  
   */   
  T1        t1;

public:

  /** Calls the callback function when the event is processed.
   */ 
  void CallHandler();
};

template <typename U1, typename T1>
void Event1Stat<U1, T1>::CallHandler()
{
  handler(t1);
}

/** Event2Stat subclasses EventBase, it defines a 
 *  time event with a 2 parameter static callback function 
 *  rather than a object member function.
 */
template<typename U1, typename T1, 
         typename U2, typename T2>
class Event2Stat : public EventBase
{
public:

  /** Constructor
   *  @arg \c t Latency in time.
   *  @arg \c f 2 Parameter static callback function pointer.
   *  @arg \c t1_0 1st parameter in callback function list
   *  @arg \c t2_0 2nd parameter in callback function list    
   */
  Event2Stat(double t, void (*f)(U1, U2), T1 t1_0, T2 t2_0)
    : EventBase(t), handler(f), t1(t1_0), t2(t2_0) {}
    
  /** Defines the static callback handler function pointer.
   */      
  void (*handler)(U1, U2);

  /** 1st parameter in callback function list  
   */   
  T1        t1;

  /** 2nd parameter in callback function list  
   */   
  T2        t2;

public:

  /** Calls the callback function when the event is processed.
   */ 
  void CallHandler();
};

template <typename U1, typename T1,
          typename U2, typename T2>
void Event2Stat<U1, T1, U2, T2>::CallHandler()
{
  handler(t1, t2);
}

/** Event3Stat subclasses EventBase, it defines a 
 *  time event with a 3 parameter static callback function 
 *  rather than a object member function.
 */
template <typename U1, typename T1,
          typename U2, typename T2,
          typename U3, typename T3>
class Event3Stat : public EventBase {
public:

   /** Constructor
    *  @arg \c t Latency in time.
    *  @arg \c f 3 Parameter static callback function pointer.
    *  @arg \c t1_0 1st parameter in callback function list
    *  @arg \c t2_0 2nd parameter in callback function list 
    *  @arg \c t3_0 3rd parameter in callback function list    
    */
   Event3Stat(double t, void (*f)(U1, U2, U3), T1 t1_0, T2 t2_0, T3 t3_0)  
     : EventBase(t), handler(f), t1(t1_0), t2(t2_0), t3(t3_0) {}

   /** Defines the static callback handler function pointer.
    */           
   void (*handler)(U1, U2, U3);

   /** 1st parameter in callback function list  
    */    
   T1 t1;
   
   /** 2nd parameter in callback function list  
    */    
   T2 t2;

   /** 3rd parameter in callback function list  
    */    
   T3 t3;
   
public:

   /** Calls the callback function when the event is processed.
    */ 
   void CallHandler();
};

template <typename U1, typename T1,
          typename U2, typename T2,
          typename U3, typename T3> 
void Event3Stat<U1,T1,U2,T2,U3,T3>::CallHandler()
{
  handler(t1,t2,t3);
}

/** Event4Stat subclasses EventBase, it defines a 
 *  time event with a 4 parameter static callback function 
 *  rather than a object member function.
 */
template <typename U1, typename T1,
          typename U2, typename T2,
          typename U3, typename T3,
          typename U4, typename T4>
class Event4Stat : public EventBase {
public:

   /** Constructor
    *  @arg \c t Latency in time.
    *  @arg \c f 4 Parameter static callback function pointer.
    *  @arg \c t1_0 1st parameter in callback function list
    *  @arg \c t2_0 2nd parameter in callback function list 
    *  @arg \c t3_0 3rd parameter in callback function list
    *  @arg \c t4_0 4th parameter in callback function list    
    */
   Event4Stat(double t, void (*f)(U1, U2, U3, U4), T1 t1_0, T2 t2_0, T3 t3_0, T4 t4_0)  
     : EventBase(t), handler(f), t1(t1_0), t2(t2_0), t3(t3_0), t4(t4_0){}

   /** Defines the static callback handler function pointer.
    */      
   void (*handler)(U1, U2, U3, U4);
   
   /** 1st parameter in callback function list  
    */    
   T1 t1;

   /** 2nd parameter in callback function list  
    */    
   T2 t2;
   
   /** 3rd parameter in callback function list  
    */    
   T3 t3;

   /** 4th parameter in callback function list  
    */    
   T4 t4;
   
public:

   /** Calls the callback function when the event is processed.
    */ 
   void CallHandler();
};

template <typename U1, typename T1,
          typename U2, typename T2,
          typename U3, typename T3,
          typename U4, typename T4> 
void Event4Stat<U1,T1,U2,T2,U3,T3,U4,T4>::CallHandler() 
{
     handler(t1,t2,t3,t4);
}

/** Defines the event comparator.
 */
/*
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
*/

/** Defines the type for the sorted event list.
 */
//typedef std::set<EventBase*, event_less> EventSet_t;




} //namespace kernel
} //namespace manifold


#endif // MANIFOLD_KERNEL_MANIFOLD_EVENT_H

