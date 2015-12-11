#include <stdlib.h>

#include <iostream>

#include "scheduler.h"
#ifndef NO_MPI
#include "messenger.h"
#endif

#include "clock.h"
#include "component.h"
#include <sys/time.h>

using namespace std;

namespace manifold {
namespace kernel {

//====================================================================
//====================================================================
Scheduler :: Scheduler() : m_halted(false), m_simTime(0), m_terminate_initiated(false)
{
    #ifndef NO_MPI
    m_syncAlg = 0;
    #endif
    stats = new Scheduler_stat_engine();
}


Scheduler :: ~Scheduler()
{
    delete stats;
}


void Scheduler :: print_stats(ostream& out)
{
    #ifndef NO_MPI
    if(m_syncAlg) { //in parallel sim, if NP==1, then sequential algo is used and m_syncAlg is 0
	m_syncAlg->PrintStats(out);
	TheMessenger.print_stats(out);
    }
    #endif
}


void Scheduler :: terminate()
{
    m_halted = true;
    m_terminate_initiated = true;
}


#ifndef NO_MPI
void Scheduler::UpdateLookahead(Time_t delay, LpId_t src, LpId_t dst)
{
    m_syncAlg->UpdateLookahead(delay,src,dst);
}


#ifdef FORECAST_NULL
void Scheduler :: updateOutputTick(LpId_t src, LpId_t dst, Ticks_t when, Clock* clk)
{
    m_syncAlg->updateOutputTick(src, dst, when, clk);
}
#endif

#endif


//====================================================================
//====================================================================
void Scheduler :: scheduleTimedEvent(EventBase* ev)
{
    m_timedEvents.insert(ev);
}

//====================================================================
//====================================================================
bool Scheduler :: cancelTimedEvent(EventId& evid)
{
    EventSet_t::iterator it = m_timedEvents.find(&evid);
    if (it == m_timedEvents.end()) return false; // Not found
    m_timedEvents.erase(it);              // Otherwise erase it
    return true;
}


//====================================================================
//====================================================================
EventId Scheduler :: peek()
{
    // Return eventid for earliest event, but do not remove it
    // Event list must not be empty
    EventSet_t::iterator it = m_timedEvents.begin();
    return EventId((*it)->time, (*it)->uid);
}


//====================================================================
//====================================================================
EventBase* Scheduler :: GetEarliestEvent()
{
    EventSet_t::iterator it = m_timedEvents.begin();
    if (it == m_timedEvents.end()) return 0;
    return *it;
}


#ifndef NO_MPI
//if parallel simulation
void Scheduler::AddPredecessorLp(LpId_t lp)
{
    for(size_t i=0;i<predecessors.size();++i) {
	if(lp==predecessors[i]) return;
    }
    predecessors.push_back(lp);
    //m_neighborVersion++;
}

void Scheduler::AddSuccessorLp(LpId_t lp)
{
    for(size_t i=0;i<successors.size();++i) {
	if(lp==successors[i]) return;
    }
    successors.push_back(lp);
    //m_neighborVersion++;
}


//====================================================================
//====================================================================
void Scheduler :: handle_incoming_messages()
{
    int received = 0;
    do {
        Message_s& msg = TheMessenger.irecv_message(&received);
	if(received != 0) {
	    Component* comp = Component :: GetComponent<Component>(msg.compIndex);
	    switch(msg.type) {
	        case Message_s :: M_UINT32:
		    comp->Recv_remote(msg.inputIndex, msg.sendTick, msg.recvTick,
		                      msg.sendTime, msg.recvTime, msg.uint32_data);
		    break;
	        case Message_s :: M_UINT64:
		    comp->Recv_remote(msg.inputIndex, msg.sendTick, msg.recvTick,
		                      msg.sendTime, msg.recvTime, msg.uint64_data);
		    break;
	        case Message_s :: M_SERIAL:
		    comp->Recv_remote(msg.inputIndex, msg.sendTick, msg.recvTick,
		                      msg.sendTime, msg.recvTime, msg.data, msg.data_len);
		    break;
		default:
		    std::cerr << "unknown message type" << std::endl;
		    exit(1);
	    }//switch
	}
    }while(received != 0);
}
#endif //#ifndef NO_MPI




//Define macros because the code is used in more than 1 place.

#define GET_NEXT_TICK_TIME \
    Clock::ClockVec_t& clocks = Clock::GetClocks(); \
    Clock* nextClock = clocks[0]; \
    Time_t nextClockTime = clocks[0]->NextTickTime(); \
    \
    for (size_t i = 1; i < clocks.size(); ++i) { \
	if (clocks[i]->NextTickTime() < nextClockTime) { \
	    nextClock = clocks[i]; \
	    nextClockTime = nextClock->NextTickTime(); \
	} \
    } \
    \
    assert(nextClock);


#define GET_NEXT_TICK_OR_EVENT \
    EventBase* nextEvent = nil; \
    if (!m_timedEvents.empty()) { \
       nextEvent = *m_timedEvents.begin(); \
    } \
    \
    Clock* nextClock = nil; \
    Time_t nextClockTime = INFINITY; \
    \
    Clock::ClockVec_t& clocks = Clock::GetClocks(); \
    for (size_t i = 0; i < clocks.size(); ++i) { \
        if (!nextClock) { \
            nextClock = clocks[i]; \
            nextClockTime = nextClock->NextTickTime(); \
        } \
        else { \
	    if (clocks[i]->NextTickTime() < nextClockTime) { \
                nextClock = clocks[i]; \
                nextClockTime = nextClock->NextTickTime(); \
	    } \
	} \
    } \
    if (nextEvent == nil && nextClock == nil) { \
        m_halted = true; \
	break; \
    } \
    \
    \
    double nextTime = nextClockTime; \
    bool clockIsNext = true;  \
    if(nextEvent != nil && nextEvent->time < nextClockTime) { \
	nextTime = nextEvent->time; \
	clockIsNext = false; \
    }



//####################################################################
// Sequential schedulers
//####################################################################

void Seq_TickedScheduler::Run()
{
    while(!m_halted) {
        // Next we  need to find the clock object with the next earliest tick
	GET_NEXT_TICK_TIME; 

        {
            assert(nextClockTime>=m_simTime);
            m_simTime = nextClockTime;
            nextClock->ProcessThisTick();
        }
    }//while(!m_halted)
}



//====================================================================
//====================================================================
void Seq_TimedScheduler::Run()
{
    while(!m_halted) {
        // Get the time of the next timed event
        EventBase* nextEvent = nil;
        if (!m_timedEvents.empty())
            nextEvent = *m_timedEvents.begin();

        // If no events found, we are done
        if (nextEvent == nil) {
            m_halted = true; //ToDo: this is too simplistic.
	    break;
        }

        {
            // Set the simulation time
            assert(nextEvent->time>=m_simTime);
            m_simTime = nextEvent->time;
            // Call the event handler
            nextEvent->CallHandler();
            // Remove the event from the pending list
            m_timedEvents.erase(m_timedEvents.begin());
            // And delete the event
            delete nextEvent;
        }
    }//while(!m_halted)

}




//====================================================================
//====================================================================
void Seq_MixedScheduler::Run()
{
    while(!m_halted) {

        // Get the time of the next event
	GET_NEXT_TICK_OR_EVENT;

        {
	    assert(nextTime >= m_simTime);
	    // Set the simulation time
	    m_simTime = nextTime;

            // Now process either the next event or next clock tick
            // depending on which one is earliest
            if(clockIsNext) { // Process clock event
                nextClock->ProcessThisTick();
            }
            else { // Process timed event
                // Call the event handler
                nextEvent->CallHandler();
                // Remove the event from the pending list
                m_timedEvents.erase(m_timedEvents.begin());
                // And delete the event
                delete nextEvent;
            }
        }
    }//while(!m_halted)

}









#ifndef NO_MPI
//####################################################################
// LBTS schedulers
//####################################################################

//====================================================================
//====================================================================
LBTS_TickedScheduler :: LBTS_TickedScheduler(Lookahead::LookaheadType_t lookaheadType)
{
    m_syncAlg = new LbtsSyncAlg(lookaheadType);
}


//====================================================================
//====================================================================
void LBTS_TickedScheduler::Run()
{
    LbtsSyncAlg* LBTS = dynamic_cast<LbtsSyncAlg*>(m_syncAlg);
    assert(LBTS);

    TheMessenger.barrier();

    while(!m_halted) {
        handle_incoming_messages(); //check incoming messages

        // Next we  need to find the clock object with the next earliest tick
	GET_NEXT_TICK_TIME; 

        if(LBTS->isSafeToProcess(nextClockTime))
        {
            assert(nextClockTime>=m_simTime);
            m_simTime = nextClockTime;
            nextClock->ProcessThisTick();
        }//safeToProcess
      // if not safe to process event, go back to beginning of loop to receive events
    }//while(!m_halted)


    if(m_terminate_initiated) {
	LBTS->terminateInitiated();
	m_terminate_initiated = false;
    }
}








//====================================================================
//====================================================================
LBTS_TimedScheduler :: LBTS_TimedScheduler(Lookahead::LookaheadType_t lookaheadType)
{
    m_syncAlg = new LbtsSyncAlg(lookaheadType);
}

//====================================================================
//====================================================================
void LBTS_TimedScheduler::Run()
{

    LbtsSyncAlg* LBTS = dynamic_cast<LbtsSyncAlg*>(m_syncAlg);
    assert(LBTS);

    TheMessenger.barrier();

    while(!m_halted) {
        //check incoming messages
        handle_incoming_messages();

        // Get the time of the next timed event
        EventBase* nextEvent = nil;
        if (!m_timedEvents.empty())
            nextEvent = *m_timedEvents.begin();

        // If no events found, we are done
        if (nextEvent == nil) {
            m_halted = true; //ToDo: this is too simplistic.
	    break;
        }

        if(LBTS->isSafeToProcess(nextEvent->time))
        {
            // Set the simulation time
            assert(nextEvent->time>=m_simTime);
            m_simTime = nextEvent->time;
            // Call the event handler
            nextEvent->CallHandler();
            // Remove the event from the pending list
            m_timedEvents.erase(m_timedEvents.begin());
            // And delete the event
            delete nextEvent;
        }//safeToProcess
        // if not safe to process event, go back to beginning of loop to receive events
    }//while(!m_halted)


    if(m_terminate_initiated) {
	LBTS->terminateInitiated();
	m_terminate_initiated = false;
    }
}









//====================================================================
//====================================================================
LBTS_MixedScheduler :: LBTS_MixedScheduler(Lookahead::LookaheadType_t lookaheadType)
{
    m_syncAlg = new LbtsSyncAlg(lookaheadType);
}

//====================================================================
//====================================================================
void LBTS_MixedScheduler::Run()
{
    LbtsSyncAlg* LBTS = dynamic_cast<LbtsSyncAlg*>(m_syncAlg);
    assert(LBTS);

    TheMessenger.barrier();

    while(!m_halted) {
        //check incoming messages
        handle_incoming_messages();

        // Get the time of the next event
	GET_NEXT_TICK_OR_EVENT;


        if(LBTS->isSafeToProcess(nextTime))
        {
	    assert(nextTime >= m_simTime);
	    // Set the simulation time
	    m_simTime = nextTime;

            // Now process either the next event or next clock tick
            // depending on which one is earliest
            if(clockIsNext) { // Process clock event
                nextClock->ProcessThisTick();
            }
            else { // Process timed event
                // Call the event handler
                nextEvent->CallHandler();
                // Remove the event from the pending list
                m_timedEvents.erase(m_timedEvents.begin());
                // And delete the event
                delete nextEvent;
            }
        }//safeToProcess
      // if not safe to process event, go back to beginning of loop to receive events
    }//while(!m_halted)


    if(m_terminate_initiated) {
	LBTS->terminateInitiated();
	m_terminate_initiated = false;
    }
}















//####################################################################
// CMB schedulers
//####################################################################

//====================================================================
//====================================================================
CMB_Scheduler :: CMB_Scheduler()
{
    stats_num_fini = 0;
    stats_num_stop = 0;
    stats_num_end = 0;
    stats_blocking_time = 0;
}

//One problem that has to be solved is proper termination. In addition to the
//normal termination, where all LPs process the same Stop event and then exit
//the main loop, there are other cases that may trigger termination. Here are
//2 examples:
// 1. In a ticked simulation, one processor model runs out of instructions and
//    initiates termination.
// 2. In a timed simulation, one LP runs out of events and exits the main loop.
//
// If an LP exits the main loop without notifying others, the program will hang.
// Here is how we handle termination.
// 1. When a process i exits the main loop for any reason, it notifies process 0
//    by sending a FINI message. Of course, if i==0, a FINI message is not necessary.
//    Process i then enters a loop where it receives messages but doesn't process
//    them unless the messages are for the termination protocol.
//    The reason why process i needs to receive messages is to prevent the sender
//    from blocking.
// 2. Process 0 on receiving the first FINI, or itself is the first one to exit,
//    sends each process a STOP message.
// 3. Process i != 0, when receiving a STOP, will exit the main loop, if it hasn't
//    exited already. If it already exited, the STOP message is ignored.
// 4. When process 0 receives a FINI from all other processes, it sends an END message
//    to all processes, and then terminates the program.
// 5. Process i != 0 receives the END message and terminates the program.

//====================================================================
//====================================================================
void CMB_Scheduler :: CMB_handle_incoming_messages()
{
    int received = 0;
    do {
        Message_s& msg = TheMessenger.irecv_message(&received);
	if(received != 0) {
	    Component* comp = 0;
	    if(msg.type != Message_s::M_PROTO1)
		comp = Component :: GetComponent<Component>(msg.compIndex);
	    switch(msg.type) {
	        case Message_s :: M_UINT32:
		    comp->Recv_remote(msg.inputIndex, msg.sendTick, msg.recvTick,
		                      msg.sendTime, msg.recvTime, msg.uint32_data);
		    break;
	        case Message_s :: M_UINT64:
		    comp->Recv_remote(msg.inputIndex, msg.sendTick, msg.recvTick,
		                      msg.sendTime, msg.recvTime, msg.uint64_data);
		    break;
	        case Message_s :: M_SERIAL:
		    comp->Recv_remote(msg.inputIndex, msg.sendTick, msg.recvTick,
		                      msg.sendTime, msg.recvTime, msg.data, msg.data_len);
		    break;
	        case Message_s :: M_PROTO1:
		    handle_proto1(msg);
		    break;
		default:
		    std::cerr << "unknown message type" << std::endl;
		    exit(1);
	    }//switch
	}
    }while(received != 0);
}


//====================================================================
//====================================================================
void CMB_Scheduler :: handle_proto1(Message_s& msg)
{
    int type = (int)(msg.uint32_data);

    switch(type) {
        case MSG_FINI:
	    assert(TheMessenger.get_node_id() == 0);
	    #ifdef STATS
	    stats_num_fini++;
	    #endif

	    m_num_fini++;
	    if(m_num_fini == 1) { //The first FINI
		TheMessenger.broadcast_proto1(MSG_STOP, 0);
		m_halted = true;
	    }
	    else if(m_num_fini == TheMessenger.get_node_size()) {
	        //all nodes have exited the main loop
		TheMessenger.broadcast_proto1(MSG_END, 0);
		m_end = true;
	    }
	    break;
        case MSG_STOP:
	    assert(TheMessenger.get_node_id() != 0);
	    #ifdef STATS
	    stats_num_stop++;
	    #endif
	    m_halted = true;
	    break;
        case MSG_END:
	    assert(TheMessenger.get_node_id() != 0);
	    #ifdef STATS
	    stats_num_end++;
	    #endif
	    m_end = true;
	    break;
    }
}


//====================================================================
// This is called to receive msgs so the sender doesn't block.
// We don't process the messages unless the messages are signals.
//====================================================================
void CMB_Scheduler :: CMB_recv_incoming_messages()
{
    int received = 0;
    do {
        Message_s& msg = TheMessenger.irecv_message(&received);
	if(received != 0) {
	    switch(msg.type) {
	        case Message_s :: M_UINT32:
	        case Message_s :: M_UINT64:
	        case Message_s :: M_SERIAL:
		    break;
	        case Message_s :: M_PROTO1:
		    handle_proto1(msg);
		    break;
		default:
		    std::cerr << "unknown message type" << std::endl;
		    exit(1);
	    }//switch
	}
    }while(received != 0);

    //Receive NULL message too to prevent sender from blocking
    //if(m_end == false) //this if maybe correct, but won't gain much with it.
    TheMessenger.RecvPendingNullMsg();
}


//====================================================================
//====================================================================
void CMB_Scheduler :: post_main_loop()
{
    CmbSyncAlg* CMB = dynamic_cast<CmbSyncAlg*>(m_syncAlg);
    CMB->send_null_msgs();

    m_end = false;

    if(TheMessenger.get_node_id() == 0) {
        m_num_fini++; //count self
	if(m_num_fini == 1) { //The first to exit main loop
	    TheMessenger.broadcast_proto1(MSG_STOP, 0);
	} //with 2 LPs, LP 0 could be the last one to exit main loop!
	else if(m_num_fini == TheMessenger.get_node_size()) {
	    //all nodes have exited the main loop
	    TheMessenger.broadcast_proto1(MSG_END, 0);
	    m_end = true;
	}
    }
    else {
	TheMessenger.send_proto1_msg(0, MSG_FINI); //tell node 0 we have exited main loop
    }


    while(!m_end) {
        CMB_recv_incoming_messages();
    }

    TheMessenger.barrier();
}


//====================================================================
//====================================================================
void CMB_Scheduler :: print_stats(std::ostream& out)
{
    Scheduler :: print_stats(out);
    out << "FINI: " << stats_num_fini << endl;
    out << "STOP: " << stats_num_stop << endl;
    out << "END: " << stats_num_end << endl;
    out << "blocking time: " << stats_blocking_time << " usec" << endl;
}




//====================================================================
//====================================================================
CMB_TickedScheduler :: CMB_TickedScheduler(Lookahead::LookaheadType_t lookaheadType)
{
    m_syncAlg = new CmbSyncAlg(lookaheadType);
}


//====================================================================
//====================================================================
void CMB_TickedScheduler::Run()
{
    CmbSyncAlg* CMB = dynamic_cast<CmbSyncAlg*>(m_syncAlg);
    assert(CMB);

    TheMessenger.barrier();

    m_num_fini = 0; //number of nodes that have exited main loop

    while(!m_halted) {
        // Next we  need to find the clock object with the next earliest tick
	GET_NEXT_TICK_TIME; 


        if(CMB->isSafeToProcess(nextClockTime))
        {
            assert(nextClockTime>=m_simTime);
            m_simTime = nextClockTime;
            nextClock->ProcessThisTick(); //note m_halted may be set to true as a result
	    if(m_halted)
		break;
        }
        else {
	    //send null messages
	    CMB->send_null_msgs();
	}

        CMB_handle_incoming_messages(); //check incoming messages

    }//while(!m_halted)

    post_main_loop();

}




//====================================================================
//====================================================================
CMB_TimedScheduler :: CMB_TimedScheduler(Lookahead::LookaheadType_t lookaheadType)
{
    m_syncAlg = new CmbSyncAlg(lookaheadType);
}

//====================================================================
//====================================================================
void CMB_TimedScheduler::Run()
{
    CmbSyncAlg* CMB = dynamic_cast<CmbSyncAlg*>(m_syncAlg);
    assert(CMB);

    TheMessenger.barrier();

    m_num_fini = 0; //number of nodes that have exited main loop

    while(!m_halted) {

        // Get the time of the next timed event
        EventBase* nextEvent = nil;
        if (!m_timedEvents.empty())
            nextEvent = *m_timedEvents.begin();

        // If no events found, we are done
        if (nextEvent == nil) {
            m_halted = true; //ToDo: this is too simplistic.
	    break;
        }

        if(CMB->isSafeToProcess(nextEvent->time))
        {
            // Set the simulation time
            assert(nextEvent->time>=m_simTime);
            m_simTime = nextEvent->time;
            // Call the event handler
            nextEvent->CallHandler();
            // Remove the event from the pending list
            m_timedEvents.erase(m_timedEvents.begin());
            // And delete the event
            delete nextEvent;

	    if(m_halted)
	        break;
        }//safeToProcess
	else {
	    //send null messages
	    CMB->send_null_msgs();
	}

        //check incoming messages
        CMB_handle_incoming_messages();

    }//while(!m_halted)

    post_main_loop();

}





//====================================================================
//====================================================================
CMB_MixedScheduler :: CMB_MixedScheduler(Lookahead::LookaheadType_t lookaheadType)
{
    m_syncAlg = new CmbSyncAlg(lookaheadType);
}

//====================================================================
//====================================================================
void CMB_MixedScheduler::Run()
{
    CmbSyncAlg* CMB = dynamic_cast<CmbSyncAlg*>(m_syncAlg);
    assert(CMB);

    TheMessenger.barrier();

    m_num_fini = 0; //number of nodes that have exited main loop

    while(!m_halted) {
        // Get the time of the next event
	GET_NEXT_TICK_OR_EVENT;

        if(CMB->isSafeToProcess(nextTime))
        {
	    assert(nextTime >= m_simTime);
	    // Set the simulation time
	    m_simTime = nextTime;

            // Now process either the next event or next clock tick
            // depending on which one is earliest
            if(clockIsNext) { // Process clock event
                nextClock->ProcessThisTick();
            }
            else { // Process timed event
                // Call the event handler
                nextEvent->CallHandler();
                // Remove the event from the pending list
                m_timedEvents.erase(m_timedEvents.begin());
                // And delete the event
                delete nextEvent;
            }
	    if(m_halted)
	        break;
        }//safeToProcess
	else {
	    //send null messages
	    CMB->send_null_msgs();
	}

        //check incoming messages
        CMB_handle_incoming_messages();

    }//while(!m_halted)


    post_main_loop();
}


//####################################################################
// CMB optimized for tick-based LPs
// Delays of all the output links from the LP must be specified in ticks.
// Otherwise this should not be used.
//
// There is also this subtle problem. Suppose the system has two LPs: LP0 and LP1.
// There is one link between the two LPs, and it uses Clock1 (4hz). There is another
// clock, Clock2 (10hz), but it's not used in any inter-LP links.
//
// Assume an event is scheduled for rising edge of cycle 11 of Clock1 (4hz), i.e., at
// time 2.75, and it's arrival time is 3.0 (delay is one cycle, 0.25).
// Note time 2.75 happens to be the falling edge of cycle 27 of Clock2 (10hz).
// Assume at time 2.75, Clock2 is picked first. It's not safe, so it sends null
// messages with timestamp = 2.80 + 0.25*0.9 = 3.025. When Clock1's cycle 11
// is processed, it sends an message with arrival time 3.0, which is less than 3.025.
//
// Conclusions: when there are multiple links between two LPs and they are associated
// with different clocks, there might be some problem with this scheduler.
//####################################################################

//====================================================================
//====================================================================
CMB_TickedOptScheduler :: CMB_TickedOptScheduler(Lookahead::LookaheadType_t lookaheadType)
{
    m_syncAlg = new CmbSyncAlg(lookaheadType);
}



//====================================================================
//====================================================================
//#define GET_BLOCKING_TIME  //measure blocking time

void CMB_TickedOptScheduler::Run()
{
    CmbSyncAlg* CMB = dynamic_cast<CmbSyncAlg*>(m_syncAlg);
    assert(CMB);

    TheMessenger.barrier();

    m_num_fini = 0; //number of nodes that have exited main loop

    bool processed = false; //a clock edge was just processed; no null msg sent yet
    struct timeval t_null;
    t_null.tv_sec = 0;

    stats_blocking_time = 0;

    while(!m_halted) {
        // Next we  need to find the clock object with the next earliest tick
	GET_NEXT_TICK_TIME; 


        if(CMB->isSafeToProcess(nextClockTime))
        {
#ifdef GET_BLOCKING_TIME
if(processed == false) { //first safe after null was sent
    struct timeval t_safe;
    gettimeofday(&t_safe, NULL);
    if(t_null.tv_sec != 0) { //the very first safe doen't count because no null-msg has been sent
	//blocking time = from sending null-msg to the next safe processing
        stats_blocking_time += (t_safe.tv_sec - t_null.tv_sec)*1000000 + (t_safe.tv_usec - t_null.tv_usec);
    }
    //cout << "safe " << t_safe.tv_sec*1000000 + t_safe.tv_usec << endl;
}
processed = true;
#endif
            assert(nextClockTime>=m_simTime);
            m_simTime = nextClockTime;
            nextClock->ProcessThisTick(); //note m_halted may be set to true as a result
	    if(m_halted)
		break;
        }
        else {
	    //send null messages
	    if(CMB->send_null_msgs(nextClock)) { //returns true when null-msg is actually sent.
#ifdef GET_BLOCKING_TIME
    processed = false;
    gettimeofday(&t_null, NULL);
    //cout << "null " << t_null.tv_sec*1000000 + t_null.tv_usec << endl;
#endif
	    }
	}

        CMB_handle_incoming_messages(); //check incoming messages

    }//while(!m_halted)

    post_main_loop();

}






#ifdef FORECAST_NULL
//####################################################################
// CMB optimized: using Null message + forecast
// This is for research only at present (2013-02-27).
//####################################################################

//====================================================================
//====================================================================
CMB_TickedForecastScheduler :: CMB_TickedForecastScheduler(Lookahead::LookaheadType_t lookaheadType)
{
    m_syncAlg = new CmbSyncAlg(lookaheadType);
}


//====================================================================
//====================================================================
void CMB_TickedForecastScheduler::Run()
{
    CmbSyncAlg* CMB = dynamic_cast<CmbSyncAlg*>(m_syncAlg);
    assert(CMB);

    TheMessenger.barrier();

    m_num_fini = 0; //number of nodes that have exited main loop

    bool done_predict = false;

    while(!m_halted) {
        // Next we  need to find the clock object with the next earliest tick
	GET_NEXT_TICK_TIME; 


        if(CMB->isSafeToProcess(nextClockTime))
        {
//cout << "########################################### process tick " << nextClock->nextTick << " rising= " << nextClock->nextRising << endl;
            assert(nextClockTime>=m_simTime);
            m_simTime = nextClockTime;
            nextClock->ProcessThisTick(); //note m_halted may be set to true as a result
	    if(m_halted)
		break;
	    done_predict = false;
        }
        else {
	    //send null messages
	    if(nextClock->nextRising) {
	        if(!done_predict) {
		    nextClock->do_output_prediction();
		    done_predict = true;
		}
	    }
	    CMB->send_null_msgs_with_forecast(nextClock);
	}

        CMB_handle_incoming_messages(); //check incoming messages

    }//while(!m_halted)

    post_main_loop();

}
#endif //#ifdef FORECAST_NULL



#endif //#ifndef NO_MPI













Scheduler_stat_engine::Scheduler_stat_engine ()
{
}

Scheduler_stat_engine::~Scheduler_stat_engine ()
{
}

void Scheduler_stat_engine::global_stat_merge(Stat_engine * e)
{
    Scheduler_stat_engine * global_engine = (Scheduler_stat_engine*)e;
}

void Scheduler_stat_engine::print_stats (ostream & out)
{
    #ifndef NO_MPI
    TheMessenger.print_stats(out);
    #endif
}

void Scheduler_stat_engine::clear_stats()
{
}

void Scheduler_stat_engine::start_warmup () 
{
}

void Scheduler_stat_engine::end_warmup () 
{
}

void Scheduler_stat_engine::save_samples () 
{
}

} //kernel
} //manifold


