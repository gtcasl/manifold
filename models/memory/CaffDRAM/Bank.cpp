/*
 * Bank.cpp
 *
 *  Created on: Jun 14, 2011
 *      Author: rishirajbheda
 */

#include "Bank.h"
#include "kernel/manifold.h"


using namespace std;


namespace manifold {
namespace caffdram {


Bank::Bank(const Dsettings* dramSetting) {
	// TODO Auto-generated constructor stub

	this->dramSetting = dramSetting;
	this->firstAccess = true;
	this->lastAccessedRow = 0;
	this->bankAvailableAtTime = 0;

	stats = new Bank_stat_engine();
}

Bank::~Bank() {
	// TODO Auto-generated destructor stub
	delete stats;

}


//private copy contructor only used by unit test
Bank :: Bank(const Bank& b)
{
    *this = b;
    this->stats = new Bank_stat_engine();
}




unsigned long int Bank::processRequest(Dreq* dRequest)
{
#ifdef DBG_CAFFDRAM
cout << "request originTime= " << dRequest->get_originTime() << "  Bank available time= "
     << bankAvailableAtTime << endl;
#endif
	//req_info info;
	//info.old_avail = bankAvailableAtTime;
	//info.orig = dRequest->get_originTime();

        pair<uint64_t, uint64_t> busy;

	unsigned long int returnLatency = 0;
	//bool org_less_than_avail = false;
	if (dRequest->get_originTime() <= this->bankAvailableAtTime)
	{
		//org_less_than_avail = true;
		busy.first = bankAvailableAtTime; //become busy starting at bankAvailableAtTime
	}
	else
	{
		busy.first = dRequest->get_originTime(); //become busy starting at origin time
		this->bankAvailableAtTime = dRequest->get_originTime();
	}

	if (this->dramSetting->memPagePolicy == OPEN_PAGE)
	{
		returnLatency = this->processOpenPageRequest(dRequest);
	}
	else
	{
		returnLatency = this->processClosedPageRequest(dRequest);
	}

	//info.new_avail = bankAvailableAtTime;
	//reqs.push_back(info);

	busy.second = bankAvailableAtTime; //bankAvailableAtTime has been updated

	busy.first = bankAvailableAtTime - busy.first; //busy for this number of cycles due to this request

	if(stats_busy_cycles.size() > 0)
	    busy.first += stats_busy_cycles.back().first; //accumulate
	assert(busy.first <= busy.second);

        stats_busy_cycles.push_back(busy);
	if(stats_busy_cycles.size() > 100) //only keep 100 pairs
	    stats_busy_cycles.pop_front();

        stats->num_requests++;
	stats->latencies.collect(returnLatency - dRequest->get_originTime());

	return (returnLatency);
}

unsigned long int Bank::processOpenPageRequest(Dreq* dRequest)
{
	unsigned long int returnLatency = 0;

	if (this->firstAccess == false)
	{
		if (dRequest->get_rowId() == this->lastAccessedRow)
		{
			this->bankAvailableAtTime += this->dramSetting->t_CAS;
		}
		else
		{
			this->bankAvailableAtTime += this->dramSetting->t_RP;
			this->bankAvailableAtTime += (this->dramSetting->t_RCD + this->dramSetting->t_CAS);
		}
	}
	else
	{
		this->bankAvailableAtTime += (this->dramSetting->t_RCD + this->dramSetting->t_CAS);
		this->firstAccess = false;
	}
	unsigned long endT = this->bankAvailableAtTime + this->dramSetting->t_BURST;
	dRequest->set_endTime(endT);
	returnLatency = endT;
	this->lastAccessedRow = dRequest->get_rowId();
	return (returnLatency);
}

unsigned long int Bank::processClosedPageRequest(Dreq* dRequest)
{
	unsigned long int returnLatency = 0;
	this->bankAvailableAtTime += (this->dramSetting->t_RCD + this->dramSetting->t_CAS);
	this->bankAvailableAtTime += this->dramSetting->t_RP;
	unsigned long endT = this->bankAvailableAtTime + this->dramSetting->t_BURST;
	dRequest->set_endTime(endT);
	returnLatency = endT;
	return (returnLatency);
}


void Bank::print_stats (ostream & out)
{
    stats->print_stats(out);
    uint64_t endTick = manifold::kernel::Manifold::NowTicks();
#if 0
for(list<req_info>::iterator it=reqs.begin(); it != reqs.end(); ++it) {
out << "(" << it->old_avail << ", " << it->orig << ", " << it->new_avail << ") ";
}
out<<endl;
for(list<pair<uint64_t, uint64_t> >::iterator it=stats_busy_cycles.begin(); it != stats_busy_cycles.end(); ++it) {
out << "(" << it->first << ", " << it->second << ") ";
}
out<<endl;
#endif
    if(stats_busy_cycles.size() > 0) {
        if(stats_busy_cycles.back().second > endTick) {
	    list<pair<uint64_t, uint64_t> >::iterator it = stats_busy_cycles.begin();
	    for(; it != stats_busy_cycles.end(); ++it) {
	        if(it->second > endTick)
		    break;
	    }
	    out << "busy cycles= " << it->first << "(" << (it->first) / ((double)endTick) * 100 << "%)";
	    if(it == stats_busy_cycles.begin())
	        out << " **";
	}
	else {
	    out << "busy cycles= " << stats_busy_cycles.back().first << "(" << (stats_busy_cycles.back().first)/((double)endTick) * 100 << "%)";
	}
	out << endl;
    }
    else {
        out << "busy cycles= 0\n";
    }
}


Bank_stat_engine::Bank_stat_engine () : Stat_engine(),
num_requests("Number of Requests", ""),
latencies("Average Memory Latency", "", 1000, 5, 0)
{
}

Bank_stat_engine::~Bank_stat_engine ()
{
}

void Bank_stat_engine::global_stat_merge(Stat_engine * e)
{
    Bank_stat_engine * global_engine = (Bank_stat_engine*)e;
    global_engine->num_requests += num_requests.get_value();
    global_engine->latencies.merge(&latencies);
}

void Bank_stat_engine::print_stats (ostream & out)
{
    num_requests.print(out);
    latencies.print(out);
    //out << latencies.get_average() << endl;
}

void Bank_stat_engine::clear_stats()
{
    num_requests.clear();
    latencies.clear();
}

void Bank_stat_engine::start_warmup () 
{
}

void Bank_stat_engine::end_warmup () 
{
}

void Bank_stat_engine::save_samples () 
{
}


} //namespace caffdram
} //namespace manifold

