// Contains the component implementations of the Component Object
// We separate this because component needs manifold, and manifold
// needs component.  By separating the definition (component.h) from
// the implementation (this file), and the same for manifold.h
// and manifold-impl.h, we can make this work.
//  
// George F. Riley, (and others) Georgia Tech, Fall 2010
//
#ifndef MANIFOLD_KERNEL_COMPONENT_H
#define MANIFOLD_KERNEL_COMPONENT_H

#include "manifold-decl.h"
#include "component-decl.h"
#include "link.h"
#include <assert.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <typeinfo>

namespace manifold {
namespace kernel {

// Implementations for the member functions

template <typename T>
Link<T>* Component::AddOutputLink(int sourceIndex) throw (LinkTypeMismatchException)
{
  // First insure the outLinks vector is large enough, add nulls if not
  while ((int)outLinks.size() <= sourceIndex) outLinks.push_back(0);
  //Link<T>* complexLink = outLinks[sourceIndex];
  Link<T>*  complexLink = nil;
  if (outLinks[sourceIndex] == nil)
    {
      complexLink = new Link<T>;
    }
  else
    {
      complexLink = dynamic_cast<Link<T>*>(outLinks[sourceIndex]);
      if(complexLink == 0)
        throw LinkTypeMismatchException();
    }
  outLinks[sourceIndex] = complexLink;
  return complexLink;
}

#ifndef NO_MPI
//! Add an input for the component. An input is only created when the corresponding
//! output component is in a different LP. We store the component and the handler
//! so that we can handle input data.
//! @arg inputIndex The index of the input.
//! @arg handler A two-parameter member function that handles input data.
//! @arg obj The object (component).
//! @arg c The associated clock.
//! @arg isTimed Flag set to true if latency is timed.
//! @arg isHalf  Flag set to true if ticks latency is half ticks.
template<typename T, typename T2>
void Component::AddInput(int inputIndex, void (T::*handler)(int, T2), T* obj, Clock* c,
              bool isTimed=false, bool isHalf=false)
{
  // First insure the vector is large enough, add nulls if not
  while ((int)inLinks.size() <= inputIndex) inLinks.push_back(0);

  assert(inLinks[inputIndex] == 0); //input for the index cannot have already been created.
  inLinks[inputIndex] = new LinkInput<T2, T>(handler, obj, inputIndex, c, isTimed, isHalf);
}
#endif


// Implementations for the Create functions
template <typename T> CompId_t Component::Create(LpId_t lp, CompName name )
{
  if (lp == Manifold::GetRank())
    {
      T* n = new T;
      AllComponents.push_back(ComponentLpMapping(n, lp));
      AllNames[name.get_name()] = nextId;
      n->setComponentId(nextId++);
      n->setComponentName(name);
      return n->getComponentId();
    }
  else
    {  // Not on this LP
      AllComponents.push_back(ComponentLpMapping(0, lp));
      AllNames[name.get_name()] = nextId;
      return nextId++;
    }
}

template <typename T, typename T1>
  CompId_t Component::Create(LpId_t lp, T1& t1, CompName name )
{
  if (lp == Manifold::GetRank())
    {
      T* n = new T(t1);
      AllComponents.push_back(ComponentLpMapping(n, lp));
      AllNames[name.get_name()] = nextId;
      n->setComponentId(nextId++);
      n->setComponentName(name);
      return n->getComponentId();
    }
  else
    {  // Not on this LP
      AllComponents.push_back(ComponentLpMapping(0, lp));
      AllNames[name.get_name()] = nextId;
      return nextId++;
    }
}

template <typename T, typename T1>
  CompId_t Component::Create(LpId_t lp, const T1& t1, CompName name )
{
  if (lp == Manifold::GetRank())
    {
      T* n = new T(t1);
      AllComponents.push_back(ComponentLpMapping(n, lp));
      AllNames[name.get_name()] = nextId;
      n->setComponentId(nextId++);
      n->setComponentName(name);
      return n->getComponentId();
    }
  else
    {  // Not on this LP
      AllComponents.push_back(ComponentLpMapping(0, lp));
      AllNames[name.get_name()] = nextId;
      return nextId++;
    }
}

template <typename T, typename T1, typename T2>
  CompId_t Component::Create(LpId_t lp, const T1& t1, const T2& t2, CompName name )
{
  if (lp == Manifold::GetRank())
    {
      T* n = new T(t1, t2);
      AllComponents.push_back(ComponentLpMapping(n, lp));
      AllNames[name.get_name()] = nextId;
      n->setComponentId(nextId++);
      n->setComponentName(name);
      return n->getComponentId();
    }
  else
    {  // Not on this LP
      AllComponents.push_back(ComponentLpMapping(0, lp));
      AllNames[name.get_name()] = nextId;
      return nextId++;
    }
}

template <typename T, typename T1, typename T2>
  CompId_t Component::Create(LpId_t lp, T1& t1, T2& t2, CompName name )
{
  if (lp == Manifold::GetRank())
    {
      T* n = new T(t1, t2);
      AllComponents.push_back(ComponentLpMapping(n, lp));
      AllNames[name.get_name()] = nextId;
      n->setComponentId(nextId++);
      n->setComponentName(name);
      return n->getComponentId();
    }
  else
    {  // Not on this LP
      AllComponents.push_back(ComponentLpMapping(0, lp));
      AllNames[name.get_name()] = nextId;
      return nextId++;
    }
}



template <typename T, typename T1, typename T2, typename T3>
  CompId_t Component::Create(LpId_t lp, const T1& t1, const T2& t2, const T3& t3, CompName name )
{
  if (lp == Manifold::GetRank())
    {
      T* n = new T(t1, t2, t3);
      AllComponents.push_back(ComponentLpMapping(n, lp));
      AllNames[name.get_name()] = nextId;
      n->setComponentId(nextId++);
      n->setComponentName(name);
      return n->getComponentId();
    }
  else
    {  // Not on this LP
      AllComponents.push_back(ComponentLpMapping(0, lp));
      AllNames[name.get_name()] = nextId;
      return nextId++;
    }
}

template <typename T, typename T1, typename T2, typename T3>
  CompId_t Component::Create(LpId_t lp, T1& t1, T2& t2, T3& t3, CompName name )
{
  if (lp == Manifold::GetRank())
    {
      T* n = new T(t1, t2, t3);
      AllComponents.push_back(ComponentLpMapping(n, lp));
      AllNames[name.get_name()] = nextId;
      n->setComponentId(nextId++);
      n->setComponentName(name);
      return n->getComponentId();
    }
  else
    {  // Not on this LP
      AllComponents.push_back(ComponentLpMapping(0, lp));
      AllNames[name.get_name()] = nextId;
      return nextId++;
    }
}

template <typename T, typename T1, typename T2, typename T3, typename T4>
  CompId_t Component::Create(LpId_t lp, const T1& t1, const T2& t2, const T3& t3, const T4& t4, CompName name )
{
  if (lp == Manifold::GetRank())
    {
      T* n = new T(t1, t2, t3, t4);
      AllComponents.push_back(ComponentLpMapping(n, lp));
      AllNames[name.get_name()] = nextId;
      n->setComponentId(nextId++);
      n->setComponentName(name);
      return n->getComponentId();
    }
  else
    {  // Not on this LP
      AllComponents.push_back(ComponentLpMapping(0, lp));
      AllNames[name.get_name()] = nextId;
      return nextId++;
    }
}

template <typename T, typename T1, typename T2, typename T3, typename T4>
  CompId_t Component::Create(LpId_t lp, T1& t1, T2& t2, T3& t3, T4& t4, CompName name )
{
  if (lp == Manifold::GetRank())
    {
      T* n = new T(t1, t2, t3, t4);
      AllComponents.push_back(ComponentLpMapping(n, lp));
      AllNames[name.get_name()] = nextId;
      n->setComponentId(nextId++);
      n->setComponentName(name);
      return n->getComponentId();
    }
  else
    {  // Not on this LP
      AllComponents.push_back(ComponentLpMapping(0, lp));
      AllNames[name.get_name()] = nextId;
      return nextId++;
    }
}

template <typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
  CompId_t Component::Create(LpId_t lp, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, CompName name )
{
  if (lp == Manifold::GetRank())
    {
      T* n = new T(t1, t2, t3, t4, t5);
      AllComponents.push_back(ComponentLpMapping(n, lp));
      AllNames[name.get_name()] = nextId;
      n->setComponentId(nextId++);
      n->setComponentName(name);
      return n->getComponentId();
    }
  else
    {  // Not on this LP
      AllComponents.push_back(ComponentLpMapping(0, lp));
      AllNames[name.get_name()] = nextId;
      return nextId++;
    }
}

template <typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
  CompId_t Component::Create(LpId_t lp, T1& t1, T2& t2, T3& t3, T4& t4, T5& t5, CompName name )
{
  if (lp == Manifold::GetRank())
    {
      T* n = new T(t1, t2, t3, t4, t5);
      AllComponents.push_back(ComponentLpMapping(n, lp));
      AllNames[name.get_name()] = nextId;
      n->setComponentId(nextId++);
      n->setComponentName(name);
      return n->getComponentId();
    }
  else
    {  // Not on this LP
      AllComponents.push_back(ComponentLpMapping(0, lp));
      AllNames[name.get_name()] = nextId;
      return nextId++;
    }
}

// Get the component with a dynamic cast
template <typename T>
T* Component::GetComponent(CompId_t id)  // Get component by id
{
  if (AllComponents[id].component == 0) return 0; // Does not exist on this lp
  return dynamic_cast<T*>(AllComponents[id].component);
}

// Get the component with a dynamic cast
template <typename T>
T* Component::GetComponent(const std::string& name)  // Get component by name
{
  NameMap::iterator iter = AllNames.find(name);
  if ( name == "None" || iter == AllNames.end() ) return 0;	
  if ( AllComponents[AllNames[name]].component == 0) return 0; // Does not exist on this lp
  return dynamic_cast<T*>(AllComponents[AllNames[name]].component);
}

template <typename T>
void Component::Send(int whichLink, T t)
{
    try {
        outLinks[whichLink]->Send(t);
    }
    catch (BadSendTypeException) {
        std::cerr << "Component sending wrong data type " << typeid(t).name() << endl;
	exit(1);
    }
}

template <typename T>
void Component::SendTick(int whichLink, T t, Ticks_t delay)
{
    try {
        outLinks[whichLink]->SendTick(t, delay);
    }
    catch (BadSendTypeException) {
        std::cerr << "Component sending wrong data type " << typeid(t).name() << endl;
	exit(1);
    }
}

template <typename T>
void Component::SendTime(int whichLink, T t, Time_t delay)
{
    try {
        outLinks[whichLink]->SendTime(t, delay);
    }
    catch (BadSendTypeException) {
        std::cerr << "Component sending wrong data type " << typeid(t).name() << endl;
	exit(1);
    }
}


#ifndef NO_MPI
//! This is called by scheduler to handle incoming messages from another LP.
template <typename T>
void Component::Recv_remote(int inputIndex, Ticks_t sendTick, Ticks_t recvTick,
                            Time_t sendTime, Time_t recvTime, T data)
{
    LinkInputBase* input = inLinks[inputIndex];
    assert(input != 0);

    input->Recv(recvTick, recvTime, data);
}
#endif //#ifndef NO_MPI

} //namespace kernel
} //namespace manifold


#endif

