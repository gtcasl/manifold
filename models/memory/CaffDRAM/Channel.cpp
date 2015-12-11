/*
 * Channel.cpp
 *
 *  Created on: Jun 14, 2011
 *      Author: rishirajbheda
 */

#include "Channel.h"

using namespace std;


namespace manifold {
namespace caffdram {

Channel::Channel(const Dsettings* dramSetting) {
	// TODO Auto-generated constructor stub

	this->dramSetting = dramSetting;
	this->myRank = new Rank*[this->dramSetting->numRanks];
	for (int i = 0; i < this->dramSetting->numRanks; i++)
	{
		this->myRank[i] = new Rank(this->dramSetting);
	}
}

Channel::~Channel() {
	// TODO Auto-generated destructor stub

	for (int i = 0; i < this->dramSetting->numRanks; i++)
	{
		delete this->myRank[i];
	}
	delete [] this->myRank;
}


//private copy constructor for unit test only
Channel :: Channel(const Channel& c)
{
    *this = c;
}


unsigned long int Channel::processRequest(Dreq* dRequest)
{
	unsigned long int returnLatency = 0;
	returnLatency = this->myRank[dRequest->get_rankId()]->processRequest(dRequest);
	return (returnLatency);
}


void Channel::print_stats (ostream & out)
{
    for (int i = 0; i < dramSetting->numRanks; i++) {
	out << "Rank " << i << ":" << endl;
	myRank[i]->print_stats(out);
    }

}




} //namespace caffdram
} //namespace manifold

