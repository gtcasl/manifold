// Component implementation for Manifold
// George F. Riley, (and others) Georgia Tech, Spring 2010

#include <vector>
#include <map>

#include "common-defs.h"
#include "manifold.h"
#include "component.h"

using namespace std;


namespace manifold {
namespace kernel {

// Static members
CompId_t                       Component::nextId = 0;
vector<ComponentLpMapping>     Component::AllComponents;
std::map<string, CompId_t> Component::AllNames;

// Static functions
bool Component::IsLocal(CompId_t id)
{ 
    return (Manifold::GetRank() == GetComponentLP(id)); 
}

bool Component::IsLocal(const string& name)
{
    return (Manifold::GetRank() == GetComponentLP(name)); 
}

// This is bogus; find the right spot for this
LpId_t GetRank()
{
  return 0;
}

Component::Component()
{
    m_clk = 0;
}

Component::~Component()
{ // Virtual destructor
}




//void Component::AddInputLink(LinkBase* l)
//{
//  inLinks.push_back(l);
//}

// Get the component, returning base class
Component* Component::GetComponent(CompId_t id)
{
  if ( id >= (signed int)AllComponents.size() || id < 0 ) return 0;	  
  return AllComponents[id].component;
}

// Get the component, returning base class
Component* Component::GetComponent(const string& name)
{
  NameMap::iterator iter = AllNames.find(name);  
  if ( name == "None" || iter == AllNames.end() ) return 0;	  
  return AllComponents[AllNames[name]].component;
}

LpId_t Component::GetComponentLP(CompId_t id)
{
  if ( id >= (signed int)AllComponents.size() || id < 0 ) return -1;	  
  return AllComponents[id].lp;
}

LpId_t Component::GetComponentLP(const string& name)
{
  NameMap::iterator iter = AllNames.begin();
  iter = AllNames.find(name);
  if ( name == "None" || iter == AllNames.end() ) return -1;	  
  return AllComponents[AllNames[name]].lp;
}

#ifndef NO_MPI
void Component::Recv_remote(int inputIndex, Ticks_t sendTick, Ticks_t recvTick,
                   Time_t sendTime, Time_t recvTime, unsigned char* data, int len)
{
    LinkInputBase* input = inLinks[inputIndex];
    assert(input != 0);

    input->Recv(recvTick, recvTime, data, len);

    //remote_input_notify(recvTick, input->get_data(), inputIndex);

    //use data directly is not correct if the data type contains pointers
    //however the implementation of input->get_data() is not complete yet
    //until then, we have no choice.
    remote_input_notify(recvTick, (void*)data, inputIndex);
}


void Component::add_border_port(int srcPort, int dstLP, Clock* clk)
{
    // First insure the vector is large enough, add nulls if not
    while ((int)border_ports.size() <= srcPort)
	border_ports.push_back(0);

    if(border_ports[srcPort] != 0) { //already created
	cerr << "WARNING: output port connects to multiple components! this is not currently supported!\n";
	return;
    }

    border_ports[srcPort] = new BorderPort(srcPort, GetComponentLP(myId), dstLP, clk);
}


#ifdef FORECAST_NULL
void BorderPort :: update_output_tick(Ticks_t tick)
{
    Manifold::get_scheduler()->updateOutputTick(m_SRCLP, m_DSTLP, tick, m_clk);
}
#endif


#endif //#ifndef NO_MPI

} //namespace kernel
} //namespace manifold
