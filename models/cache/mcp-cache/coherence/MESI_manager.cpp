#include "MESI_manager.h"
#include <cstdio>
#include <cassert>
#include <iostream>




namespace manifold {
namespace mcp_cache_namespace {


/** Constructor */
MESI_manager::MESI_manager(int id) : ManagerInterface(id)
{
    state = MESI_MNG_I;
    owner = -1;
    unblocked_s_recv = false;
    clean_wb_recv = false;
}

/** Destructor */
MESI_manager::~MESI_manager()
{

}


/** Demand data supply from lower tier's paired manager or L1 */
void MESI_manager::Supply()
{

}

/** Demand lower realm to forfeit both read and write permissions, invaliding all local copies of data */
void MESI_manager::Invalidate()
{

}

/** Demand data from lower realm's paired manager.  The lower realm must forfeit write permissions but can retain read permissions and data */
void MESI_manager::SupplyDowngrade()
{

}

/** Demand data from lower realm's paired manager.  The lower realm must forfeit write permissions and read permissions, invalidating all local copies of data */
void MESI_manager::SupplyInvalidate()
{
}

void MESI_manager :: Evict()
{
    process(GET_EVICT, 0);
}


/** Called from the client to complete previous GetReadD request.  Supplies data packed and signifies paired client now has read permissions */
void MESI_manager::GrantReadD()
{

}

/** Called from the client to complete previous GetWrite/GetWriteD request. Supplies data packet and signifies paired client now has write permissions */
void MESI_manager::GrantWriteD()
{

}

/** Called from the client to complete previous GetWrite request. Supplies data packet and signifies paired client now has write permissions */
void MESI_manager::GrantWrite()
{

}

/** Called from the client to complete previous GetEvict request. Supplies data packet and signifies paired client is now invalid */
void MESI_manager::GrantEvict()
{

}

bool MESI_manager :: req_pending()
{
    switch(state) {
        case MESI_MNG_E:
        case MESI_MNG_S:
        case MESI_MNG_I:
	    return false;
	default:
	    return true;
    }
}






void MESI_manager :: process(int msg_type, int src_id)
{
    MESI_messages_t msg = (MESI_messages_t)msg_type;

    switch(state)
    {
        case MESI_MNG_I:
            do_I(msg, src_id);
            break;
        case MESI_MNG_E:
            do_E(msg, src_id);
            break;
        case MESI_MNG_S:
            do_S(msg, src_id);
            break;
        case MESI_MNG_IE:
            do_IE(msg, src_id);
            break;
        case MESI_MNG_EE:
            do_EE(msg, src_id);
            break;
        case MESI_MNG_EI_PUT:
            do_EI_PUT(msg, src_id);
            break;
        case MESI_MNG_EI_EVICT:
            do_EI_EVICT(msg);
            break;
        case MESI_MNG_ES:
            do_ES(msg, src_id);
            break;
        case MESI_MNG_SS:
            do_SS(msg, src_id);
            break;
        case MESI_MNG_SIE:
            do_SIE(msg, src_id);
            break;
        case MESI_MNG_SI_EVICT:
            do_SI_EVICT(msg, src_id);
            break;
        default:
            invalid_msg((MESI_messages_t)msg_type);
	    break;
    }
}




inline void MESI_manager::do_I(MESI_messages_t msg_type, int src_id)
{
#ifdef DBG_MESI_MANAGER
std::cout << "Manager in state I, msg= " << msg_type << std::endl;
#endif
    switch(msg_type)
    {
        case MESI_CM_I_to_E:
        case MESI_CM_I_to_S:
            sendmsg(false, MESI_MC_GRANT_E_DATA, src_id);
            // who sent it? record sender.
            owner = src_id;
            transition_to_ie();
            break;
        case MESI_CM_E_to_I:
        case MESI_CM_M_to_I:
	    //This happens when manager sends DEMAND_I to client, and client
	    //before receiving it already sent E_to_I or M_to_I, which would
	    //stall. Client in EI or MI state processes DEMAND_I and enters I.
	    //The E_to_I or M_to_I should simply be ignored.
	    ignore();
	    break;
        default:
            invalid_msg((MESI_messages_t)msg_type);
	    break;
    }
}




inline void MESI_manager::do_E(MESI_messages_t msg_type, int srcID)
{
#ifdef DBG_MESI_MANAGER
std::cout << "Manager in state E, msg= " << msg_type << std::endl;
#endif
    switch(msg_type)
    {
        case MESI_CM_I_to_E:
            // must be an owner here...
            assert(owner != -1);
	    assert(owner != srcID);
            // has to be sent to current owner though.
            // we have a new owner record it.
            sendmsg(true, MESI_MC_FWD_E, owner, srcID);
            owner = srcID;
            transition_to_ee();
            break;
        case MESI_CM_I_to_S:
            // No more owner we have sharers now.
            sendmsg(true, MESI_MC_FWD_S, owner, srcID); //send msg to owner
            sharersList.set(owner);
            sharersList.set(srcID);
            owner =-1;
            transition_to_es();
            break;
        case GET_EVICT:
            sendmsg(true, MESI_MC_DEMAND_I, owner);
            // has to be sent to owner.
            // no owner anymore
            owner = -1;
            transition_to_ei_evict();
            break;
        case MESI_CM_E_to_I: // no more owner
        case MESI_CM_M_to_I: // no more owner
	    if(srcID == owner) {
		if(msg_type == MESI_CM_M_to_I)
		    client_writeback();
		sendmsg(false, MESI_MC_GRANT_I, owner);
		owner = -1;
		transition_to_ei_put();
	    }
	    else {
	        //This happens in a race condition; right before receiving his
		//E_to_I or M_to_I, we got a write request, which is now complete
		//and owner has changed. Since owner also has detected the race
		//condition and has changed to I, we just call ignore().
	        ignore();
	    }
            break;
        default:
            invalid_msg((MESI_messages_t)msg_type);
            break;
    }
}


//In S state we can receive CM_E_to_I. Here is the scenario:
//
//    C0              M               C1
//     |------1------>|                |
//     |              |---2---  ---3---|  
//     |              |       \/       |
//     |              |       /\       |
//     |              |<------  ------>|
//
// 1. Client C0 sends I_to_S to LOAD a missed line.
// 2. The line is in E state, so manager M sends FWD_S to owner C1.
// 3. Before 2 reaches C1, the line is being evicted in C1, so it
//    sends CM_E_to_I to M. 
// 4. When E_to_I from C1 reaches M, it PREV_PEND_STALLs since an
//    mshr entry for the same line already exists.
// 5. C1 receives FWD_S in the EI state. It processes it as if it were
//    in the E state: it sends CC_S_DATA to C0, and CM_CLEAN to M.
//    Then it silently enters I state.
// 6. M gets CM_CLEAN from C1, and UNBLOCK_S from C0 and enters S state.
// 7. Now M, in S state, processes E_to_I in step 3.
//    Since C1 is already in I state, there's nothing to be done, so we
//    call ignore() in case the owning object needs cleanup.
inline void MESI_manager::do_S(MESI_messages_t msg_type, int srcID)
{
#ifdef DBG_MESI_MANAGER
std::cout << "Manager in state S, msg= " << msg_type << std::endl;
#endif
    switch(msg_type)
    {
        case MESI_CM_I_to_S:
            sendmsg(false, MESI_MC_GRANT_S_DATA, srcID);
            sharersList.set(srcID);
            transition_to_ss();
            break;
        case MESI_CM_I_to_E:
	    sharersList.reset(srcID); //don't send DEMAND_I to requestor if it's a sharer.
            sendmsgtosharers(MESI_MC_DEMAND_I);
            num_invalidations = 0;
            num_invalidations_req = sharersList.count();
            sharersList.clear();
            owner = srcID;
            transition_to_sie();
            break;
        case GET_EVICT:
            sendmsgtosharers(MESI_MC_DEMAND_I);
            num_invalidations = 0;
            num_invalidations_req = sharersList.count();
            sharersList.clear();
            transition_to_si_evict();
            break;
	    /* The following shouldn't happen! The race between E_to_I and FWD_S has been re-implemented.
        case MESI_CM_E_to_I: //E_to_I was sent before the client turns into S. Since client already
	                     //transits to I, we do nothing.
        case MESI_CM_M_to_I: //same as E_to_I above.
	    ignore();
	    {
		assert(sharersList.count() == 2);
		sharersList.reset(srcID);
		std::vector<int> shers;
		sharersList.ones(shers);
		owner = shers[0];
		sharersList.reset(owner);
	    }
	    transition_to_e();
            break;
	    */
        case MESI_CM_E_to_I: //E_to_I was delayed; manager already in S state.
	    std::cout << "WARNING: manager receiving CM_E_to_I in S state.\n";
	    ignore();
            break;
        case MESI_CM_M_to_I: //M_to_I was delayed; manager already in S state.
	    std::cout << "WARNING: manager receiving CM_M_to_I in S state.\n";
	    ignore();
            break;
        default:
            invalid_msg((MESI_messages_t)msg_type);
	    break;
    }
}


inline void MESI_manager::do_IE(MESI_messages_t msg_type, int srcID)
{
#ifdef DBG_MESI_MANAGER
std::cout << "Manager in state IE, msg= " << msg_type << std::endl;
#endif
    switch(msg_type)
    {
        case MESI_CM_UNBLOCK_E:
            transition_to_e();
            break;
        default:
            invalid_msg(msg_type);
            break;
    }
}




inline void MESI_manager::do_EE(MESI_messages_t msg_type, int srcID)
{
#ifdef DBG_MESI_MANAGER
std::cout << "Manager in state EE, msg= " << msg_type << std::endl;
#endif
    switch(msg_type)
    {
        case MESI_CM_UNBLOCK_E:
            transition_to_e();
            break;
        default:
            invalid_msg(msg_type);
            break;
    }
}


inline void MESI_manager::do_EI_PUT(MESI_messages_t msg_type, int srcID)
{
#ifdef DBG_MESI_MANAGER
std::cout << "Manager in state EI_PUT, msg= " << msg_type << std::endl;
#endif
    switch(msg_type)
    {
        case MESI_CM_UNBLOCK_I:
            transition_to_i();
            break;
        default:
            invalid_msg(msg_type);
            break;
    }
}



inline void MESI_manager::do_EI_EVICT(MESI_messages_t msg_type)
{
#ifdef DBG_MESI_MANAGER
std::cout << "Manager in state EI_EVICT, msg= " << msg_type << std::endl;
#endif
    switch(msg_type)
    {
        case MESI_CM_UNBLOCK_I:
            transition_to_i();
            break;
        case MESI_CM_UNBLOCK_I_DIRTY:
            // Writeback
	    client_writeback();
            transition_to_i();
            break;
        default:
            invalid_msg(msg_type);
            break;
    }
}


inline void MESI_manager::do_ES(MESI_messages_t msg_type, int srcID)
{
#ifdef DBG_MESI_MANAGER
std::cout << "Manager in state ES, msg= " << msg_type << std::endl;
#endif
    switch(msg_type)
    {
        case MESI_CM_UNBLOCK_E: //a race condition occurred on client side. While we sent FWD_S
	                        //to client, it was trying to evict the line. Since manager request
				//takes priority, it sent CC_E_DATA to requestor, which in turn sent
				//this UNBLOCK_E to us.
	    sharersList.clear();
	    owner = srcID;
	    transition_to_e();
            break;
        case MESI_CM_UNBLOCK_S:
            assert(!unblocked_s_recv);
            unblocked_s_recv = true;
            break;
        case MESI_CM_CLEAN:
        case MESI_CM_WRITEBACK:
            assert(!clean_wb_recv);
            clean_wb_recv = true;
	    if(msg_type == MESI_CM_WRITEBACK)
	        client_writeback();
            break;
        default:
            invalid_msg(msg_type);
            break;
    }
    if (unblocked_s_recv && clean_wb_recv)
    {
        unblocked_s_recv = false;
        clean_wb_recv = false;
        transition_to_s();
    }
}



inline void MESI_manager::do_SS(MESI_messages_t msg_type, int srcID)
{
#ifdef DBG_MESI_MANAGER
std::cout << "Manager in state SS, msg= " << msg_type << std::endl;
#endif
    switch(msg_type)
    {
        case MESI_CM_UNBLOCK_S:
            transition_to_s();
            break;
        default:
            invalid_msg(msg_type);
            break;
    }
}

inline void MESI_manager::do_SI_EVICT(MESI_messages_t msg_type, int srcID)
{
#ifdef DBG_MESI_MANAGER
std::cout << "Manager in state SI_EVICT, msg= " << msg_type << std::endl;
#endif
    switch(msg_type)
    {
        case MESI_CM_UNBLOCK_I:
            num_invalidations++;
            if (num_invalidations == num_invalidations_req)
                transition_to_i();
            break;
        default:
            invalid_msg(msg_type);
            break;
    }
}

inline void MESI_manager::do_SIE(MESI_messages_t msg_type, int srcID)
{
#ifdef DBG_MESI_MANAGER
std::cout << "Manager in state SIE, msg= " << msg_type << std::endl;
#endif
    switch(msg_type)
    {
        case MESI_CM_UNBLOCK_I:
            num_invalidations++;
            if (num_invalidations == num_invalidations_req)
            {
                transition_to_ie();
                sendmsg(false, MESI_MC_GRANT_E_DATA, owner);
            }
            break;
        default:
            invalid_msg(msg_type);
            break;
    }
}


void MESI_manager::sendmsgtosharers(MESI_messages_t msg)
{
    std::vector<int> sharerIDs;
    sharersList.ones(sharerIDs);

    for (unsigned int i = 0; i < sharerIDs.size(); i++)
    {
        sendmsg(true, msg, sharerIDs[i]);
    }

}

void MESI_manager::transition_to_i()
{
    state = MESI_MNG_I;
    invalidate();
}

void MESI_manager::transition_to_e()        {state = MESI_MNG_E;}
void MESI_manager::transition_to_s()        {state = MESI_MNG_S;}
void MESI_manager::transition_to_ie()       {state = MESI_MNG_IE;}
void MESI_manager::transition_to_ee()       {state = MESI_MNG_EE;}
void MESI_manager::transition_to_ei_evict() {state = MESI_MNG_EI_EVICT;}
void MESI_manager::transition_to_ei_put()   {state = MESI_MNG_EI_PUT;}
void MESI_manager::transition_to_es()       {state = MESI_MNG_ES;}
void MESI_manager::transition_to_ss()       {state = MESI_MNG_SS;}
void MESI_manager::transition_to_sie()      {state = MESI_MNG_SIE;}
void MESI_manager::transition_to_si_evict() {state = MESI_MNG_SI_EVICT;}

void MESI_manager::invalid_msg(MESI_messages_t msg) {printf("Invalid message %d for state %d\n", msg, state); assert(0);}


} //namespace mcp_cache_namespace
} //namespace manifold
