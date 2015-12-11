#ifndef MANIFOLD_IRIS_VNETASSIGN_H
#define MANIFOLD_IRIS_VNETASSIGN_H

#include "uarch/networkPacket.h"

namespace manifold {
namespace iris {

//! This is the base class of a class that would assign virtual networks to packets
//! based on packet contents.
template<typename T>
class VnetAssign {
public:
    virtual int get_virtual_net(T*) { return 0; }
};

} //namespace iris
} //namespace manifold

#endif // MANIFOLD_IRIS_VNETASSIGN_H
