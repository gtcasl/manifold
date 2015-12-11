#ifndef MANAGERINTERFACE_H
#define MANAGERINTERFACE_H

#include <stdint.h>


namespace manifold {
namespace mcp_cache_namespace {


class ManagerInterface
{
    public:
        ManagerInterface(int id);
        virtual ~ManagerInterface();

        int getManagerID() { return id; }

        virtual bool process_lower_client_request(void* req, bool first) = 0;
	virtual void process_lower_client_reply(void* req) = 0;

	//Test if a lower client request is an invalidation request.
	virtual bool is_invalidation_request(void* req) = 0;



        /** Used by the cache to tell whether a block is 'busy' or not. */
        virtual bool req_pending() =0;

        /** Demand data supply from lower tier's paired manager or L1 */
        virtual void Supply() = 0;
        /** Demand lower realm to forfeit both read and write permissions, invaliding all local copies of data */
        virtual void Invalidate() = 0;
        /** Demand data from lower realm's paired manager.  The lower realm must forfeit write permissions but can retain read permissions and data */
        virtual void SupplyDowngrade() = 0;
        /** Demand data from lower realm's paired manager.  The lower realm must forfeit write permissions and read permissions, invalidating all local copies of data */
        virtual void SupplyInvalidate() = 0;

	//Strictly speaking, Evict() is not necessary since it can be achieved with Invalidate().
	virtual void Evict() = 0;


        /** Called from the client to complete previous GetReadD request.  Supplies data packed and signifies paired client now has read permissions */
        virtual void GrantReadD() = 0;
        /** Called from the client to complete previous GetWrite/GetWriteD request. Supplies data packet and signifies paired client now has write permissions */
        virtual void GrantWriteD() = 0;
        /** Called from the client to complete previous GetWrite request. Supplies data packet and signifies paired client now has write permissions */
        virtual void GrantWrite() = 0;
        /** Called from the client to complete previous GetEvict request. Supplies data packet and signifies paired client is now invalid */
        virtual void GrantEvict() = 0;


    protected:
        int id;

};

} //namespace mcp_cache_namespace
} //namespace manifold


#endif
