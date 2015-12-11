/*
 * Rank.h
 *
 *  Created on: Jun 14, 2011
 *      Author: rishirajbheda
 */

#ifndef MANIFOLD_CAFFDRAM_RANK_H_
#define MANIFOLD_CAFFDRAM_RANK_H_

#include "Dsettings.h"
#include "Bank.h"
#include "Dreq.h"
#include "kernel/stat_engine.h"

#include <iostream>

namespace manifold {
namespace caffdram {


class Rank {
public:
	unsigned long int processRequest (Dreq* dRequest);

	Rank(const Dsettings* dramSetting);
	~Rank();

        void print_stats(ostream& out);

#ifdef CAFFDRAM_TEST
public:
#else
private:
#endif
        Rank(const Rank&); //for unit test only


	const Dsettings* dramSetting;
	Bank** myBank;

};





} //namespace caffdram
} //namespace manifold

#endif // MANIFOLD_CAFFDRAM_RANK_H_
