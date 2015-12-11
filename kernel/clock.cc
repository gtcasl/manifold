// Implementation of Clock objects for Manifold
// George F. Riley, (and others) Georgia Tech, Fall 2010

#include <list>
#include <cstdlib>

#include "clock.h"
#include "manifold.h"

using namespace std;

namespace manifold {
namespace kernel {

// Map of all clocks
Clock::ClockVec_t* Clock::clocks = 0;

Clock::Clock(double f) : period(1/f), freq(f), nextRising(true), nextTick(0),
                         calendar(CLOCK_CALENDAR_LENGTH, EventVec_t())
{
  if (!clocks)
    {
      clocks = new ClockVec_t;
    }

  clocks->push_back(this);

    stats = new Clock_stat_engine();
}

Clock::~Clock()
{
    delete stats;
}

void Clock::Rising()
{ // Call rising edge function on all registered objects
  list<tickObjBase*>::iterator iter;
  for(iter=tickObjs.begin(); iter!=tickObjs.end(); iter++)
    {
      tickObjBase* to = *iter;
      if (to->enabled) {
        to->CallRisingTick();
	#ifdef STATS
	stats->registered_events++;
	#endif
      }
    }
}

void Clock::Falling()
{ // Call falling edge function on all registered objects
  list<tickObjBase*>::iterator iter;
  for(iter=tickObjs.begin(); iter!=tickObjs.end(); iter++)
    {
      tickObjBase* to = *iter;
      if (to->enabled) {
        to->CallFallingTick();
      }
    }
}

TickEventId Clock::Insert(TickEventBase* ev)
{
  Ticks_t when = nextTick + ev->time;
  // First make sure it's in range for the calendar queue
  if (ev->time >= CLOCK_CALENDAR_LENGTH)
    { // Just insert into the sorted queue
      // The sorted queue uses units of half-ticks
      when = when * 2;
      if (!ev->rising) when++;
      ev->time = when; // Convert the time in the event to half ticks
      events.insert(make_pair(when, ev));
    }
  else
    { // Add to calendar
      ev->time = when;  // Convert ticks time to absolute from relative
      size_t index = when % CLOCK_CALENDAR_LENGTH;
      EventVec_t& eventVec = calendar[index];
      eventVec.push_back(ev);
    }
  return TickEventId((Ticks_t)ev->time,(int)ev->uid,(Clock&)*this);
}

TickEventId Clock::InsertHalf(TickEventBase* ev)
{ // Time is in  units of half-ticks.  If odd, toggle rising/falling
/*
  Ticks_t when = nextTick + ev->time;
  if (when & 0x01) ev->rising = !nextRising;
  // And convert time to full tick units
  ev->time = ev->time / 2;
  return Insert(ev);
*/
  Ticks_t thisTick = nextTick * 2;
  if (!nextRising) thisTick++;   //thisTick is the number of half ticks
  Ticks_t when = thisTick + ev->time;
  if (ev->time & 0x01) ev->rising = !nextRising;
  else ev->rising = nextRising;

  // And convert time to RELATIVE (relative to now, i.e, nextTick) full tick units
  ev->time = when / 2 - nextTick;
  return Insert(ev);
}

void Clock::Cancel(TickEventId)
{ // Implement later
}

Time_t Clock::NextTickTime() const
{
  Time_t time = (Time_t)nextTick / freq;
  if (!nextRising) time += period/ 2.0; // The falling is 1/2 period later
  return time;
}

Ticks_t Clock::NowTicks() const
{
  return nextTick;
}

Ticks_t Clock::NowHalfTicks() const
{
  return nextTick * 2 + (nextRising ? 0 : 1);
}

void Clock::ProcessThisTick()
{
  // First see if any events in the sorted queue need processing
  if (!events.empty())
    {
      // This structure uses half-tick units, so we can mix and
      // match rising and falling edge events
      Ticks_t thisTick = nextTick * 2;
      if (!nextRising) thisTick++;
      //while(true)
      while(events.size() > 0)
        {
          TickEventBase* ev = events.begin()->second;
          if (ev->time > thisTick) break; // Not time for this event
          assert(ev->time==thisTick);
          // Process the event
          ev->CallHandler();
          // Delete the event
          delete ev;
          // Remove from queue
          events.erase(events.begin());

	  #ifdef STATS
	  stats->queued_events++;
	  #endif
        }
    }
  // Now process all events
  Ticks_t thisIndex = nextTick % CLOCK_CALENDAR_LENGTH;
  EventVec_t& events = calendar[thisIndex];


  for (size_t i = 0; i < events.size(); ++i)
    {
      TickEventBase* ev = events[i];
      if (!ev) continue; // Already processed
      // make sure the event matches the rising/falling
      if (!ev->rising && nextRising) continue;
      if (ev->rising && !nextRising) continue;
      assert(ev->time==nextTick);
      // Process the event
      ev->CallHandler();

      #ifdef STATS
      if(nextRising)
	  stats->rising_calendar_events++;
      else
	  stats->falling_calendar_events++;
      #endif

      delete ev;
      events[i] = nil;
    }

  if (!nextRising)
    {
      // Clear these events from the vector
      events.clear();
    }
  // Finally call the Tick function on all registered objects.
  if (nextRising)
    {
      Rising();
      nextRising = false;
    }
  else
    {
      Falling();
      // After falling edge, advance to next tick time
      nextRising = true;
      nextTick++;
    }
}

// Static functions
Clock& Clock::Master()
{
  if (!clocks)
    {
      cout << "FATAL ERROR, Clock::Master called with no clocks" << endl;
      exit(1);
    }
  return *((*clocks)[0]);
}

Ticks_t Clock::Now()
{
  Clock& c = Master();
  return c.NowTicks();
}

Ticks_t Clock::NowHalf()
{
  Clock& c = Master();
  return c.NowHalfTicks();
}

Ticks_t Clock::Now(Clock& c)
{
  return c.NowTicks();
}

Ticks_t Clock::NowHalf(Clock& c)
{
  return c.NowHalfTicks();
}

Clock::ClockVec_t& Clock::GetClocks()
{
  if (!clocks)
    {
      clocks = new ClockVec_t;
    }
  return *clocks;
}


void Clock :: print_stats(ostream& out)
{
    out << "cycles: " << NowTicks() << endl
        << "registered objects: " << tickObjs.size() << endl;
    stats->print_stats(out);
}


void Clock :: terminate()
{
    //may also need to clear the calendar.
    disableAll();
}


void Clock :: disableAll()
{
  for(list<tickObjBase*>::iterator iter=tickObjs.begin(); iter!=tickObjs.end(); ++iter) {
      (*iter)->Disable();
  }
}


void Clock :: do_output_prediction()
{
    if(output_predictors.size() > 0) {
        for(list<tickObjBase*>::iterator it=output_predictors.begin(); it!=output_predictors.end(); ++it) {
	    (*it)->CallRisingTick();
        }
    }
}


#ifdef KERNEL_UTEST
void Clock :: Reset()
{
    Clock::ClockVec_t& clocks = GetClocks();
    for (size_t c = 0; c < clocks.size(); ++c) {
        Clock* clk = clocks[c];

        //when simulation restarts, clock should be at rising edge
        clk->nextRising = true;
	clk->nextTick = 0;

	//clear future list
	for(EventMap_t::iterator it = clk->events.begin(); it != clk->events.end(); ++it) {
	    delete (*it).second;
	}
        clk->events.clear();

	//clear calendar
	for(size_t i=0; i<clk->calendar.size(); i++) {
	    for(size_t j=0; j<clk->calendar[i].size(); j++) {
		delete clk->calendar[i][j];
	    }
	    clk->calendar[i].clear();
	}

	//clear registered components
	clk->tickObjs.clear();
    }
}
#endif


Clock_stat_engine::Clock_stat_engine () : Stat_engine(),
queued_events("queued events", ""),
rising_calendar_events("rising calendar events", ""),
falling_calendar_events("failing calengar events", ""),
registered_events("registered events", "")
{
}

Clock_stat_engine::~Clock_stat_engine ()
{
}

void Clock_stat_engine::global_stat_merge(Stat_engine * e)
{
    Clock_stat_engine * global_engine = (Clock_stat_engine*)e;
    global_engine->queued_events += queued_events.get_value();
    global_engine->rising_calendar_events += rising_calendar_events.get_value();
    global_engine->falling_calendar_events += falling_calendar_events.get_value();
    global_engine->registered_events += registered_events.get_value();
}

void Clock_stat_engine::print_stats (ostream & out)
{
    queued_events.print(out);
    rising_calendar_events.print(out);
    falling_calendar_events.print(out);
    registered_events.print(out);
}

void Clock_stat_engine::clear_stats()
{
    queued_events.clear();
    rising_calendar_events.clear();
    falling_calendar_events.clear();
    registered_events.clear();
}

void Clock_stat_engine::start_warmup () 
{
}

void Clock_stat_engine::end_warmup () 
{
}

void Clock_stat_engine::save_samples () 
{
}




DVFSClock :: DVFSClock(double f) : Clock(f)
{
    m_lastChangeTick = 0;
    m_lastChangeTime = 0;
}


Time_t DVFSClock::NextTickTime() const
{
    Time_t time = (Time_t)(nextTick - m_lastChangeTick) / freq + m_lastChangeTime;
    if (!nextRising) time += period/ 2.0; // The falling is 1/2 period later
    return time;
}


void DVFSClock :: set_frequency(double f) throw (MultipleFreqChangeException)
{
    if(nextTick != m_lastChangeTick) {
	m_lastChangeTime = (nextTick - m_lastChangeTick) / freq + m_lastChangeTime;
	m_lastChangeTick = nextTick;
	//set new frequency
	freq = f;
	period = 1.0/freq;
    }
    else
	throw MultipleFreqChangeException();
}




} //namespace kernel
} //namespace manifold


