#include "MESI_client.h"
#include <assert.h>
#include <cstdio>
#include <iostream>

//! There are 8 cases where the client has conflicting requests:

//!           | client state | MC_FWD_E | MC_FWD_S | MC_DEMAND_I
//! ----------|--------------|----------|----------|-------------
//! CM_I_to_E |      IE      |    NA    |    NA    | conflict*
//! ----------|--------------|----------|----------|-------------
//! CM_I_to_S |      IE      |    NA    |    NA    |    NA
//! ----------|--------------|----------|----------|-------------
//! CM_E_to_I |      EI      | conflict | conflict | conflict
//! ----------|--------------|----------|----------|-------------
//! CM_M_to_I |      MI      | conflict | conflict | conflict
//!
//! Note 1: Another is between CM_I_to_E and MC_DEMAND_I, when the CM_I_to_E really means
//! CM_S_to_E, sent when a client in S is being written to.
//!
//! Note 2: Client in IE could also get a DEMAND_I. If a line in S is evicted, manager is
//! not notified. Later if the SAME line is accessed, an I_to_E is sent to manager. If before
//! processing the I_to_E, manager starts to evict the line, since manager thinks the client
//! is a sharer, it sends DEMAND_I, so client gets DEMAND_I in IE state.
//!
//!
//! For example, the conflict between CM_E_to_I and MC_FWD_E happens as follows:
//! Manager gets a write request from a client. Since it's in E, it sends FWD_E
//! to owner. Before the owner receives the FWD_E, it starts evicting the line,
//! so it sends CM_E_to_I to manager. Here is how the manager and client deal
//! with the situation:
//! Client: manager's request takes precedence, so it processes FWD_E in EI state
//!         as if it were in E state. Then enters I state.
//! Manager: manager is unaware of the conflict that's occurring. After the store
//!          request is complete, it enters E state with owner set to the new
//!          requestor. Now it processes CM_E_to_I in E state, but it will notice
//!          the sender is not the owner, so it must be the old owner. It simply
//!          ignores the request since client already enters I state.


using namespace std;


