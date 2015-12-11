/*
 * Dreq.cpp
 *
 *  Created on: Jun 14, 2011
 *      Author: rishirajbheda
 */

#include "Dreq.h"
#include "math.h"

namespace manifold {
namespace caffdram {


Dreq::Dreq(unsigned long int reqAddr, unsigned long int currentTime, const Dsettings* dramSetting) {
	// TODO Auto-generated constructor stub

	this->generateId(reqAddr, dramSetting);
	this->originTime = currentTime + dramSetting->t_CMD;
	this->endTime = currentTime + dramSetting->t_CMD;
}

Dreq::~Dreq() {
	// TODO Auto-generated destructor stub
}

void Dreq::generateId(unsigned long int reqAddr, const Dsettings* dramSetting)
{
	this->rowId = (reqAddr>>(dramSetting->rowShiftBits))&(dramSetting->rowMask);
	this->bankId = (reqAddr>>(dramSetting->bankShiftBits))&(dramSetting->bankMask);
	this->rankId = (reqAddr>>(dramSetting->rankShiftBits))&(dramSetting->rankMask);
	if (dramSetting->numChannels == 1)
	{
		this->chId = 0;
	}
	else
	{
		this->chId = (reqAddr>>(dramSetting->channelShiftBits))%(dramSetting->numChannels);
	}
}



} //namespace caffdram
} //namespace manifold

