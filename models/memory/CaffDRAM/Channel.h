/*
 * Channel.h
 *
 *  Created on: Jun 14, 2011
 *      Author: rishirajbheda
 */

#ifndef MANIFOLD_CAFFDRAM_CHANNEL_H_
#define MANIFOLD_CAFFDRAM_CHANNEL_H_

#include "Rank.h"
#include "Dsettings.h"
#include "Dreq.h"
#include "kernel/stat_engine.h"

/*
 *
 */
namespace manifold {
namespace caffdram {

class Channel_stat_engine;

class Channel {
public:
	unsigned long int processRequest (Dreq* dRequest);

	Channel(const Dsettings* dramSetting);
	~Channel();


        void print_stats(ostream& out);

#ifdef CAFFDRAM_TEST
public:
#else
private:
#endif
	Channel(const Channel&); //for unit test only


	const Dsettings* dramSetting;
	Rank** myRank;

};




} //namespace caffdram
} //namespace manifold

#endif // MANIFOLD_CAFFDRAM_CHANNEL_H_
