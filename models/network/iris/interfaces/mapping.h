#ifndef MANIFOLD_IRIS_MAPPING_H
#define MANIFOLD_IRIS_MAPPING_H

namespace manifold {
namespace iris {

//! When a network terminal sends info to another terminal, it uses the terminal
//! level address, but when a network interface sends a packet to another, it
//! uses the network interface IDs. Therefore, there must be a mapping that maps
//! the terminal level address to network interface ID.
//! This is similar to the mapping from IP address to MAC address, except here
//! we use static mapping, not something dynamic like ARP.
class Terminal_to_net_mapping {
public:
    virtual ~Terminal_to_net_mapping() {}
    virtual int terminal_to_net(int aid) const = 0;
};


//! In this mapping, the terminal address is the same as the network interface ID,
//! so the mapping is F(x) = x.
class Simple_terminal_to_net_mapping : public Terminal_to_net_mapping {
public:
    virtual int terminal_to_net(int aid) const;
};


} //namespace iris
} //namespace manifold

#endif

