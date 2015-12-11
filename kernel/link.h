// Implementation of Link objects for Manifold
// George F. Riley, (and others) Georgia Tech, Fall 2010

#ifndef MANIFOLD_KERNEL_LINK_H
#define MANIFOLD_KERNEL_LINK_H

#include <iostream>
#include <stdint.h>
#include <vector>
#include <assert.h>
#include <typeinfo>

#include "common-defs.h"
#include "link-decl.h"
#include "manifold-decl.h"
#include "serialize.h"


namespace manifold {
namespace kernel {

template <typename T1, typename OBJ>
  void LinkOutput<T1, OBJ>::ScheduleRxEvent()
{
  if (LinkOutputBase<T1>::timed)
    { // Timed link, use the timed schedule
      Manifold::ScheduleTime(LinkOutputBase<T1>::timeLatency, handler, obj,
                         LinkOutputBase<T1>::inputIndex,
                         LinkOutputBase<T1>::data);
    }
  else if (LinkOutputBase<T1>::half)
    { // Schedule on half ticks
      Manifold::ScheduleClockHalf(
                                  LinkOutputBase<T1>::latency,
                                  *LinkOutputBase<T1>::clock,
                                  handler, obj, 
                                  LinkOutputBase<T1>::inputIndex,
                                  LinkOutputBase<T1>::data);
    }
  else
    { // Schedule on ticks
      Manifold::ScheduleClock(
                              LinkOutputBase<T1>::latency,
                              *LinkOutputBase<T1>::clock,
                              handler, obj, 
                              LinkOutputBase<T1>::inputIndex,
                              LinkOutputBase<T1>::data);
    }
}


#ifndef NO_MPI
template<typename T>
LinkOutputRemote<T>::LinkOutputRemote(Link<T>* link, int compIdx,
            int inputIndex, Clock* c, Ticks_t latency, Time_t d, 
            bool isT, bool isH)
   :   LinkOutputBase<T>(latency, inputIndex, c, d, isT, isH),
    compIndex(compIdx), pLink(link)
{
  dest = Component :: GetComponentLP(compIdx);
}

/*
//The following 2 templated Serialize() functions are created to solve a compiler
//problem occurring when the data sent between 2 components is a pointer type,
//such as MyType*. In this case, calling T :: Serialize() directly in ScheduleRxEvent()
//below would cause a compile problem.
template <typename T>
int Serialize(T data, unsigned char** d)
{
    return T :: Serialize(data, d);
}

template <typename T>
int Serialize(T* data, unsigned char** d)
{
    return T :: Serialize(data, d);
}
*/


template <typename T>
size_t get_serialize_size_internal(const T& data)
{
    return Get_serialize_size(data);
}

//In serialize.h we only provide Get_serialize_size(const T*), to prevent
//Get_serialize_size(const T&) from being mistakenly called when data is
//a non-const pointer, we created the following function.
template <typename T>
size_t get_serialize_size_internal(T* data)
{
    return Get_serialize_size((const T*)data); //calls the "const T*" version
}

template <typename T>
size_t get_serialize_size_internal(const T* data)
{
    return Get_serialize_size(data);
}


//The following, which are implemented in link.cc, make it possible for this
//file to not include messenger.h.
void Ensure_send_buf_size(int size);
unsigned char* Get_send_buf_data_addr();
void Send_serial_msg(int dest, int compIndex, int inputIndex, Ticks_t sendTick, Ticks_t recvTick, int len);
void Send_serial_msg(int dest, int compIndex, int inputIndex, double sendTime, double recvTime, int len);


template <typename T>
void LinkOutputRemote<T>::ScheduleRxEvent()
{
  //unsigned char d[MAX_DATA_SIZE];
  //Cannot call T :: Serialize(this->data, d), because if T is a pointer type such as
  //MyType*, then compiler would complain Serialize() is not a member of MyType*.
  //Therefore, we created 2 template functions above to solve this problem.

  int size = get_serialize_size_internal(this->data);

  Ensure_send_buf_size(size);

/*
  int len = Serialize(this->data, TheMessenger.get_send_buf_data_addr());
  if(this->timed) {
    TheMessenger.send_serial_msg(dest, compIndex, this->inputIndex, Manifold::Now(),
	                 Manifold::Now() + this->timeLatency, len);
  }
  else if(this->half) {
    TheMessenger.send_serial_msg(dest, compIndex, this->inputIndex, Manifold::NowHalfTicks(*(this->clock)),
	                 Manifold::NowHalfTicks(*(this->clock)) + this->latency, len);
  }
  else {
    TheMessenger.send_serial_msg(dest, compIndex, this->inputIndex, Manifold::NowTicks(*(this->clock)),
	                 Manifold::NowTicks(*(this->clock)) + this->latency, len);
  }
*/
  //Remove all references to the messenger
  int len = Serialize(this->data, Get_send_buf_data_addr());
  if(this->timed) {
    Send_serial_msg(dest, compIndex, this->inputIndex, Manifold::Now(),
	                 Manifold::Now() + this->timeLatency, len);
  }
  else if(this->half) {
    Send_serial_msg(dest, compIndex, this->inputIndex, Manifold::NowHalfTicks(*(this->clock)),
	                 Manifold::NowHalfTicks(*(this->clock)) + this->latency, len);
  }
  else {
    Send_serial_msg(dest, compIndex, this->inputIndex, Manifold::NowTicks(*(this->clock)),
	                 Manifold::NowTicks(*(this->clock)) + this->latency, len);
  }
}


//! specialization for uint32_t
template <>
void LinkOutputRemote<uint32_t>::ScheduleRxEvent();

//! specialization for uint64_t
template <>
void LinkOutputRemote<uint64_t>::ScheduleRxEvent();

//####### must provide specialization for char, unsigned char, int, etc.


template<typename T>
void LinkInputBase :: Recv(Ticks_t tick, Time_t time, T& data)
{
  LinkInputBaseT<T>* baseT = (LinkInputBaseT<T>*)(this); // downcast!!
  baseT->set_data(data);
  baseT->ScheduleRxEvent(tick, time);
}

template<>
void LinkInputBaseT<uint32_t> ::
Recv(Ticks_t tick, Time_t time, unsigned char* data, int len);

template<>
void LinkInputBaseT<uint64_t> ::
Recv(Ticks_t tick, Time_t time, unsigned char* data, int len);


template <typename T, typename OBJ>
void LinkInput<T, OBJ>::ScheduleRxEvent(Ticks_t tick, Time_t time)
{
    if(this->timed) {
        assert(time>=Manifold::Now());
        Manifold::ScheduleTime(time - Manifold::Now(), handler, obj, this->inputIndex, this->data);
    }
    else if(this->half) {
        assert(tick>=Manifold::NowHalfTicks(*(this->clock)));
        Manifold::ScheduleClockHalf(tick - Manifold::NowHalfTicks(*(this->clock)), *(this->clock), handler, obj, this->inputIndex, this->data);
    }
    else {
        //if (tick < Manifold::NowTicks(*(this->clock)) || (tick == Manifold::NowTicks(*(this->clock)) && this->clock->nextRising==false)) {
//cerr << "VVVVVVVVVVVVV LP " << Manifold::GetRank() << " tick= " << tick << " clock= " << this->clock->NowTicks() << " rising= " << this->clock->nextRising << endl;
//}
        //if event is for the current tick, then clock must be at the rising edge; otherwise, the event is in the past.
        assert(tick > Manifold::NowTicks(*(this->clock)) || (tick == Manifold::NowTicks(*(this->clock)) && this->clock->nextRising));
        Manifold::ScheduleClock(tick - Manifold::NowTicks(*(this->clock)), *(this->clock), handler, obj, this->inputIndex, this->data);
    }
}
#endif //#ifndef NO_MPI

template<typename T>
void LinkBase::Send(T t) throw (BadSendTypeException)
{
  Link<T>* pLink = dynamic_cast<Link<T>*>(this);
  if (pLink == 0)
    {
      throw BadSendTypeException();
    }
  else
    {
      for (size_t i = 0; i < pLink->outputs.size(); ++i)
        {
          pLink->outputs[i]->Send(t);
        }
    }
}

template<typename T>
void LinkBase::SendTick(T t, Ticks_t delay) throw (BadSendTypeException)
{
  //Link<T>* pLink = dynamic_cast<Link<T>*>(this);
  // Figure this out later. the dynamic_cast should have worked
  Link<T>* pLink = (Link<T>*)(this);
  if (pLink == 0)
    {
      throw BadSendTypeException();
    }
  else
    {
      for (size_t i = 0; i < pLink->outputs.size(); ++i)
        {
          pLink->outputs[i]->SendTick(t, delay);
        }
    }
}

template<typename T>
void LinkBase::SendTime(T t, Time_t delay) throw (BadSendTypeException)
{
  //Link<T>* pLink = dynamic_cast<Link<T>*>(this);
  // Figure this out later. the dynamic_cast should have worked
  Link<T>* pLink = (Link<T>*)(this);
  if (pLink == 0)
  {
      throw BadSendTypeException();
  }
  else
  {
    for (size_t i = 0; i < pLink->outputs.size(); ++i)
    {
      pLink->outputs[i]->SendTime(t, delay);
    }
  }
}


template <typename T>
template <typename O> void Link<T>::AddOutput(void(O::*f)(int, T), 
                                              O* o,
                                              Ticks_t l, int ii,
                                              Clock* c,
                                              Time_t delay,
                                              bool isTimed,
                                              bool isHalf)
{
  LinkOutputBase<T>* lo = 
    new LinkOutput<T, O>(this, f, o, l, ii, c, delay, isTimed, isHalf);
  outputs.push_back(lo);
}

#ifndef NO_MPI
template <typename T>
void Link<T>::AddOutputRemote(CompId_t compIdx, int inputIndex, Clock* c, Ticks_t l,
                              Time_t delay, bool isTimed, bool isHalf)
{
  LinkOutputBase<T>* lo = 
    new LinkOutputRemote<T>(this, compIdx, inputIndex, c, l, delay, isTimed, isHalf);
  outputs.push_back(lo);
}
#endif //#ifndef NO_MPI

} //namespace kernel
} //namespace manifold

#endif
