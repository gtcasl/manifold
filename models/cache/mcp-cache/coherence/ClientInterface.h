#ifndef CLIENTINTERFACE_H
#define CLIENTINTERFACE_H

#include <stdint.h>


namespace manifold {
namespace mcp_cache_namespace {

class hash_entry;

typedef enum {
    NO_CLIENT = 0,
    SIMPLE_CLIENT,
    MESI_CLIENT
} ClientInterface_t;


class ClientInterface
{
    public:
        /** Constructor */
        ClientInterface(int id);
        /** Destructor */
        virtual ~ClientInterface();

        int getClientID() { return id; }

        //virtual void process(cache_req *request) =0;
	//! process an incoming request.
        virtual void process(int type, int fwdId) = 0;

        /** Used by the cache to tell whether a block is 'busy' or not. */
        virtual bool req_pending() =0;

        /** Client begins data and read permission acquistion within its native coherence realm L1/Lower Manager expects GrantReadD upon completion */
        virtual void GetReadD() = 0;
        /** Client begins data and write permission acquistion within its native coherence realm L1/Lower Manager expects GrantWriteD upon completion */
        virtual void GetWriteD() = 0;
        /** Client begins write permission acquistion within its native coherence realm L1/Lower Manager expects GrantWrite or GrantWriteD upon completion */
        virtual void GetWrite() = 0;
        /** Client begins eviction sequence within its native coherence realm L1/Lower Manager expects GrantWrite or GrantWriteD upon completion */
        virtual void GetEvict() = 0;

        /** Returns true if data exists or can be supplied to the manager */
        virtual bool HaveData() = 0;
        /** Returns true if you have read permissions */
        virtual bool HaveReadP() = 0;
        /** Returns true if you have write permissions */
        virtual bool HaveWriteP() = 0;
        /** Returns true if you can be safely evicted */
        virtual bool HaveEvictP() = 0;

        /** Called by the Manager to complete previous Supply demand. Supplies data packet */
        virtual void SupplyAck() = 0;
        /** Called by the Manager to complete previous Invalidate demand.  Signifies realm invalidation has completed */
        virtual void InvalidateAck() = 0;
        /** Called by the Manager to complete preivous SupplyDowngrade demand.  Supplies data packet and signifies realm downgrade has completed */
        virtual void SupplyDowngradeAck() = 0;
        /** Called by the Manager to complete preivous SupplyInvalidate demand.  Supplies data packet and signifies realm invalidation has completed */
        virtual void SupplyInvalidateAck() = 0;


    protected:
        /** ID of the client */
        int id;
        //hash_entry *my_entry;

    private:

};

}
}
#endif
