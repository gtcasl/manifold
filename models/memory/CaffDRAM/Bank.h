/*
 * Bank.h
 *
 *  Created on: Jun 14, 2011
 *      Author: rishirajbheda
 */

#ifndef MANIFOLD_CAFFDRAM_BANK_H_
#define MANIFOLD_CAFFDRAM_BANK_H_

#include "Dsettings.h"
#include "Dreq.h"
#include "kernel/stat_engine.h"
#include "kernel/stat.h"

#include <iostream>
#include <list>
#include <utility>
#include <stdint.h>

/*
 *
 */

namespace manifold {
namespace caffdram {


class Bank_stat_engine;

class Bank {
public:

	unsigned long int processRequest(Dreq* dRequest);

	Bank(const Dsettings* dramSetting);
	~Bank();

        Bank_stat_engine* get_stats() { return stats; }

        void print_stats(ostream& out);

#ifdef CAFFDRAM_TEST
public:
#else
private:
#endif
        Bank(const Bank& b); //for unit test only

	const Dsettings* dramSetting;
	unsigned int lastAccessedRow;
	bool firstAccess;
	unsigned long int bankAvailableAtTime;

	unsigned long int processOpenPageRequest (Dreq* dRequest);
	unsigned long int processClosedPageRequest (Dreq* dRequest);


	//stats
	friend class Bank_stat_engine;
	Bank_stat_engine *stats;

        //stats on number of cycles the bank is busy; here a pair (x, y) is used: meaning between cycles 0 and y
	//there are x cycles the bank is busy.
	//The reason for using a list is as follows: CaffDRAM doesn't register with a clock, so there is not a
	//function that is called every cycle. The simulation end time E is likely different from the last pair's
	//y value. To find out the num of busy cycles between 0 and E, we simply find the first pair whose y value
	// is greater than or equal to E. Another case is that the y value of the last pair is < E.
	//The size of the list could be big, so we limit the size to 100. Therefore, if the y value of the very
	//first pair is greater than E, the computed number of busy cycles is only an approximate.
	std::list<std::pair<uint64_t, uint64_t> > stats_busy_cycles;

	#if 0
	struct req_info {
	    uint64_t old_avail; //bank's available time when the request is started
	    uint64_t orig; //the origin time of the request
	    uint64_t new_avail; //bank's available time when the request is finished
	};
	std::list<req_info> reqs;
	#endif

};



class Bank_stat_engine : public manifold::kernel::Stat_engine
{
    public:
        Bank_stat_engine ();
        ~Bank_stat_engine ();

        // Stats
        manifold::kernel::Persistent_stat<manifold::kernel::counter_t> num_requests;
        manifold::kernel::Persistent_histogram_stat<manifold::kernel::counter_t> latencies;

        void global_stat_merge(Stat_engine * e);
        void clear_stats();
        void print_stats (ostream & out);

        void start_warmup ();
        void end_warmup ();	
        void save_samples ();
};

} //namespace caffdram
} //namespace manifold

#endif // MANIFOLD_CAFFDRAM_BANK_H_
