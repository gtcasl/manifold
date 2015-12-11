// Mirko Stoffers, (and others) Georgia Tech, 2012

#ifndef NO_MPI


#include <vector>
#include <assert.h>

#include "manifold-decl.h"
#include "message.h"
#include "syncalg.h"
#include "manifold-decl.h"
#include "clock.h"

#include <iostream>
#include <sys/time.h>

namespace manifold {
namespace kernel {


//#define GET_NULL_MSG_TIME


SyncAlg :: SyncAlg(Lookahead::LookaheadType_t laType)
{
    m_lookahead = Lookahead::Create(laType);
    m_outputTS = 0;
}

void SyncAlg::UpdateLookahead(Time_t delay, LpId_t src, LpId_t dst)
{
    m_lookahead->UpdateLookahead(delay,src,dst);
}


#ifdef FORECAST_NULL
//! Update the tick when an output is to be sent.
//! @param src This LP.
//! @param dst The destination LP.
//! @param when The tick when an output is to be sent.
//! @param clk The clock in question.
void SyncAlg :: updateOutputTick(LpId_t src, LpId_t dst, Ticks_t when, Clock* clk)
{
//cout << "*************** syncAlg, at tick " << clk->NowTicks() << " predict= " << when << " src= " << src << " dst= " << dst << " m_times= " << m_outputTS->m_times[src][dst] << endl;

    if(clk->NowTicks() < m_outputTS->m_prev_ticks[src][dst])
	when = (when < m_outputTS->m_prev_ticks[src][dst]) ? when : m_outputTS->m_prev_ticks[src][dst];

    Time_t t = when / clk->freq;
    if(m_outputTS->m_times[src][dst] == 0 || m_outputTS->m_times[src][dst] < Manifold::NowTicks(*clk) / clk->freq) {
        m_outputTS->m_times[src][dst] = t;
        m_outputTS->m_ticks[src][dst] = when;
    }
    else {
        if(t < m_outputTS->m_times[src][dst]) {
	    m_outputTS->m_times[src][dst] = t;
	    m_outputTS->m_ticks[src][dst] = when;
	}
    }
//cout << "m_outputTS[" << src << ", " << dst << "]= " << m_outputTS->m_ticks[src][dst] << endl;
}
#endif //#ifdef FORECAST_NULL





//####################################################################
// LBTS
//####################################################################

LbtsSyncAlg::LbtsSyncAlg(Lookahead::LookaheadType_t laType) : SyncAlg(laType)
{
    m_grantedTime = 0;
    m_stats_LBTS_sync = 0;
}


bool LbtsSyncAlg::isSafeToProcess(double requestTime)
{
    static LBTS_Msg* LBTS = new LBTS_Msg[TheMessenger.get_node_size()];

    if(requestTime <= m_grantedTime) {
        return true;
    }
    else {
	LBTS_Msg lbts_msg = {0,0,0,0,0};
	lbts_msg.tx_count = TheMessenger.get_numSent();
	lbts_msg.rx_count = TheMessenger.get_numReceived();
	lbts_msg.smallest_time = requestTime;
	int nodeId = TheMessenger.get_node_id();
	LBTS[nodeId] = lbts_msg;

	TheMessenger.allGather((char*)&(lbts_msg), sizeof(LBTS_Msg), (char*)LBTS);

	#ifdef STATS
	m_stats_LBTS_sync++;
	#endif
	//MPI_Allgather(&(LBTS[nodeId]), sizeof(LBTS_MSG), MPI_BYTE, LBTS, 
	//		sizeof(LBTS_MSG), MPI_BYTE, MPI_COMM_WORLD);
	int rx=0;
	int tx=0;
	double smallest_time = LBTS[0].smallest_time;

	for(int i=0; i<TheMessenger.get_node_size(); i++) {
	    tx+=LBTS[i].tx_count;
	    rx+=LBTS[i].rx_count;

	    if(LBTS[i].smallest_time < smallest_time) {
		smallest_time=LBTS[i].smallest_time;
	    }
	}
	if(rx==tx) {
	    //grantedTime=DistributedSimulator::smallest_time+LinkRTI::minTxTime;
	    m_grantedTime = smallest_time;
	    if(m_grantedTime < 0) //for termination detection.
		Manifold :: get_scheduler()->Stop(); //set halted to true

	    if(requestTime <= m_grantedTime)
		return true;
	    else
		return false;
        }
	else {
	    return false;
	}
    }
}


void LbtsSyncAlg :: terminateInitiated()
{
    //With LBTS, isSafeToProcess() calls a collective function, MPI_Allgather(). This
    //means if the terminating process simply quits, then the other processes will block forever.
    //Therefore, the terminator must call isSafeToProcess() one more time, and we use
    //-1 as a signal that all processes should terminate.

    Time_t grantedTime_save = m_grantedTime; //save it.
    //ensure requestTime > grantedTime, so the collective function is called.
    m_grantedTime = -2;
    this->isSafeToProcess(-1);
    m_grantedTime = grantedTime_save;
}


void LbtsSyncAlg::PrintStats(std::ostream& out)
{
    out << "  LBTS all-gather synchronization: " << m_stats_LBTS_sync << std::endl;
}





//####################################################################
// CMB Null message
//####################################################################

CmbSyncAlg::CmbSyncAlg(Lookahead::LookaheadType_t laType) : SyncAlg(laType)
{
    m_neighborVersion=0;
    m_initialized = false;

    //stats
    int n_lps = TheMessenger.get_node_size();
    stats_received_null.resize(n_lps);
    stats_sent_null.resize(n_lps);
    for(int i=0; i<n_lps; i++) {
        stats_received_null[i] = 0;
        stats_sent_null[i] = 0;
    }

    #ifdef FORECAST_NULL
    m_outputTS = new OutputTS(n_lps);
    #endif


        null_msg_send_time = 0;
        null_msg_recv_time = 0;
        null_msg_wasted_send_time = 0;
        null_msg_wasted_recv_time = 0;
}

void CmbSyncAlg::UpdateEotSet()
{
    std::vector<LpId_t>& succs=Manifold::get_scheduler()->get_successors();
    // Check if there is a new neighbor;
    for(size_t i=0;i<succs.size();i++) {
	LpId_t succ=(succs)[i];
	if(m_eots.find(succ)==m_eots.end()) {
	    // We found a new neighbor, set EIT to 0:
	    m_eots[succ]=0.0;
	}
    }
}

void CmbSyncAlg::UpdateEitSet()
{
    std::vector<LpId_t>& preds=Manifold::get_scheduler()->get_predecessors();
    // First, check if there is a new neighbor;
    for(size_t i=0;i<preds.size();i++) {
	LpId_t pred=(preds)[i];
	if(m_eits.find(pred)==m_eits.end()) {
	    // We found a new neighbor, set EIT to 0:
	    m_eits[pred]=0.0;
	    m_in_forecast_ticks[pred] = 0;
        }
    }
  #if 0
  // Second, check if sizes equal (then we are done):
  if(preds.size()==m_eits.size()) return;

  // Third, sizes do not equal, we lost a neighbor.
  // Since searching for the lost neighbor is annoying, we copy the array and clear the old one
  ts_t eitBackup=m_eits;
  m_eits.clear();

  // Fourth, now every entry should be new, let's add it:
  for(size_t i=0;i<preds.size();i++)
  {
    LpId_t pred=(preds)[i];
    assert(m_eits.find(pred)==m_eits.end()); // no, entry should be new!
    // We found a "new" neighbor, set EIT according to backup:
    m_eits[pred]=eitBackup[pred];
  }
  #endif

    // Fifth, now we should be done, i.e. both arrays are equal:
    assert(preds.size()==m_eits.size());
}



bool CmbSyncAlg::isSafeToProcess(double requestTime)
{
    // First, check if our neighbor information is still up-to-date:
    // This performs the initialization on the first call to this function.

    if(!m_initialized) {
	UpdateEitSet();
	UpdateEotSet();
	m_initialized = true;
    }
    if(m_eits.size() == 0) //no predecessors
        return true;


#ifdef GET_NULL_MSG_TIME
    struct timeval t_start;
    gettimeofday(&t_start, NULL);
#endif
    bool got_msg = false;


    // Second, check the input buffer for pending time synch messages:
    NullMsg_t* msg;
    do {
	msg=TheMessenger.RecvPendingNullMsg();
	if(msg) {
	     got_msg = true;
	    // We have a message, let's update:
	    Time_t oldEit=m_eits[msg->src];
	    Time_t newEit=msg->t;
	    if(newEit>oldEit) m_eits[msg->src]=newEit;

	    #ifdef FORECAST_NULL
	    Ticks_t oldforecast = m_in_forecast_ticks[msg->src];
	    Ticks_t newforecast = msg->forecast;
	    if(newforecast > oldforecast) m_in_forecast_ticks[msg->src] = newforecast;
	    #endif

	    #ifdef STATS
	    stats_received_null[msg->src]++;
	    #endif
        }
    } while(msg);


    //find the min input NULL message
    ts_t::iterator it = m_eits.begin();
    m_min_null = it->second;
    ++it;

    for(; it!=m_eits.end(); ++it) {
	Time_t itTime = it->second;
	if(itTime < m_min_null) {
	    m_min_null = itTime;
	}
    }



#ifdef GET_NULL_MSG_TIME
    struct timeval t_end;
    gettimeofday(&t_end, NULL);
    double t_diff = (t_end.tv_sec - t_start.tv_sec)*1000000 + (t_end.tv_usec - t_start.tv_usec);

    if(got_msg)
        null_msg_recv_time += t_diff;
    else
        null_msg_wasted_recv_time += t_diff;
#endif

//cout << "min null= " << m_min_null << " reqTime= " << requestTime << endl;
  if(requestTime <= m_min_null) {
//cout << "min null= " << m_min_null << " reqTime= " << requestTime << "  safe" << endl;
      return true;
  }
  else
      return false;
//cout << "reqTime = " << requestTime << " safe= " << result << endl;
}



void CmbSyncAlg :: send_null_msgs()
{
    static std::vector<LpId_t>* succs=&(Manifold::get_scheduler()->get_successors());
    static int SuccsSize = succs->size();

    int src=Manifold::GetRank();


  // This function is called when the timestamp of the earliest event < min-null,
  // where min-null is the min timestamp of all the input NULL messages.
  //
  // FOREACH successor succ {
  //     send NULL = min-null + lookahead to succ
  //

    for(size_t i=0; i<SuccsSize; i++) {
	Time_t newEot = m_min_null + m_lookahead->GetLookahead(src, (*succs)[i]);
	Time_t oldEot=m_eots[i];

	//assert(newEot>=oldEot);

	if(newEot > oldEot) { // only send if necessary
//cout.precision(10);
//cout << "MMMMM " << " send null " << newEot << " to " << (*succs)[i] << " @ " << Manifold::Now() << "  min_null= " << m_min_null << endl;
	    NullMsg_t* msg = new NullMsg_t();
	    msg->src = src;
	    msg->dst = (*succs)[i];
	    m_eots[i]=newEot;
	    msg->t=newEot;
	    TheMessenger.SendNullMsg(msg);
	    delete msg;
	    #ifdef STATS
	    stats_sent_null[(*succs)[i]]++;
	    #endif
	}
    }
}


//! returs true if null-msg is sent
bool CmbSyncAlg :: send_null_msgs(Clock* clk)
{
#ifdef GET_NULL_MSG_TIME
    struct timeval t_start;
    gettimeofday(&t_start, NULL);
#endif
    bool useful_update = false;

    static std::vector<LpId_t>* succs=&(Manifold::get_scheduler()->get_successors());
    static int SuccsSize = succs->size();

    int src=Manifold::GetRank();

    // This function is called when the timestamp of the earliest event < min-null,
    // where min-null is the min timestamp of all the input NULL messages.
    //
    // FOREACH successor succ {
    //     send NULL = min-null + lookahead to succ
    //

    for(size_t i=0; i<SuccsSize; i++) {
	Time_t clockTime;
	if(clk->nextRising)
	    clockTime = clk->nextTick/clk->freq;
	else
	    clockTime = (clk->nextTick+1)/clk->freq;

	Time_t newEot = (m_min_null > clockTime) ? m_min_null : clockTime + m_lookahead->GetLookahead(src, (*succs)[i]);
	Time_t oldEot=m_eots[i];

	assert(newEot>=oldEot);

	if(newEot > oldEot) { // only send if necessary
//cout.precision(10);
//cout << "MMMMM " << " send null " << newEot << " to " << (*succs)[i] << " @ " << Manifold::NowTicks() << " rising= " << Clock::Master().nextRising << "  min_null= " << m_min_null << endl;
	    NullMsg_t* msg = new NullMsg_t();
	    msg->src = src;
	    msg->dst = (*succs)[i];
	    m_eots[i]=newEot;
	    msg->t=newEot;
	    TheMessenger.SendNullMsg(msg);
	    delete msg;
	    #ifdef STATS
	    stats_sent_null[(*succs)[i]]++;
	    #endif
	    useful_update = true;
	}
    }
#ifdef GET_NULL_MSG_TIME
    struct timeval t_end;
    gettimeofday(&t_end, NULL);
    double t_diff = (t_end.tv_sec - t_start.tv_sec)*1000000 + (t_end.tv_usec - t_start.tv_usec);

    if(useful_update) {
        null_msg_send_time += t_diff;
    }
    else
        null_msg_wasted_send_time += t_diff;
#endif

return useful_update;
}








bool CmbSyncAlg :: isSafeToProcess_send_null(double requestTime)
{
    static bool Initialized = false;

    static std::vector<LpId_t>* succs=&(Manifold::get_scheduler()->get_successors());
    static size_t SuccsSize = 0;

    if(!Initialized) {
	UpdateEitSet();
	UpdateEotSet();
	SuccsSize=succs->size();
	Initialized = true;
    }

    // Second, check the input buffer for pending time synch messages:
    NullMsg_t* msg;
    do {
	msg=TheMessenger.RecvPendingNullMsg();
	if(msg) {
	    // We have a message, let's update:
	    Time_t oldEit=m_eits[msg->src];
	    Time_t newEit=msg->t;
	    if(newEit>oldEit) m_eits[msg->src]=newEit;
	    #ifdef STATS
	    stats_received_null[msg->src]++;
	    #endif
        }
    } while(msg);

    // Third, compute time of earliest possible local event:
    Time_t earliestLocalEvent=requestTime;
    bool result=true;
    for(ts_t::iterator it=m_eits.begin();it!=m_eits.end();++it) {
	Time_t itTime=it->second;
	if(itTime<earliestLocalEvent) {
	    earliestLocalEvent=itTime;
	    result=false; // We now already know that the min EIT is smaller than requestTime
        }
    }


    int src=Manifold::GetRank();

    for(size_t i=0;i<SuccsSize;i++) {
	Time_t newEot=earliestLocalEvent + m_lookahead->GetLookahead(src, (*succs)[i]);
	Time_t oldEot=m_eots[i];
	assert(newEot>=oldEot);
    
        if(newEot>oldEot) { // only send if necessary
	    msg=new NullMsg_t();
	    msg->src = src;
	    msg->dst = (*succs)[i];
	    m_eots[i]=newEot;
	    msg->t=newEot;
	    TheMessenger.SendNullMsg(msg);
	    delete msg;
	    #ifdef STATS
	    stats_sent_null[(*succs)[i]]++;
	    #endif
	}
    }

    // Last, but not least: return result:
    return result;

}



//send null only when event is safe to process
bool CmbSyncAlg :: isSafeToProcess_send_null_if_safe(double requestTime)
{
    static bool Initialized = false;

    static std::vector<LpId_t>* succs=&(Manifold::get_scheduler()->get_successors());
    static size_t SuccsSize = 0;

    if(!Initialized) {
	UpdateEitSet();
	UpdateEotSet();
	SuccsSize=succs->size();
	Initialized = true;
    }

    // Second, check the input buffer for pending time synch messages:
    NullMsg_t* msg;
    do {
	msg=TheMessenger.RecvPendingNullMsg();
	if(msg) {
	    // We have a message, let's update:
	    Time_t oldEit=m_eits[msg->src];
	    Time_t newEit=msg->t;
	    if(newEit>oldEit) m_eits[msg->src]=newEit;
	    #ifdef STATS
	    stats_received_null[msg->src]++;
	    #endif
        }
    } while(msg);

    // Third, compute time of earliest possible local event:
    Time_t earliestLocalEvent=requestTime;
    bool result=true;
    for(ts_t::iterator it=m_eits.begin();it!=m_eits.end();++it) {
	Time_t itTime=it->second;
	if(itTime<earliestLocalEvent) {
	    earliestLocalEvent=itTime;
	    result=false; // We now already know that the min EIT is smaller than requestTime
	    return result;
	}
    }


    int src=Manifold::GetRank();

    for(size_t i=0;i<SuccsSize;i++) {
	Time_t newEot=earliestLocalEvent + m_lookahead->GetLookahead(src, (*succs)[i]);
	Time_t oldEot=m_eots[i];
	assert(newEot>=oldEot);
    
	if(newEot>oldEot) { // only send if necessary
	    msg=new NullMsg_t();
	    msg->src = src;
	    msg->dst = (*succs)[i];
	    m_eots[i]=newEot;
	    msg->t=newEot;
	    TheMessenger.SendNullMsg(msg);
	    delete msg;
	    #ifdef STATS
	    stats_sent_null[(*succs)[i]]++;
	    #endif
	}
    }

    // Last, but not least: return result:
    return result;

}



#ifdef FORECAST_NULL
//This algorithm sets the null msg time-stamp to the minimum for all successors.
void CmbSyncAlg :: send_null_msgs_with_forecast(Clock* clk)
{
#ifdef GET_NULL_MSG_TIME
    struct timeval t_start;
    gettimeofday(&t_start, NULL);
#endif
    bool useful_update = false;

    static std::vector<LpId_t>* succs=&(Manifold::get_scheduler()->get_successors());
    static int SuccsSize = succs->size();

    int src=Manifold::GetRank();

    // This function is called when the timestamp of the earliest event < min-null,
    // where min-null is the min timestamp of all the input NULL messages.
    //
    // FOREACH successor succ {
    //     send NULL = min-null + lookahead to succ
    //

    Time_t min_newEot;

    Time_t clockTime;
    if(clk->nextRising)
        clockTime = clk->nextTick/clk->freq;
    else
        clockTime = (clk->nextTick+1)/clk->freq;

    std::map<LpId_t, Ticks_t> forecast_map;

    for(size_t i=0; i<SuccsSize; i++) {
	LpId_t succ = (*succs)[i];
	Time_t newEot = (m_min_null > clockTime) ? m_min_null : clockTime + m_lookahead->GetLookahead(src, succ);

	Ticks_t forecast = 0;

	if(m_outputTS->m_ticks[src][succ] > 0)
	    forecast = m_outputTS->m_ticks[src][succ];

	if(m_outputTS->m_ticks[src][succ] > 0 && m_in_forecast_ticks[succ] > 0) {
	    Time_t null_ts = 0; //timestamp for null msg

	    if(m_outputTS->m_ticks[src][succ] > m_in_forecast_ticks[succ]) {
		const Ticks_t LINK_DELAY = 1; //assuming link delay is 1 tick

		Ticks_t tick = m_in_forecast_ticks[succ] + LINK_DELAY; //m_in_forecast_ticks is predicted send time; send_time + delay = recv_time.
		null_ts = tick/clk->freq + m_lookahead->GetLookahead(src, succ);
//cout << "succ= " << succ << " my forecast= " << m_outputTS->m_ticks[src][succ] << " other forecast= " << m_in_forecast_ticks[succ] << " use other\n";
	    }
	    else {
		null_ts = m_outputTS->m_times[src][succ] + m_lookahead->GetLookahead(src, succ);
//cout << "succ= " << succ << " my forecast= " << m_outputTS->m_ticks[src][succ] << " other forecast= " << m_in_forecast_ticks[succ] << " use mine\n";
	    }

	    if(null_ts > newEot)
		newEot = null_ts;
	    m_outputTS->m_times[src][succ] = 0;
	    m_outputTS->m_prev_ticks[src][succ] = m_outputTS->m_ticks[src][succ];
	    m_outputTS->m_ticks[src][succ] = 0;
	}

	if(i==0)
	    min_newEot = newEot;
	else if(newEot < min_newEot)
	    min_newEot = newEot;

	forecast_map[succ] = forecast;
    }//for each successor


    for(size_t i=0; i<SuccsSize; i++) {
	Time_t clockTime;
	if(clk->nextRising)
	    clockTime = clk->nextTick/clk->freq;
	else
	    clockTime = (clk->nextTick+1)/clk->freq;

	LpId_t succ = (*succs)[i];

	Time_t oldEot=m_eots[i];

	if(min_newEot > oldEot) { // only send if necessary
            NullMsg_t* msg = new NullMsg_t();
            msg->src = src;
            msg->dst = succ;
            m_eots[i]=min_newEot;
            msg->t=min_newEot;
            msg->forecast = forecast_map[succ];
            TheMessenger.SendNullMsg(msg);
            delete msg;
	    #ifdef STATS
	    stats_sent_null[succ]++;
	    #endif
	    useful_update = true;
#if 0
cout.precision(10);
cout << "NULLNULL " << " send null " << min_newEot << " to " << succ << " @ " << Manifold::NowTicks() << " rising= " << Clock::Master().nextRising << "  min_null= " << m_min_null << " forecast= " << forecast_map[succ] << "  in fcast= " << m_in_forecast_ticks[succ] << endl;
cout.flush();
#endif
	}
    }//for
#ifdef GET_NULL_MSG_TIME
    struct timeval t_end;
    gettimeofday(&t_end, NULL);
    double t_diff = (t_end.tv_sec - t_start.tv_sec)*1000000 + (t_end.tv_usec - t_start.tv_usec);

    if(useful_update)
        null_msg_send_time += t_diff;
    else
        null_msg_wasted_send_time += t_diff;
#endif
}




//This algorithm sets the null msg time-stamp to different values for the successors.
void CmbSyncAlg :: send_null_msgs_with_forecast_2(Clock* clk)
{
    static std::vector<LpId_t>* succs=&(Manifold::get_scheduler()->get_successors());
    static int SuccsSize = succs->size();

    int src=Manifold::GetRank();

    // This function is called when the timestamp of the earliest event < min-null,
    // where min-null is the min timestamp of all the input NULL messages.
    //
    // FOREACH successor succ {
    //     send NULL = min-null + lookahead to succ
    //

    for(size_t i=0; i<SuccsSize; i++) {
        Time_t clockTime;
        if(clk->nextRising)
            clockTime = clk->nextTick/clk->freq;
        else
            clockTime = (clk->nextTick+1)/clk->freq;

        LpId_t succ = (*succs)[i];

        Time_t newEot = (m_min_null > clockTime) ? m_min_null : clockTime + m_lookahead->GetLookahead(src, succ);
        Time_t oldEot=m_eots[i];

        Ticks_t forecast = 0;

	if(m_outputTS->m_ticks[src][succ] > 0)
	    forecast = m_outputTS->m_ticks[src][succ];

	if(m_outputTS->m_ticks[src][succ] > 0 && m_in_forecast_ticks[succ] > 0) {
	    Time_t null_ts = 0; //timestamp for null msg

	    if(m_outputTS->m_ticks[src][succ] > m_in_forecast_ticks[succ]) {
		const Ticks_t LINK_DELAY = 1; //assuming link delay is 1 tick

		Ticks_t tick = m_in_forecast_ticks[succ] + LINK_DELAY; //m_in_forecast_ticks is predicted send time; send_time + delay = recv_time.
		null_ts = tick/clk->freq + m_lookahead->GetLookahead(src, succ);
//cout << "succ= " << succ << " my forecast= " << m_outputTS->m_ticks[src][succ] << " other forecast= " << m_in_forecast_ticks[succ] << " use other\n";
	    }
	    else {
		null_ts = m_outputTS->m_times[src][succ] + m_lookahead->GetLookahead(src, succ);
//cout << "succ= " << succ << " my forecast= " << m_outputTS->m_ticks[src][succ] << " other forecast= " << m_in_forecast_ticks[succ] << " use mine\n";
	    }

	    if(null_ts > newEot)
		newEot = null_ts;
	    m_outputTS->m_times[src][succ] = 0;
	    m_outputTS->m_prev_ticks[src][succ] = m_outputTS->m_ticks[src][succ];
	    m_outputTS->m_ticks[src][succ] = 0;
	}


	if(newEot > oldEot) { // only send if necessary
	    NullMsg_t* msg = new NullMsg_t();
	    msg->src = src;
	    msg->dst = succ;
	    m_eots[i]=newEot;
	    msg->t=newEot;
	    msg->forecast = forecast;
	    TheMessenger.SendNullMsg(msg);
	    delete msg;
	    #ifdef STATS
	    stats_sent_null[succ]++;
	    #endif
//cout.precision(10);
//cout << "NULLNULL " << " send null " << newEot << " to " << succ << " @ " << Manifold::NowTicks() << " rising= " << Clock::Master().nextRising << "  min_null= " << m_min_null << " forecast= " << forecast << "  in fcast= " << m_in_forecast_ticks[succ] << endl;
//cout.flush();
	}
    }
}
#endif //#ifdef FORECAST_NULL





void CmbSyncAlg::PrintStats(std::ostream& out)
{
    int n_lps = TheMessenger.get_node_size();
    out << "Received Null messages:  ";
    unsigned total = 0;
    for(int i=0; i<n_lps; i++) {
        out << i << ": " << stats_received_null[i] << "  ";
	total += stats_received_null[i];
    }
    out << endl;
    out << "  Total Received Null msgs: " << total << endl;

    out << "Sent Null messages:  ";
    total = 0;
    for(int i=0; i<n_lps; i++) {
        out << i << ": " << stats_sent_null[i] << "  ";
	total += stats_sent_null[i];
    }
    out << endl;
    out << "  Total Sent Null msgs: " << total << endl;
    out << "  null stime= " << null_msg_send_time << " null rtime= " << null_msg_recv_time
        << "  waste stime= " << null_msg_wasted_send_time << " wast rtime= " << null_msg_wasted_recv_time
	<< "  total= " << (null_msg_send_time + null_msg_recv_time + null_msg_wasted_send_time + null_msg_wasted_recv_time) << endl;
}










} //kernel
} //manifold

#endif // !NO_MPI
