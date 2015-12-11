#include "ControllerSimple.h"
#include "kernel/component.h"
#include "kernel/manifold.h"

#include <iostream>

using namespace std;
using namespace manifold::kernel;


namespace manifold {
namespace caffdram {


ControllerSimple::ControllerSimple(int nid, const Dsettings& s, bool resp) :
	m_send_st_response(resp)
{
	m_nid = nid;

	this->dramSetting = new Dsettings (s);
	this->dramSetting->update(); //call update in case fileds of s were changed after construction.

	this->myChannel = new Channel*[this->dramSetting->numChannels];
	for (int i = 0; i < this->dramSetting->numChannels; i++)
	{
		this->myChannel[i] = new Channel (this->dramSetting);
	}

	//stats
	this->stats_max_output_buffer_size = 0;
}

ControllerSimple :: ~ControllerSimple() {

	for (int i = 0; i < this->dramSetting->numChannels; i++)
	{
		delete this->myChannel[i];
	}
	delete [] this->myChannel;
	delete this->dramSetting;
}

unsigned long int ControllerSimple::processRequest(unsigned long int reqAddr, unsigned long int currentTime)
{
	Dreq* dRequest = new Dreq (reqAddr, currentTime, this->dramSetting);
#ifdef DBG_CAFFDRAM
cout << "ch-rank-bank = " << dRequest->get_chId() << "-" << dRequest->get_rankId() << "-" << dRequest->get_bankId() << endl;
#endif
	unsigned long int returnLatency = this->myChannel[dRequest->get_chId()]->processRequest(dRequest);
	delete dRequest;
	return (returnLatency);
}



void ControllerSimple :: print_config(ostream& out)
{
    out << "***** CaffDRAM " << m_nid << " config *****" << endl;
    out << "  send store response: " << (m_send_st_response ? "yes" : "no") << endl;
    out << "  num of channels = " << dramSetting->numChannels << endl
        << "  num of ranks = " << dramSetting->numRanks << endl
        << "  num of banks = " << dramSetting->numBanks << endl
        << "  num of rows = " << dramSetting->numRows << endl
        << "  num of columns = " << dramSetting->numColumns << endl
        << "  num of columns = " << dramSetting->numColumns << endl
        << "  page policy = " << ((dramSetting->memPagePolicy == OPEN_PAGE) ? "Open page" : "Closed page") << endl
	<< "  t_RTRS (rank to rank switching time) = " << dramSetting->t_RTRS << endl
	<< "  t_OST (ODT switching time) = " << dramSetting->t_OST << endl
	<< "  t_BURST (burst length on mem bus) = " << dramSetting->t_BURST << endl
	<< "  t_CWD (column write delay) = " << dramSetting->t_CWD << endl
	<< "  t_RAS (row activation time) = " << dramSetting->t_RAS << endl
	<< "  t_RP (row precharge time) = " << dramSetting->t_RP << endl
	<< "  t_RTP (read to precharge delay) = " << dramSetting->t_RTP << endl
	<< "  t_CCD (column to column delay) = " << dramSetting->t_CCD << endl
	<< "  t_CAS (column access latency) = " << dramSetting->t_CAS << endl
	<< "  t_RCD (row to column delay) = " << dramSetting->t_RCD << endl
	<< "  t_CMD (command delay) = " << dramSetting->t_CMD << endl
	<< "  t_RFC (refresh cycle time) = " << dramSetting->t_RFC << endl
	<< "  t_WR (write recovery time) = " << dramSetting->t_WR << endl
	<< "  t_WTR (write to read delay) = " << dramSetting->t_WTR << endl
	<< "  t_FAW (four bank activation window) = " << dramSetting->t_FAW << endl
	<< "  t_RRD (row to row activation delay) = " << dramSetting->t_RRD << endl
	<< "  t_RC (row cycle time) = " << dramSetting->t_RC << endl
	<< "  t_REF_INT (refresh interval) = " << dramSetting->t_REF_INT << endl;
}

void ControllerSimple :: print_stats(ostream& out)
{
    out << "********** CaffDRAM " << m_nid << " stats **********" << endl;
    out << "max output buffer size= " << stats_max_output_buffer_size << endl;
    out << "=== Load misses ===" << endl;
        out << "    " << stats_ld_misses << endl;

    out << "=== stores ===" << endl;
        out << "    " << stats_stores << endl;


    for (int i = 0; i < this->dramSetting->numChannels; i++) {
	out << "Channel " << i << ":" << endl;
	myChannel[i]->print_stats(out);
    }
}





} //namespace caffdram
} //namespace manifold

