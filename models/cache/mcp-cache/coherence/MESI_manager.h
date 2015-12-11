#ifndef MESI_MANAGER_H
#define MESI_MANAGER_H

#include "ManagerInterface.h"
#include "MESI_enum.h"
#include "sharers.h"

namespace manifold {
namespace mcp_cache_namespace {


class MESI_manager : public ManagerInterface
{
    public:
        /** Constructor */
        MESI_manager(int id);
        /** Destructor */
        ~MESI_manager();

        /** Used by the cache to tell whether a block is 'busy' or not. */
        bool req_pending();


        /** Demand data supply from lower tier's paired manager or L1*/
        void Supply();
        /** Demand lower realm to forfeit both read and write permissions, invaliding all local copies of data */
        void Invalidate();
        /** Demand data from lower realm's paired manager.  The lower realm must forfeit write permissions but can retain read permissions and data */
        void SupplyDowngrade();
        /** Demand data from lower realm's paired manager.  The lower realm must forfeit write permissions and read permissions, invalidating all local copies of data */
        void SupplyInvalidate();

	void Evict();

        /** Called from the client to complete previous GetReadD request.  Supplies data packed and signifies paired client now has read permissions */
        void GrantReadD();
        /** Called from the client to complete previous GetWrite/GetWriteD request. Supplies data packet and signifies paired client now has write permissions */
        void GrantWriteD();
        /** Called from the client to complete previous GetWrite request. Supplies data packet and signifies paired client now has write permissions */
        void GrantWrite();
        /** Called from the client to complete previous GetEvict request. Supplies data packet and signifies paired client is now invalid */
        void GrantEvict();

	bool IsRequestComplete();


#ifndef MCP_CACHE_UTEST
    protected:
#else
    public:
#endif
	void process(int msg_type, int src_id);

	//! @param \c req  Whether it's an initial request or a reply.
	virtual void sendmsg(bool req, MESI_messages_t msg, int destID, int fwdID = -1) = 0;
	virtual void client_writeback() = 0; //subclass implements this to notify that client requests writeback.
	virtual void invalidate() = 0;
	virtual void ignore() = 0; //called when a request should simply be dropped.



#ifndef MCP_CACHE_UTEST
    private:
#else
    public:
#endif
        /** Current Owner */
        int owner;

        MESI_manager_state_t state;
        sharers sharersList;
        int num_invalidations;
        int num_invalidations_req;

        bool unblocked_s_recv;
        bool clean_wb_recv;

	inline void do_I(MESI_messages_t msg_type, int srcID);
	inline void do_E(MESI_messages_t msg_type, int srcID);
	inline void do_S(MESI_messages_t msg_type, int srcID);
	inline void do_IE(MESI_messages_t msg_type, int srcID);
	inline void do_EE(MESI_messages_t msg_type, int srcID);
	inline void do_EI_PUT(MESI_messages_t msg_type, int srcID);
	inline void do_EI_EVICT(MESI_messages_t msg_type);
	inline void do_ES(MESI_messages_t msg_type, int srcID);
	inline void do_SS(MESI_messages_t msg_type, int srcID);
	inline void do_SIE(MESI_messages_t msg_type, int srcID);
	inline void do_SI_EVICT(MESI_messages_t msg_type, int srcID);

        void transition_to_i();
        void transition_to_e();
        void transition_to_s();
        void transition_to_ie();
        void transition_to_ee();
        void transition_to_ei_put();
        void transition_to_ei_evict();
        void transition_to_es();
        void transition_to_ss();
        void transition_to_sie();
        void transition_to_si_evict();

        void invalid_msg( MESI_messages_t );

        void sendmsgtosharers(MESI_messages_t msg);
};

}
}
#endif
