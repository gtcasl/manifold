/*
 * Dsettings.h
 *
 *  Created on: Jun 14, 2011
 *      Author: rishirajbheda
 */

#ifndef MANIFOLD_CAFFDRAM_DSETTINGS_H_
#define MANIFOLD_CAFFDRAM_DSETTINGS_H_



namespace manifold {
namespace caffdram {


typedef enum {

	OPEN_PAGE = 0,
	CLOSED_PAGE
} pagePolicy;

/*
 *
 */
class Dsettings {
#ifdef CAFFDRAM_TEST
public:
#else
private:
#endif
	unsigned int getMask (unsigned numBits);
	static int myLog2(unsigned num);
	static bool is_powerOf2(unsigned num);

public:
	/******************* Memory Config *****************/
	int numChannels;
	int numRanks;
	int numBanks;
	int numRows;
	int numColumns;
	int channelBitWidth;
	int llcacheLineSizeBytes;
	int numDatabusPackets;
	pagePolicy memPagePolicy;

	/******************* Timing Parameters *************/

	unsigned long int t_RTRS;					// t_RTRS  	: Rank to Rank Switching time
	unsigned long int t_OST;					// t_OST   	: ODT Switching time
	unsigned long int t_BURST;					// t_BURST 	: Burst length on memory bus
	unsigned long int t_CWD;					// t_CWD	: Column Write Delay
	unsigned long int t_RAS;					// t_RAS 	: Row activation time
	unsigned long int t_RP;						// t_RP  	: Row Precharge time
	unsigned long int t_RTP;					// t_RTP 	: Read to Precharge delay
	unsigned long int t_CCD;					// t_CCD 	: Column to Column delay
	unsigned long int t_CAS;					// t_CAS 	: Column Access Latency
	unsigned long int t_RCD;					// t_RCD	: Row to Column Delay
	unsigned long int t_CMD;					// t_CMD	: Command Delay on Command Bus
	unsigned long int t_RFC;					// t_RFC	: Refresh cycle time
	unsigned long int t_WR; 					// t_WR  	: Write Recovery time
	unsigned long int t_WTR;					// t_WTR 	: Write to Read delay
	unsigned long int t_FAW;					// t_FAW	: Four Bank Activation Window
	unsigned long int t_RRD;					// t_RRD	: Row to Row Activation Delay
	unsigned long int t_RC;						// t_RC		: Row Cycle time
	unsigned long int t_REF_INT;				// t_REF_INT: Refresh Interval Period

	/*********** Dreq Helper Variables ****************/

	unsigned int rowShiftBits;
	unsigned int bankShiftBits;
	unsigned int rankShiftBits;
	unsigned int channelShiftBits;
	unsigned int rowMask;
	unsigned int bankMask;
	unsigned int rankMask;


	Dsettings(
	    int numChannels = 1,
	    int numRanks = 2,
	    int numBanks = 8,
	    int numRows = 4096,
	    int numColumns = 1024,
	    int channelBitWidth = 64,
	    int llcacheLineSizeBytes = 64,
	    int memPagePolicy = OPEN_PAGE,

	    /********************** Timing Parameters ***************/

	    unsigned long t_RTRS = 2,
	    unsigned long t_OST = 2,
	    unsigned long t_BURST = 20,
	    unsigned long t_RAS = 8,
	    unsigned long t_RP = 25,
	    unsigned long t_RCD = 25,
	    unsigned long t_CCD = 2,
	    unsigned long t_CAS = 25,
	    unsigned long t_CMD = 10,
	    unsigned long t_WR = 6,
	    unsigned long t_WTR = 3,
	    unsigned long t_FAW = 0,
	    unsigned long t_RRD = 0
	);

	~Dsettings();

	void update();
};

} //namespace caffdram
} //namespace manifold


#endif // MANIFOLD_CAFFDRAM_DSETTINGS_H_
