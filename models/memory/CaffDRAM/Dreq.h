/*
 * Dreq.h
 *
 *  Created on: Jun 14, 2011
 *      Author: rishirajbheda
 */

#ifndef MANIFOLD_CAFFDRAM_DREQ_H_
#define MANIFOLD_CAFFDRAM_DREQ_H_

#include "Dsettings.h"

/*
 *
 */
namespace manifold {
namespace caffdram {

class Dreq {
public:

	Dreq(unsigned long int reqAddr, unsigned long int currentTime, const Dsettings* dramSetting);
	~Dreq();

        unsigned long get_originTime() { return originTime; }
        unsigned long get_endTime() { return endTime; }
	void set_endTime(unsigned long t) { endTime = t; }

	unsigned get_chId() { return chId; }
	unsigned get_rankId() { return rankId; }
	unsigned get_bankId() { return bankId; }
	unsigned get_rowId() { return rowId; }

#ifdef CAFFDRAM_TEST
public:
#else
private:
#endif
	void generateId (unsigned long int reqAddr, const Dsettings* dramSetting);

	unsigned long int originTime;
	unsigned long int endTime;
	unsigned int chId;
	unsigned int rankId;
	unsigned int bankId;
	unsigned int rowId;

};

} //namespace caffdram
} //namespace manifold

#endif // MANIFOLD_CAFFDRAM_DREQ_H_
