/*
 * Rank.cpp
 *
 *  Created on: Jun 14, 2011
 *      Author: rishirajbheda
 */

#include "Rank.h"

using namespace std;


namespace manifold {
namespace caffdram {

Rank::Rank(const Dsettings* dramSetting) {
	// TODO Auto-generated constructor stub

	this->dramSetting = dramSetting;

	this->myBank = new Bank*[this->dramSetting->numBanks];
	for (int i = 0; i < this->dramSetting->numBanks; i++)
	{
		this->myBank[i] = new Bank(this->dramSetting);
	}
	
}

Rank::~Rank() {
	// TODO Auto-generated destructor stub

	for (int i = 0; i < this->dramSetting->numBanks; i++)
	{
		delete this->myBank[i];
	}
	delete [] this->myBank;

}


Rank :: Rank(const Rank& r)
{
    *this = r;
}


unsigned long int Rank::processRequest(Dreq* dRequest)
{
	unsigned long int returnLatency = 0;
	returnLatency = this->myBank[dRequest->get_bankId()]->processRequest(dRequest);
	return (returnLatency);
}


void Rank::print_stats (ostream & out)
{
     for (int i = 0; i < dramSetting->numBanks; i++) {
	out << "Bank " << i << ":" << endl;
	myBank[i]->print_stats(out);
    }
}


} //namespace caffdram
} //namespace manifold

