#ifndef MANIFOLD_IRIS_SIMULATEDLEN_H
#define MANIFOLD_IRIS_SIMULATEDLEN_H

#include "uarch/networkPacket.h"

namespace manifold {
namespace iris {

//! This is the base class of a class that would get the simulated length
//! of a NetworkPacket object.
template<typename T>
class SimulatedLen {
public:
    virtual int get_simulated_len(T*) { return sizeof(T);}
};

} //namespace iris
} //namespace manifold

#endif // MANIFOLD_IRIS_SIMULATEDLEN_H