namespace manifold {
namespace mcp_cache_namespace {

/** Constructor */
MESI_client::MESI_client(int id) : ClientInterface(id)
{
    state = MESI_C_I;
}

/** Destructor */
MESI_client::~MESI_client()
{

}

/** Client begins data and read permission acquistion within its native coherence realm L1/Lower Manager expects GrantReadD upon completion */
void MESI_client::GetReadD()
{
    //process(new cache_req(my_entry->tag, -1, GET_S, -1));
    process(GET_S, -1);
}

/** Client begins data and write permission acquistion within its native coherence realm L1/Lower Manager expects GrantWriteD upon completion */
void MESI_client::GetWriteD()
{
    //process(new cache_req(my_entry->tag, -1, GET_E, -1));
    process(GET_E, -1);
}

/** Client begins write permission acquistion within its native coherence realm L1/Lower Manager expects GrantWrite or GrantWriteD upon completion */
void MESI_client::GetWrite()
{
    //process(new cache_req(my_entry->tag, -1, GET_E, -1));
    process(GET_E, -1);
}

/** Client begins eviction sequence within its native coherence realm L1/Lower Manager expects GrantWrite or GrantWriteD upon completion */
void MESI_client::GetEvict()
{
    //process(new cache_req(my_entry->tag, -1, GET_EVICT, -1));
    process(GET_EVICT, -1);
}

/** Returns true if data exists or can be supplied to the manager */
bool MESI_client::HaveData()
{
    bool ret = false;
	switch(state)
	{
		case MESI_C_I:
            break;
		case MESI_C_E:
		case MESI_C_S:
		case MESI_C_M:
		    ret = true;
            break;
        default:
        // transient states?
            invalid_state();
            break;
	}
	return ret;
}

/** Returns true if you have read permissions */
bool MESI_client::HaveReadP()
{
    bool ret = false;
	switch(state)
	{
		case MESI_C_I:
            break;
		case MESI_C_E:
		case MESI_C_S:
		case MESI_C_M:
		    ret = true;
            break;
        default:
            invalid_state();
            break;
	}
	return ret;
}

/** Returns true if you have write permissions */
bool MESI_client::HaveWriteP()
{
    bool ret = false;
	switch(state)
	{
		case MESI_C_I:
		case MESI_C_S:
		case MESI_C_E:
		    break;
		case MESI_C_M:
		    ret = true;
            break;
        default:
            //invalid_state();
            break;
	}
	return ret;
}

/** Returns true if you can be safely evicted */
bool MESI_client::HaveEvictP()
{
    bool ret = false;
	switch(state)
	{
		case MESI_C_I:
		    ret = true;
		    break;
		case MESI_C_S:
		case MESI_C_E:
		case MESI_C_M:
            break;
        default:
            //invalid_state();
            break;
	}
	return ret;
}


/** Called by the Manager to complete previous Supply demand. Supplies data packet */
void MESI_client::SupplyAck()
{
}

/** Called by the Manager to complete previous Invalidate demand.  Signifies realm invalidation has completed */
void MESI_client::InvalidateAck()
{
}

/** Called by the Manager to complete previous SupplyDowngrade demand.  Supplies data packet and signifies realm downgrade has completed */
void MESI_client::SupplyDowngradeAck()
{
}

/** Called by the Manager to complete previous SupplyInvalidate demand.  Supplies data packet and signifies realm invalidation has completed */
void MESI_client::SupplyInvalidateAck()
{
}

bool MESI_client :: req_pending()
{
    switch(state) {
        case MESI_C_M:
        case MESI_C_E:
        case MESI_C_S:
        case MESI_C_I:
	    return false;
	default:
	    return true;
    }
}




//! @param \c fwdID  The ID of the client to forward msg to. If no forward, can be any value.
void MESI_client::process(int msg, int fwdID)
{
    MESI_messages_t m = (MESI_messages_t) msg;

    switch(state)
    {
	case MESI_C_I:
            do_I(m);
            break;
	case MESI_C_S:
            do_S(m);
            break;
	case MESI_C_E:
            do_E(m, fwdID);
            break;
        case MESI_C_M:
            do_M(m, fwdID);
            break;
        case MESI_C_IE:
            do_IE(m);
            break;
        case MESI_C_EI:
            do_EI(m, fwdID);
            break;
        case MESI_C_MI:
            do_MI(m, fwdID);
            break;
        case MESI_C_SE:
            do_SE(m);
            break;
        //case MESI_C_SIE:
            //do_SIE(m);
            //break;
        default:
            invalid_msg((MESI_messages_t)msg);
            break;
    }
}



inline void MESI_client::do_I(MESI_messages_t msg)
{
#ifdef DBG_MESI_CLIENT
std::cout << "Client in state I, msg= " << msg << std::endl;
#endif
    switch (msg)
    {
        case GET_E:
            sendmsg(true, MESI_CM_I_to_E);
            transition_to_ie();
            break;
        case GET_S:
            sendmsg(true, MESI_CM_I_to_S);
            transition_to_ie();
            break;
        default:
            invalid_msg(msg);
	    break;
    }
}



inline void MESI_client::do_S(MESI_messages_t msg)
{
#ifdef DBG_MESI_CLIENT
std::cout << "Client in state S, msg= " << msg << std::endl;
#endif
    switch (msg)
    {
        case GET_E:
            sendmsg(true, MESI_CM_I_to_E); //should we create a new message MESI_CM_S_to_E and
	                                   //use it instead ??????????????????????????????
            //transition_to_sie();
            transition_to_se(); //go to SE and wait for GRANT_E, as manager won't send DEMAND_I to us.
            break;
        case GET_S:
            break;
        case GET_EVICT:
            transition_to_i();
            break;
        case MESI_MC_DEMAND_I:
            sendmsg(false, MESI_CM_UNBLOCK_I);
            transition_to_i();
            break;
        default:
            invalid_msg(msg);
	    break;
    }
}




inline void MESI_client::do_E(MESI_messages_t msg, int fwdID)
{
#ifdef DBG_MESI_CLIENT
std::cout << "Client in state E, msg= " << msg << std::endl;
#endif
    switch (msg)
    {
        case GET_E:
            transition_to_m();
            break;
        case GET_S:
            break;
        case MESI_MC_FWD_E:
            sendmsg(false, MESI_CC_E_DATA, fwdID); // send to client in IE.
            transition_to_i();
            break;
        case MESI_MC_FWD_S:
            sendmsg(false, MESI_CC_S_DATA, fwdID); // send to client in IE.
            sendmsg(false, MESI_CM_CLEAN);
            transition_to_s();
            break;
        case GET_EVICT:
            sendmsg(true, MESI_CM_E_to_I);
            transition_to_ei();
            break;
        case MESI_MC_DEMAND_I:
            sendmsg(false, MESI_CM_UNBLOCK_I);
            transition_to_i();
            break;
        default:
            invalid_msg(msg);
	    break;
    }
}

inline void MESI_client::do_M(MESI_messages_t msg, int fwdID)
{
#ifdef DBG_MESI_CLIENT
std::cout << "Client in state M, msg= " << msg << std::endl;
#endif
    switch (msg)
    {
        case GET_E:
        case GET_S:
            break;
        case MESI_MC_DEMAND_I:
            sendmsg(false, MESI_CM_UNBLOCK_I_DIRTY);
            transition_to_i();
            break;
        case GET_EVICT:
            sendmsg(true, MESI_CM_M_to_I);
            transition_to_mi();
            break;
        case MESI_MC_FWD_E:
            sendmsg(false, MESI_CC_M_DATA, fwdID);
             transition_to_i();
             break;
        case MESI_MC_FWD_S:
            sendmsg(false, MESI_CC_S_DATA, fwdID);
            sendmsg(false, MESI_CM_WRITEBACK);
            transition_to_s();
            break;
        default:
            invalid_msg(msg);
	    break;
    }
}



inline void MESI_client::do_IE(MESI_messages_t msg)
{
#ifdef DBG_MESI_CLIENT
std::cout << "Client " << id << " in state IE, msg= " << msg << std::endl;
#endif
    switch (msg)
    {
        case MESI_CC_E_DATA: // data forwarded to me
            sendmsg(false, MESI_CM_UNBLOCK_E);
            transition_to_e();
            break;
        case MESI_CC_S_DATA: // data forwarded to me
            sendmsg(false, MESI_CM_UNBLOCK_S);
            transition_to_s();
            break;
        case MESI_MC_GRANT_E_DATA: // data given to me by manager
            sendmsg(false, MESI_CM_UNBLOCK_E);
            transition_to_e();
            break;
        case MESI_MC_GRANT_S_DATA: // sharing
            sendmsg(false, MESI_CM_UNBLOCK_S);
            transition_to_s();
            break;
        case MESI_CC_M_DATA: // data forwarded by M state guy Will never happen?
            sendmsg(false, MESI_CM_UNBLOCK_E);
            transition_to_m();
            break;
	//DEMAND_I could happen like this: The line was in S but was evicted. Manager was not notified. Later,
	//A request for the SAME line caused an I_to_E sent to manager. At the same time, the line was evicted
	//on manager side, so manager sends a DEMAND_I.
        case MESI_MC_DEMAND_I: //when a line X was evicted in the S state, manager still thinks it's a sharer;
	                       //a store for X would cause manager to invalidate X first (i.e., MC_DEMAND_I);
			       //so we need to respond.
            sendmsg(false, MESI_CM_UNBLOCK_I);
	    break;
        default:
            invalid_msg(msg);
	    break;
    }
}


inline void MESI_client::do_SE(MESI_messages_t msg)
{
#ifdef DBG_MESI_CLIENT
std::cout << "Client " << id << " in state SE, msg= " << msg << std::endl;
#endif
    switch (msg)
    {
        case MESI_MC_GRANT_E_DATA: // data given to me by manager
            sendmsg(false, MESI_CM_UNBLOCK_E);
            transition_to_e();
            break;
        case MESI_MC_DEMAND_I: //while client changing from S to E, manager is evicting the same line.
	                       //The client's write request will be processed in due time, so we respond
			       //to allow manager to finish eviciton first.
            sendmsg(false, MESI_CM_UNBLOCK_I);
            transition_to_ie();
	    break;
        case MESI_CC_M_DATA: // This happens during a race condition. While we are going from S to E, another
	                     // client writes to the same line and its request got to the manager before us.
			     // Finally when our I_to_E is being processed, the other client already is the owner,
			     // so it will send us CC_M_DATA.
            sendmsg(false, MESI_CM_UNBLOCK_E);
            transition_to_m();
            break;
        default:
            invalid_msg(msg);
	    break;
    }
}



inline void MESI_client::do_EI(MESI_messages_t msg, int fwdID)
{
#ifdef DBG_MESI_CLIENT
std::cout << "Client in state EI, msg= " << msg << std::endl;
#endif
    switch (msg)
    {
        case MESI_MC_GRANT_I:
            sendmsg(false, MESI_CM_UNBLOCK_I);
            transition_to_i();
            break;
        case MESI_MC_FWD_S: //we have started eviction, but manager just got a LOAD request and wants us to
	                    //transit to S state. We act as if the eviction never happened and we were still in E,
			    //but silenly transit to I.
			    //Manager would ignore the CM_E_to_I we just sent.
	    sendmsg(false, MESI_CC_E_DATA, fwdID); // send to peer in IE. Send CC_E_DATA so it would enter E state.
	    //sendmsg(false, MESI_CM_CLEAN); NO msg to manager since it will get UNBLOCK_E from peer.
	    transition_to_i();
            break;
        case MESI_MC_FWD_E: //we have started eviction, but manager just got a STORE request and wants us to
	                    //transit to I state. We act as if the eviction never happened and we were still in E,
			    //and silently transit to I.
			    //Manager would ignore the CM_E_to_I we just sent.
	    sendmsg(false, MESI_CC_E_DATA, fwdID);
	    transition_to_i();
	    break;
        case MESI_MC_DEMAND_I: //we have started eviction, but manager just got a request and wants to invalidate us.
	                    //We act as if the eviction never happened and we were still in E,
			    //and silently transit to I.
			    //Manager would ignore the CM_E_to_I we just sent.
            sendmsg(false, MESI_CM_UNBLOCK_I);
	    transition_to_i();
	    break;
        default:
            invalid_msg(msg);
            break;
    }
}


inline void MESI_client::do_MI(MESI_messages_t msg, int fwdID)
{
#ifdef DBG_MESI_CLIENT
std::cout << "Client in state MI, msg= " << msg << std::endl;
#endif
    switch (msg)
    {
        case MESI_MC_GRANT_I:
            sendmsg(false, MESI_CM_UNBLOCK_I);
            transition_to_i();
            break;
        case MESI_MC_FWD_S: //we have started eviction, but manager just got a LOAD request and wants us to
	                    //transit to S state. We act as if the eviction never happened and we were still in M,
			    //but silenly transit to I.
			    //Manager would ignore the CM_M_to_I we just sent.
	    sendmsg(false, MESI_CC_M_DATA, fwdID); // send MESI_CC_M_DATA so requestor would enter M state.
	    //sendmsg(false, MESI_CM_WRITEBACK); No msg to manager since it will get UNBLOCK_E from requestor.
	    transition_to_i();
	    break;
        case MESI_MC_FWD_E: //we have started eviction, but manager just got a STORE request and wants us to
	                    //transit to I state. We act as if the eviction never happened and we were still in M,
			    //and silently transit to I.
			    //Manager would ignore the CM_M_to_I we just sent.
	    sendmsg(false, MESI_CC_M_DATA, fwdID);
	    transition_to_i(); //go to I as if we were evicting in S state.
	    break;
        case MESI_MC_DEMAND_I: //we have started eviction, but manager just got a request and wants to invalidate us.
	                    //We act as if the eviction never happened and we were still in M,
			    //and silently transit to I.
			    //Manager would ignore the CM_M_to_I we just sent.
            sendmsg(false, MESI_CM_UNBLOCK_I_DIRTY);
	    transition_to_i();
	    break;
        default:
            invalid_msg(msg);
            break;
    }
}



/*
inline void MESI_client::do_SIE(MESI_messages_t msg)
{
#ifdef DBG_MESI_CLIENT
std::cout << "Client in state SIE, msg= " << msg << std::endl;
#endif
    switch (msg)
    {
        case MESI_MC_DEMAND_I:
            sendmsg(false, MESI_CM_UNBLOCK_I);
            transition_to_ie();
            break;
        default:
            invalid_msg(msg);
            break;
    }
}
*/



void MESI_client::transition_to_i()
{
    state = MESI_C_I;
    invalidate();
}

void MESI_client::transition_to_ie() { state = MESI_C_IE; }
void MESI_client::transition_to_se() { state = MESI_C_SE; }
void MESI_client::transition_to_s() {state = MESI_C_S;}
void MESI_client::transition_to_e() {state = MESI_C_E;}
void MESI_client::transition_to_ei() {state = MESI_C_EI;}
void MESI_client::transition_to_m() {state = MESI_C_M;}
void MESI_client::transition_to_mi() {state = MESI_C_MI;}
//void MESI_client::transition_to_sie() {state = MESI_C_SIE;}
void MESI_client::invalid_msg(MESI_messages_t msg) {printf("Invalid message %d for state %d\n", msg, state); assert(0);}
void MESI_client::invalid_state() {printf("Invalid state %d", state);}


/*
bool MESI_client :: invalidated()
{
    return state == MESI_C_I;
}
*/



} //namespace mcp_cache_namespace
} //namespace manifold
