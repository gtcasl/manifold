#ifndef MANIFOLD_CROSS_MCP_IRIS_MCP_IRIS_H
#define MANIFOLD_CROSS_MCP_IRIS_MCP_IRIS_H

#include "uarch/networkPacket.h"
#include "iris/interfaces/simulatedLen.h"
#include "iris/interfaces/vnetAssign.h"
#include <vector>
#include <set>

//Object of this class is passed to Iris to help determine simulated length of a packet.
class MCPSimLen : public manifold::iris::SimulatedLen<manifold::uarch::NetworkPacket> {
public:
    MCPSimLen(int cacheLineSize, int coh_msg_type, int mem_msg_type, int credit_msg_type, std::vector<int>& mc_nodes);

    int get_simulated_len(manifold::uarch::NetworkPacket* pkt);
private:
    std::set<int> m_mc_nodes;

    const int m_cacheLineSize;
    const int m_COH; //coherence message type
    const int m_MEM; //memory message type
    const int m_CREDIT; //credit message type
};




//Object of this class is passed to Iris to help determine virtual network of a packet.
class MCPVnet : public manifold::iris::VnetAssign<manifold::uarch::NetworkPacket> {
public:
    MCPVnet(int coh_msg_type, int mem_msg_type, int credit_msg_type, std::vector<int>& mc_nodes);

    int get_virtual_net(manifold::uarch::NetworkPacket* pkt);

private:
    std::set<int> m_mc_nodes;

    const int m_COH; //coherence message type
    const int m_MEM; //memory message type
    const int m_CREDIT; //credit message type
};


#endif // MANIFOLD_CROSS_MCP_IRIS_MCP_IRIS_H
