#ifndef NO_MPI
#include "quantum_scheduler.h"
#include "component.h"
#include "clock.h"

#include <cstring>
#include <stdlib.h>
#include <iostream>


using namespace std;

namespace manifold {
namespace kernel {


//same macro as in scheduler.cc; might need to be in a separate header.

#define GET_NEXT_TICK_TIME \
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
    \
    assert(nextClock);





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
//    sends each process a EXIT message.
// 3. Process i != 0, when receiving a EXIT, will exit the main loop, if it hasn't
//    exited already. If it already exited, the EXIT message is ignored.
// 4. When process 0 receives a FINI from all other processes, it sends an END message
//    to all processes, and then terminates the program.
// 5. Process i != 0 receives the END message and terminates the program.




//####################################################################
// Quantum schedulers
//####################################################################

//====================================================================
//====================================================================
Quantum_Scheduler :: Quantum_Scheduler()
{
    m_quantum_inited = false;
    m_syncAlg = new QtmSyncAlg();

    stats_num_exited = 0;
    stats_num_exit = 0;
    stats_num_end = 0;
    stats_num_timestamp_violation = 0;
    stats_max_num_pending_msgs = 0;
    stats_total_pending_msgs = 0;
    stats_num_barrier = 0;
}

//====================================================================
//====================================================================
void Quantum_Scheduler :: quantum_handle_incoming_messages()
{
    int received = 0;
    Message_s msg_tmp;
    do {
	Message_s& msg = TheMessenger.irecv_message(&received);

	if(received != 0) {
	    Component* comp = 0;
	    if(msg.type != Message_s::M_PROTO1) {
		comp = Component :: GetComponent<Component>(msg.compIndex);
		Clock* linkClock = comp->getInLinkClock(msg.inputIndex); //clock for the input port
		Ticks_t nowTicks = linkClock->NowTicks();

		/*
		 * Note:
		 * When full tick is used, if event is for the current tick, then clock must be at the rising edge;
		 * otherwise, the event is in the past.
		 * The second part of the if condition (msg.recvTick == nowTicks && linkClock->nextRising == false) is only
		 * valid for full-tick system. For a half-tick system, the only possible timestamp violations are when msg.recvTick < nowTicks.
		 * For a half tick system, new functionalities need to be implemented in Link to tell whether full tick or half tick is used.
		 */
		if (msg.recvTick < nowTicks || (msg.recvTick == nowTicks && linkClock->nextRising == false) ) {
		    Ticks_t recvTick = msg.recvTick;
		    #ifdef STATS
		    ++stats_num_timestamp_violation;
		    #endif
		    if(linkClock->nextRising == true)
		        msg.recvTick = nowTicks;
		    else
			msg.recvTick = nowTicks + 1;

		    //cout << "OOO event: cid= " << msg.compIndex << " original tick= " << recvTick << " updated to " << msg.recvTick << endl;
		}
	    }
	    switch(msg.type) {
		case Message_s :: M_UINT32:
		    if(m_barrier) {
		        cerr << "not implemented for uint32 !!!!\n";
			exit(1);
		    }
		    else {
			comp->Recv_remote(msg.inputIndex, msg.sendTick, msg.recvTick,
		                      msg.sendTime, msg.recvTime, msg.uint32_data);
                    }
		    break;
		case Message_s :: M_UINT64:
		    if(m_barrier) {
		        cerr << "not implemented for uint64 !!!!\n";
			exit(1);
		    }
		    else {
			comp->Recv_remote(msg.inputIndex, msg.sendTick, msg.recvTick,
		                      msg.sendTime, msg.recvTime, msg.uint64_data);
                    }
		    break;
		case Message_s :: M_SERIAL:
		    if(m_barrier) {
			// deep copy the message
			msg_tmp = msg;
			msg_tmp.data = new unsigned char[msg_tmp.data_len];
			memcpy(msg_tmp.data, msg.data, msg_tmp.data_len);
			m_pending_msg_list.push_back(msg_tmp);
		    }
		    else {
			comp->Recv_remote(msg.inputIndex, msg.sendTick, msg.recvTick,
				   msg.sendTime, msg.recvTime, msg.data, msg.data_len);
		    }
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
void Quantum_Scheduler :: handle_proto1(const Message_s& msg)
{
    int type = (int)(msg.uint32_data);

    switch(type) {
        case MSG_BARRIER_ENTER:
	    assert(TheMessenger.get_node_id() == 0); //a node, other than 0, has entered barrier
	    m_barrier_count++;
	    if (m_barrier_count == TheMessenger.get_node_size()) { //everyone has entered barrier
	    	TheMessenger.broadcast_proto1(MSG_BARRIER_PROCEED, 0);
	    	m_barrier = false;
	    	m_barrier_count = 0;
	    }
	    break;
        case MSG_BARRIER_PROCEED:
	    assert(TheMessenger.get_node_id() != 0);
	    m_barrier = false;
	    break;
        case MSG_NOTIFY_EXIT:
        case MSG_EXIT:
        case MSG_END:
	cout << "received termination msg= " << type << " msg= " << &msg << "\n";
	cout.flush();
	    handle_proto1_for_termination(msg);
	    break;
	default:
	    std::cerr << "invalid protocol message type" << std::endl;
	    exit(1);
    }
}


//====================================================================
//! handle termination-related protocol messages
//====================================================================
void Quantum_Scheduler :: handle_proto1_for_termination(const Message_s& msg)
{
    int type = (int)(msg.uint32_data);

    switch(type) {
        case MSG_NOTIFY_EXIT:
	    assert(TheMessenger.get_node_id() == 0);
	    stats_num_exited++;

	    m_num_exited++;
	    if(m_num_exited == 1) { //The first NOTIFY_EXIT
	    	TheMessenger.broadcast_proto1(MSG_EXIT, 0); //tell others to exit main loop
cout << "node 0 broadcast EXIT\n";
	cout.flush();
	    	m_halted = true;
	    }
	    else if(m_num_exited == TheMessenger.get_node_size()) {
	        //all nodes have exited the main loop
	    	TheMessenger.broadcast_proto1(MSG_END, 0);
	    	m_end = true;
	    }
	    break;
        case MSG_EXIT:
	cout << "got EXIT\n";
	cout.flush();
	    assert(TheMessenger.get_node_id() != 0);
	    stats_num_exit++;
	    m_halted = true;
	    break;
        case MSG_END:
	    assert(TheMessenger.get_node_id() != 0);
	    stats_num_end++;
	    m_end = true;
	    break;
	default:
	    std::cout << "node " << TheMessenger.get_node_id() << " msg= " << &msg << " invalid termination message type= " << type << std::endl;
	    exit(1);
    }
}


//====================================================================
//====================================================================
void Quantum_Scheduler :: processPendingMsg()
{
    //stats
    if(m_pending_msg_list.size() > stats_max_num_pending_msgs)
	stats_max_num_pending_msgs = m_pending_msg_list.size();

    stats_total_pending_msgs += m_pending_msg_list.size();
    stats_num_barrier++;

    for(std::list<Message_s>::iterator it = m_pending_msg_list.begin(); it != m_pending_msg_list.end(); ++it) {
	Message_s msg = *it;
	Component* comp = Component::GetComponent<Component>(msg.compIndex);
        comp->Recv_remote(msg.inputIndex, msg.sendTick, msg.recvTick,
			     msg.sendTime, msg.recvTime, msg.data, msg.data_len);
	delete[] msg.data;
    }
    m_pending_msg_list.clear();
}


//====================================================================
//====================================================================
void Quantum_Scheduler :: enterBarrier()
{
    assert(!m_barrier);

    if (TheMessenger.get_node_id() != 0) {
//cout << ">>>>> node " << TheMessenger.get_node_id() << " enter barrier at " << Clock::Master().NowTicks() << endl;
        TheMessenger.send_proto1_msg(0, MSG_BARRIER_ENTER); //tell node 0 we have entered barrier
        m_barrier = true;
    }
    else {// node 0
//cout << ">>>>> node 0 " << " enter barrier at " << Clock::Master().NowTicks() << endl;
        m_barrier_count++; //count self
        m_barrier = true;
        if (m_barrier_count == TheMessenger.get_node_size()) { //everyone has entered barrier
            TheMessenger.broadcast_proto1(MSG_BARRIER_PROCEED, 0); //everyone can get out of barrier now
	    m_barrier = false;
	    m_barrier_count = 0;
	    //proceed pending list at node 0
	    //processPendingMsg();
        }
    }

    while (m_barrier) { //while in barrier, only process incoming events
        quantum_handle_incoming_messages();
	if(m_halted) { //received MSG_EXIT
	cout << "got termination while in barrier\n";
cout.flush();
	    m_barrier = false; //get out of barrier
	}
    }

    //cout << "..... node " << TheMessenger.get_node_id() << " leave barrier at " << Clock::Master().NowTicks() << endl;

}



//====================================================================
// This is called to receive msgs so the sender doesn't block.
// We don't process the messages unless the messages are signals.
//====================================================================
void Quantum_Scheduler :: quantum_recv_incoming_msg_after_mainloop()
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
		    {
			int type = (int)(msg.uint32_data);
			switch(type) {
			    case MSG_BARRIER_ENTER:
			        assert(TheMessenger.get_node_id() == 0);
				//ignore this message because node 0 already exited main loop
				break;
			    case MSG_BARRIER_PROCEED:
			        //this is not possible
				assert(0);
			        break;
			    default:
				handle_proto1_for_termination(msg);
			        break;
			}
		    }
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
void Quantum_Scheduler :: post_main_loop()
{
    m_end = false;

    if(TheMessenger.get_node_id() == 0) {
        m_num_exited++; //count self
        if(m_num_exited == 1) { //The first to exit main loop
cout << "node 0 broadcast EXIT\n";
cout.flush();
	    TheMessenger.broadcast_proto1(MSG_EXIT, 0);
        }
	else if(m_num_exited == TheMessenger.get_node_size()) {
	    //all nodes have exited the main loop
cout << "node 0 broadcast END\n";
cout.flush();
	    TheMessenger.broadcast_proto1(MSG_END, 0);
	    m_end = true;
	}
    }
    else {
cout << "sending notify exit to 0\n";
cout.flush();
    	TheMessenger.send_proto1_msg(0, MSG_NOTIFY_EXIT); //tell node 0 we have exited main loop
    }

    while(!m_end) {
        quantum_recv_incoming_msg_after_mainloop();
    }

    TheMessenger.barrier();
}


//====================================================================
//====================================================================
void Quantum_Scheduler :: print_stats(std::ostream& out)
{
    Scheduler :: print_stats(out);
    out << "NOTIFY_EXIT: " << stats_num_exited << endl;
    out << "EXIT: " << stats_num_exit << endl;
    out << "END: " << stats_num_end << endl;
    out << "Timestamp violation: " << stats_num_timestamp_violation << endl;
    out << "Max num of pending msgs: " << stats_max_num_pending_msgs << endl;
    out << "Avg num of pending msgs: " << (double)stats_total_pending_msgs / stats_num_barrier << endl;
}


//====================================================================
//====================================================================
void Quantum_Scheduler::Run()
{
    if(!m_quantum_inited) {
        cerr << "Before calling Run(), you must call Quantum_Scheduler::init_quantum() !\n";
	exit(1);
    }

    TheMessenger.barrier();

    m_num_exited = 0; // number of nodes that have exited main loop
    m_barrier_count = 0;
    m_barrier = false;

    Ticks_t next_barrier = m_init_quantum;

    while(!m_halted) {
        // Next we  need to find the clock object with the next earliest tick
        GET_NEXT_TICK_TIME;

        if(Clock::Master().NowTicks() < next_barrier) { //have we reached quantum (next barrier)?
            assert(nextClockTime >= m_simTime);
            m_simTime = nextClockTime;
            nextClock->ProcessThisTick(); // note m_halted may be set to true as a result
            if(m_halted)
	        break;
        }
        else {
	    //enter barrier
	    enterBarrier();

	    //out of barrier
            if(m_halted)
	        break;

	    TheMessenger.barrier();

	    processPendingMsg();

	    next_barrier += m_init_quantum;
	    TheMessenger.barrier();
        }

        quantum_handle_incoming_messages();
    }//while(!m_halted)

cout << " exit main loop\n";
cout.flush();
    post_main_loop();
}














} //kernel
} //manifold


#endif //#ifndef NO_MPI
