#ifndef MESI_CLIENT_H
#define MESI_CLIENT_H

#include "ClientInterface.h"
#include "MESI_enum.h"

namespace manifold {
namespace mcp_cache_namespace {


class MESI_client : public ClientInterface
{
    public:
        MESI_client(int id);
        ~MESI_client();

	void process(int msg_type, int fwd_id);

        /** Used by the cache to tell whether a block is 'busy' or not. */
        bool req_pending();
        /** Client begins data and read permission acquistion within its native coherence realm L1/Lower Manager expects GrantReadD upon completion */
        void GetReadD();
        /** Client begins data and write permission acquistion within its native coherence realm L1/Lower Manager expects GrantWriteD upon completion */
        void GetWriteD();
        /** Client begins write permission acquistion within its native coherence realm L1/Lower Manager expects GrantWrite or GrantWriteD upon completion */
        void GetWrite();
        /** Client begins eviction sequence within its native coherence realm L1/Lower Manager expects GrantWrite or GrantWriteD upon completion */
        void GetEvict();

        /** Returns true if data exists or can be supplied to the manager */
        bool HaveData();
        /** Returns true if you have read permissions */
        bool HaveReadP();
        /** Returns true if you have write permissions */
        bool HaveWriteP();
        /** Returns true if you can be safely evicted */
        bool HaveEvictP();

        /** Called by the Manager to complete previous Supply demand. Supplies data packet */
        void SupplyAck();
        /** Called by the Manager to complete previous Invalidate demand.  Signifies realm invalidation has completed */
        void InvalidateAck();
        /** Called by the Manager to complete previous SupplyDowngrade demand.  Supplies data packet and signifies realm downgrade has completed */
        void SupplyDowngradeAck();
        /** Called by the Manager to complete previous SupplyInvalidate demand.  Supplies data packet and signifies realm invalidation has completed */
        void SupplyInvalidateAck();


    protected:
	//! @param \c req  Whether it's an initial request or a reply.
	//! @param \c destID  Destination ID; -1 means manager.
	virtual void sendmsg(bool req, MESI_messages_t msg, int destID=-1) = 0;
	virtual void invalidate() = 0;

	MESI_client_state_t get_state() { return state; }


#ifndef MCP_CACHE_UTEST
    private:
#else
    public:
#endif
        MESI_client_state_t state;

	inline void do_I(MESI_messages_t msg);
	inline void do_S(MESI_messages_t msg);
	inline void do_E(MESI_messages_t msg, int fwdID);
	inline void do_M(MESI_messages_t msg, int fwdID);
	inline void do_IE(MESI_messages_t msg);
	inline void do_SE(MESI_messages_t msg);

	inline void do_EI(MESI_messages_t msg, int fwdID);
	inline void do_MI(MESI_messages_t msg, int fwdID);
	inline void do_SIE(MESI_messages_t msg);

        void transition_to_i();
        void transition_to_s();
        void transition_to_e();
        void transition_to_m();
        void transition_to_ie();
        void transition_to_ei();
        void transition_to_mi();
        //void transition_to_sie();
        void transition_to_se();
        void invalid_msg(MESI_messages_t);
        void invalid_state();


};

}
}
#endif
