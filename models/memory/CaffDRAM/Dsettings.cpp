/*
 * Dsettings.cpp
 *
 *  Created on: Jun 14, 2011
 *      Author: rishirajbheda
 */

#include "Dsettings.h"
//#include <math.h>
#include <stdlib.h>
#include <assert.h>

namespace manifold {
namespace caffdram {


Dsettings::Dsettings(
	int numChannels,
	int numRanks,
	int numBanks,
	int numRows,
	int numColumns,
	int channelBitWidth,
	int llcacheLineSizeBytes,
	int memPagePolicy,
	/********************** Timing Parameters ***************/
	unsigned long t_RTRS,
	unsigned long t_OST,
	unsigned long t_BURST,
	unsigned long t_RAS,
	unsigned long t_RP,
	unsigned long t_RCD,
	unsigned long t_CCD,
	unsigned long t_CAS,
	unsigned long t_CMD,
	unsigned long t_WR,
	unsigned long t_WTR,
	unsigned long t_FAW,
	unsigned long t_RRD
	)

{
        assert(is_powerOf2(numChannels));
        assert(is_powerOf2(numRanks));
        assert(is_powerOf2(numBanks));
        assert(is_powerOf2(numRows));
        assert(is_powerOf2(numColumns));

	/********************** Memory Config *******************/
	this->numChannels = numChannels;
	this->numRanks = numRanks;
	this->numBanks = numBanks;
	this->numRows = numRows;
	this->numColumns = numColumns;
	this->channelBitWidth = channelBitWidth;
	this->llcacheLineSizeBytes = llcacheLineSizeBytes;
	this->memPagePolicy = OPEN_PAGE;
	this->numDatabusPackets = (int) ((this->llcacheLineSizeBytes*8)/this->channelBitWidth);

	/********************** Timing Parameters ***************/
	this->t_RTRS = t_RTRS;
	this->t_OST = t_OST;
	this->t_BURST = t_BURST;
	this->t_RAS = t_RAS;
	this->t_RP = t_RP;
	this->t_RCD = t_RCD;
	this->t_CCD = t_CCD;
	this->t_CAS = t_CAS;
	this->t_CMD = t_CMD;
	this->t_WR = t_WR;
	this->t_WTR = t_WTR;
	this->t_FAW = t_FAW;
	this->t_RRD = t_RRD;

	this->t_RC = this->t_RAS + this->t_RP;
	this->t_RTP = this->t_RAS - this->t_RCD;
	this->t_CWD = (this->t_CAS - 1);
	this->t_RFC = this->numRows*(this->t_RC);

	/************** Dreq Helper Variables ********************/

	this->rowShiftBits = (int) (myLog2(this->numColumns));
	this->bankShiftBits = ((int) (myLog2(this->numRows))) + this->rowShiftBits;
	this->rankShiftBits = ((int) (myLog2(this->numBanks))) + this->bankShiftBits;
	this->channelShiftBits = ((int) (myLog2(this->numRanks))) + this->rankShiftBits;
	//this->rowMask = this->getMask(this->numRows);
	//this->bankMask = this->getMask(this->numBanks);
	//this->rankMask = this->getMask(this->numRanks);
	this->rowMask = this->getMask(myLog2(this->numRows));
	this->bankMask = this->getMask(myLog2(this->numBanks));
	this->rankMask = this->getMask(myLog2(this->numRanks));
}


Dsettings::~Dsettings() {
	// TODO Auto-generated destructor stub
}

//! Some members are computed based on the value of other members. If the latter
//! are changed after construction, the former must be re-computed.
void Dsettings :: update()
{
    assert(is_powerOf2(numChannels));
    assert(is_powerOf2(numRanks));
    assert(is_powerOf2(numBanks));
    assert(is_powerOf2(numRows));
    assert(is_powerOf2(numColumns));

    this->numDatabusPackets = (int) ((this->llcacheLineSizeBytes*8)/this->channelBitWidth);

    this->t_RC = this->t_RAS + this->t_RP;
    this->t_RTP = this->t_RAS - this->t_RCD;
    this->t_CWD = (this->t_CAS - 1);
    this->t_RFC = this->numRows*(this->t_RC);

    this->rowShiftBits = (int) (myLog2(this->numColumns));
    this->bankShiftBits = ((int) (myLog2(this->numRows))) + this->rowShiftBits;
    this->rankShiftBits = ((int) (myLog2(this->numBanks))) + this->bankShiftBits;
    this->channelShiftBits = ((int) (myLog2(this->numRanks))) + this->rankShiftBits;
    //this->rowMask = this->getMask(this->numRows);
    //this->bankMask = this->getMask(this->numBanks);
    //this->rankMask = this->getMask(this->numRanks);

    this->rowMask = this->getMask(myLog2(this->numRows));
    this->bankMask = this->getMask(myLog2(this->numBanks));
    this->rankMask = this->getMask(myLog2(this->numRanks));
}


unsigned int Dsettings::getMask(unsigned int numBits)
{
	unsigned int returnMask = 0x0;
	returnMask = ~returnMask;
	returnMask = returnMask<<numBits;
	returnMask = ~returnMask;
	return (returnMask);
}

int Dsettings :: myLog2(unsigned num)
{
    assert(num > 0);

    int bits = 0;
    while(((unsigned)0x1 << bits) < num) {
	bits++;
    }
    return bits;
}

//! Test if a number is a power of 2.
bool Dsettings :: is_powerOf2(unsigned num)
{
    int bits = myLog2(num);
    return ((unsigned)(0x1 << bits) == num);
}


} //namespace caffdram
} //namespace manifold

