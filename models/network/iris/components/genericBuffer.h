/*!
 * =====================================================================================
 *
 *       Filename:  genericbuffer.h
 *
 *    Description:  implements a buffer component. Can support multiple virtual
 *    channels. Used by the router and network interface
 *
 *        Version:  1.0
 *        Created:  02/20/2010 11:48:27 PM
 *       Revision:  none
 *       Compiler:  g++
 *
 *         Author:  Mitchelle Rasquinha (), mitchelle.rasquinha@gatech.edu
 *        Company:  Georgia Institute of Technology
 *
 * =====================================================================================
 */

#ifndef  MANIFOLD_IRIS_GENERICBUFFER_H
#define  MANIFOLD_IRIS_GENERICBUFFER_H

#include	"../data_types/flit.h"
#include        "../interfaces/genericHeader.h"
#include	<deque>
#include	<vector>
#include	<assert.h>

namespace manifold {
namespace iris {

class GenericBuffer
{
    public:
	GenericBuffer (unsigned vcs, unsigned size);
        ~GenericBuffer ();                             /* desstructor */
        void push( unsigned ch,  Flit* f );
        Flit* pull(unsigned ch);
        Flit* peek(unsigned ch);
        uint get_occupancy( uint ch ) const;
        bool is_channel_full( uint ch ) const;
        bool is_empty( uint ch ) const;

        std::string toString() const;

#ifdef IRIS_TEST
        std::vector<std::deque<Flit*> >& get_buffers() { return buffers; }
#endif
    protected:

    private:
        std::vector < std::deque<Flit*> > buffers;
        const uint buffer_size; //max size of each buffer

}; /* -----  end of class GenericBuffer  ----- */


} // namespace iris
} // namespace manifold

#endif // MANIFOLD_IRIS_GENERICBUFFER_H
