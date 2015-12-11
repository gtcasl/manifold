// George F. Riley, (and others) Georgia Tech, Fall 2010

#include "link.h"

#ifndef NO_MPI

#include "messenger.h"

namespace manifold {
namespace kernel {

//! Template specialization for uint32_t.
template<>
void LinkOutputRemote<uint32_t> :: ScheduleRxEvent()
{
    if(timed) {
	TheMessenger.send_uint32_msg(dest, compIndex, inputIndex, Manifold::Now(),
	                 Manifold::Now() + timeLatency, data);
    }
    else if(half) {
	TheMessenger.send_uint32_msg(dest, compIndex, inputIndex, Manifold::NowHalfTicks(*(this->clock)),
	                 Manifold::NowHalfTicks(*(this->clock)) + latency, data);
    }
    else {
	TheMessenger.send_uint32_msg(dest, compIndex, inputIndex, Manifold::NowTicks(*(this->clock)),
	                 Manifold::NowTicks(*(this->clock)) + latency, data);
    }
}


//! Template specialization for uint64_t.
template<>
void LinkOutputRemote<uint64_t> :: ScheduleRxEvent()
{
    if(timed) {
	TheMessenger.send_uint64_msg(dest, compIndex, inputIndex, Manifold::Now(),
	                 Manifold::Now() + timeLatency, data);
    }
    else if(half) {
	TheMessenger.send_uint64_msg(dest, compIndex, inputIndex, Manifold::NowHalfTicks(*(this->clock)),
	                 Manifold::NowHalfTicks(*(this->clock)) + latency, data);
    }
    else {
	TheMessenger.send_uint64_msg(dest, compIndex, inputIndex, Manifold::NowTicks(*(this->clock)),
	                 Manifold::NowTicks(*(this->clock)) + latency, data);
    }
}


template<>
void LinkInputBaseT<uint32_t> ::
Recv(Ticks_t tick, Time_t time, unsigned char* data, int len)
{
  assert(0); //This function should never be called.
}

template<>
void LinkInputBaseT<uint64_t> ::
Recv(Ticks_t tick, Time_t time, unsigned char* data, int len)
{
  assert(0); //This function should never be called.
}



//Code in this function was originally in LinkOutputRemote<T>::ScheduleRxEvent() in link.h.
//However, since all clients of the kernel include link.h, they will have to add configuration
//to configure the flag KERNEL_ANY_DATA_SIZE. By creating the following function, the flag
//doesn't appear in link.h, and the clients don't need to worry about the flag.
void Ensure_send_buf_size(int size)
{
#ifdef KERNEL_ANY_DATA_SIZE
      assert(TheMessenger.get_send_buf_data_addr());
  if(size > TheMessenger.get_send_data_buffer_size()) {
      TheMessenger.resize_send_buffer(size);
  }
#else
  assert(size <= TheMessenger.get_send_data_buffer_size());
#endif
}


void Send_serial_msg(int dest, int compIndex, int inputIndex, Ticks_t sendTick, Ticks_t recvTick, int len)
{
    TheMessenger.send_serial_msg(dest, compIndex, inputIndex, sendTick, recvTick, len);
}

void Send_serial_msg(int dest, int compIndex, int inputIndex, double sendTime, double recvTime, int len)
{
    TheMessenger.send_serial_msg(dest, compIndex, inputIndex, sendTime, recvTime, len);
}

unsigned char* Get_send_buf_data_addr()
{
    return TheMessenger.get_send_buf_data_addr();
}





} //namespace kernel
} //namespace manifold

#endif //#ifndef NO_MPI
