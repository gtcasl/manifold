/*
 * =====================================================================================
 *
 *       Filename:  linkData.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/14/2011 02:35:25 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Mitchelle Rasquinha
 *        Company:  Georgia Institute of Technology
 *
 * =====================================================================================
 */

#ifndef  MANIFOLD_IRIS_LINKDATA_H
#define  MANIFOLD_IRIS_LINKDATA_H

#include	"../interfaces/genericHeader.h"
#include	"flit.h"
#include	<cstring>
#include "kernel/serialize.h"


namespace manifold {
namespace iris {

enum link_arrival_data_type { FLIT, CREDIT };

class LinkData
{
    public:
        ~LinkData();

        link_arrival_data_type type;
        uint vc;
        Flit *f; //Note LinkData doesn't own the flit, so the flit is deleted separately.
        uint src;

        std::string toString(void) const;

};

} // namespace iris
} // namespace manifold


namespace manifold
{
    namespace kernel
    {
        template<>
            size_t
            Get_serialize_size<manifold::iris::LinkData>(const manifold::iris::LinkData* ld);

        template<>
            size_t
            Serialize<manifold::iris::LinkData>(manifold::iris::LinkData* p,unsigned char* buf );

            template<>
            manifold::iris::LinkData*
            Deserialize<manifold::iris::LinkData>(unsigned char* data );
    }
}


#endif // MANIFOLD_IRIS_LINKDATA_H
