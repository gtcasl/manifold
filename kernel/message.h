#ifndef MANIFOLD_KENEL_MESSAGE_H
#define MANIFOLD_KENEL_MESSAGE_H

#include "common-defs.h"

namespace manifold {
namespace kernel {

//const int MAX_DATA_SIZE = 16384;

struct Message_s {
    enum { M_UINT32, M_UINT64, M_SERIAL,
           M_PROTO1  //private message used by synchronization protocols; the message body is a 32-bit int
         };

    unsigned type;
    int compIndex;  //component index
    int inputIndex; //index of input receiving the message
    int isTick;     //a boolean indicating whether sendTime/recvTime is tick or seconds
    Ticks_t sendTick;
    Ticks_t recvTick;
    Time_t sendTime;
    Time_t recvTime;
    uint32_t uint32_data;
    uint64_t uint64_data;
    //unsigned char data[MAX_DATA_SIZE];
    unsigned char* data;
    int data_len;

    void print()
    {
      std::cout
        << " type:" << type
        << " compIndex:" << compIndex
        << " inputIndex:" << inputIndex
        << " isTick:" << isTick
        << " sendTick:" << sendTick
        << " recvTick:" << recvTick
        << " sendTime:" << sendTime
        << " recvTime:" << recvTime
        << " uint32_data:" << uint32_data
        << " uint64_data:" << uint64_data
        << " data:" << data
        << " data_len:" << data_len
        << std::endl;
    }
};

/*
 * The message exchanged in the CMB time synchronization algorithm.
 */
typedef struct NullMsg_s
{
  Time_t t;
  #ifdef FORECAST_NULL
  Ticks_t forecast; //forecast of next event from sender
  #endif
  LpId_t src;
  LpId_t dst;
  uint64_t txCnt;
} NullMsg_t;


//const int MAX_MSG_SIZE = sizeof(Message_s);

} //namespace kernel
} //namespace manifold


#endif
