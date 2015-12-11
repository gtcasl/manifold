#ifndef NO_MPI

#include <iostream>
#include <list>
#include <stdlib.h>
#include <assert.h>

#include "messenger.h"

using namespace std;

namespace manifold {
namespace kernel {

//====================================================================
//====================================================================
Messenger :: Messenger()
{
    m_numSent = 0; //total sent
    m_numReceived = 0; //total recv

    stats_sent_proto1 = 0; //sent Proto1 messages: eg. quantum related messages
    stats_recv_proto1 = 0;
}

//====================================================================
//====================================================================
#ifdef KERNEL_ANY_DATA_SIZE
void Messenger :: init(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &m_nodeId);
    MPI_Comm_size(MPI_COMM_WORLD, &m_nodeSize);

    m_numSent = 0;
    m_numReceived = 0;
    stats_sent_proto1 = 0; //sent Proto1 messages: eg. quantum related messages
    stats_recv_proto1 = 0;

    m_rxcount = new uint64_t[m_nodeSize];
    m_txcount = new uint64_t[m_nodeSize];
    for(int i=0;i<m_nodeSize;i++) {
	m_rxcount[i]=m_txcount[i]=0;
    }


    const int init_data_size = 16384;
    m_recv_buf_size = sizeof(Message_s) + init_data_size;

    m_recv_buf = new unsigned char[m_recv_buf_size];

    //initialize m_header_size: size of the header of serial message.
    //verify MPI_LONG_LONG and MPI_DOUBLE are the same size.
    int sz1, sz2;
    MPI_Type_size(MPI_UNSIGNED_LONG_LONG, &sz1);
    MPI_Type_size(MPI_DOUBLE, &sz2);
    assert(sz1 == sz2);

    m_header_size = 0;
    int sz;
    MPI_Type_size(MPI_UNSIGNED, &sz); //msg_type, compIndex, inputIndex, isTick
    m_header_size += sz*4;
    MPI_Type_size(MPI_UNSIGNED_LONG_LONG, &sz); //sendTick, recvTick; assuming sendTime(MPI_DOUBLE)
                                                //is the same size
    m_header_size += sz*2;
    MPI_Type_size(MPI_INT, &sz); //len
    m_header_size += sz;

    m_send_buf_size = m_header_size + init_data_size;
    m_send_buf = new unsigned char[m_send_buf_size];
}

#else
void Messenger :: init(int argc, char** argv, int max_data_size)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &m_nodeId);
    MPI_Comm_size(MPI_COMM_WORLD, &m_nodeSize);

    m_numSent = 0;
    m_numReceived = 0;
    stats_sent_proto1 = 0; //sent Proto1 messages: eg. quantum related messages
    stats_recv_proto1 = 0;

    m_rxcount = new uint64_t[m_nodeSize];
    m_txcount = new uint64_t[m_nodeSize];
    for(int i=0;i<m_nodeSize;i++) {
	m_rxcount[i]=m_txcount[i]=0;
    }

    m_max_data_size = max_data_size;

    m_recv_buf_size = sizeof(Message_s) + max_data_size;
    m_recv_buf = new unsigned char[m_recv_buf_size];

    //initialize m_header_size: size of the header of serial message.
    //verify MPI_LONG_LONG and MPI_DOUBLE are the same size.
    int sz1, sz2;
    MPI_Type_size(MPI_UNSIGNED_LONG_LONG, &sz1);
    MPI_Type_size(MPI_DOUBLE, &sz2);
    assert(sz1 == sz2);

    m_header_size = 0;
    int sz;
    MPI_Type_size(MPI_UNSIGNED, &sz); //msg_type, compIndex, inputIndex, isTick
    m_header_size += sz*4;
    MPI_Type_size(MPI_UNSIGNED_LONG_LONG, &sz); //sendTick, recvTick; assuming sendTime(MPI_DOUBLE)
                                                //is the same size
    m_header_size += sz*2;
    MPI_Type_size(MPI_INT, &sz); //len
    m_header_size += sz;

    m_send_buf_size = m_header_size + max_data_size;
    m_send_buf = new unsigned char[m_send_buf_size];
}
#endif



//====================================================================
//====================================================================
void Messenger :: finalize()
{
    MPI_Finalize();
}

//====================================================================
//====================================================================
void Messenger :: barrier()
{
    MPI_Barrier(MPI_COMM_WORLD);
}

//====================================================================
//! Perform an AllGater.
//! @param item   address of the data to send
//! @param itemSize    size of the data in terms of bytes
//! @param recvbuf    this is where the gathered data are stored
//====================================================================
void Messenger :: allGather(char* item, int itemSize, char* recvbuf)
{
    MPI_Allgather(item, itemSize, MPI_BYTE, recvbuf,
                  itemSize, MPI_BYTE, MPI_COMM_WORLD);
}


//====================================================================
//====================================================================
void Messenger :: send_uint32_msg(int dest, int compIndex, int inputIndex,
				  Ticks_t sendTick, Ticks_t recvTick, uint32_t data)
{
    const int Buf_size = sizeof(Message_s);

    unsigned char buf[Buf_size];

    unsigned msg_type = Message_s :: M_UINT32;
    int isTick = 1;  //time unit for sendTime/recvTime is tick

    int position = 0;

    MPI_Pack(&msg_type, 1, MPI_UNSIGNED, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&compIndex, 1, MPI_INT, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&inputIndex, 1, MPI_INT, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&isTick, 1, MPI_INT, buf, Buf_size, &position, MPI_COMM_WORLD);
    //Ticks_t == uint64_t
    MPI_Pack(&sendTick, 1, MPI_UNSIGNED_LONG_LONG, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&recvTick, 1, MPI_UNSIGNED_LONG_LONG, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&data, 1, MPI_UNSIGNED, buf, Buf_size, &position, MPI_COMM_WORLD);

    send_message(dest, buf, position);

    #ifdef DBG_MSG
    #endif

}

//====================================================================
//====================================================================
void Messenger :: send_uint32_msg(int dest, int compIndex, int inputIndex,
                                  double sendTime, double recvTime, uint32_t data)
{
    const int Buf_size = sizeof(Message_s);
    unsigned char buf[Buf_size];  

    unsigned msg_type = Message_s :: M_UINT32;
    int isTick = 0;  //time unit for sendTime/recvTime is not tick

    int position = 0;

    MPI_Pack(&msg_type, 1, MPI_UNSIGNED, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&compIndex, 1, MPI_INT, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&inputIndex, 1, MPI_INT, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&isTick, 1, MPI_INT, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&sendTime, 1, MPI_DOUBLE, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&recvTime, 1, MPI_DOUBLE, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&data, 1, MPI_UNSIGNED, buf, Buf_size, &position, MPI_COMM_WORLD);

    send_message(dest, buf, position);

    #ifdef DBG_MSG
    #endif

}


//====================================================================
//====================================================================
void Messenger :: send_uint64_msg(int dest, int compIndex, int inputIndex,
				  Ticks_t sendTick, Ticks_t recvTick, uint64_t data)
{
    const int Buf_size = sizeof(Message_s);
    unsigned char buf[Buf_size];  

    unsigned msg_type = Message_s :: M_UINT64;
    int isTick = 1;  //time unit for sendTime/recvTime is tick

    int position = 0;

    MPI_Pack(&msg_type, 1, MPI_UNSIGNED, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&compIndex, 1, MPI_INT, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&inputIndex, 1, MPI_INT, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&isTick, 1, MPI_INT, buf, Buf_size, &position, MPI_COMM_WORLD);
    //Ticks_t == uint64_t
    MPI_Pack(&sendTick, 1, MPI_UNSIGNED_LONG_LONG, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&recvTick, 1, MPI_UNSIGNED_LONG_LONG, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&data, 1, MPI_UNSIGNED_LONG_LONG, buf, Buf_size, &position, MPI_COMM_WORLD);

    send_message(dest, buf, position);

    #ifdef DBG_MSG
    #endif

}

//====================================================================
//====================================================================
void Messenger :: send_uint64_msg(int dest, int compIndex, int inputIndex,
                                  double sendTime, double recvTime, uint64_t data)
{
    const int Buf_size = sizeof(Message_s);
    unsigned char buf[Buf_size];  

    unsigned msg_type = Message_s :: M_UINT64;
    int isTick = 0;  //time unit for sendTime/recvTime is not tick

    int position = 0;

    MPI_Pack(&msg_type, 1, MPI_UNSIGNED, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&compIndex, 1, MPI_INT, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&inputIndex, 1, MPI_INT, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&isTick, 1, MPI_INT, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&sendTime, 1, MPI_DOUBLE, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&recvTime, 1, MPI_DOUBLE, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&data, 1, MPI_UNSIGNED_LONG_LONG, buf, Buf_size, &position, MPI_COMM_WORLD);

    send_message(dest, buf, position);

    #ifdef DBG_MSG
    #endif

}


//====================================================================
//====================================================================
//! When this function is called, data is already in the buffer.
//!
void Messenger :: send_serial_msg(int dest, int compIndex, int inputIndex,
				  Ticks_t sendTick, Ticks_t recvTick, int len)
{

    unsigned msg_type = Message_s :: M_SERIAL;
    int isTick = 1;  //time unit for sendTime/recvTime is tick

    int position = 0;

    MPI_Pack(&msg_type, 1, MPI_UNSIGNED, m_send_buf, m_send_buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&compIndex, 1, MPI_INT, m_send_buf, m_send_buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&inputIndex, 1, MPI_INT, m_send_buf, m_send_buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&isTick, 1, MPI_INT, m_send_buf, m_send_buf_size, &position, MPI_COMM_WORLD);
    //Ticks_t == uint64_t
    MPI_Pack(&sendTick, 1, MPI_UNSIGNED_LONG_LONG, m_send_buf, m_send_buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&recvTick, 1, MPI_UNSIGNED_LONG_LONG, m_send_buf, m_send_buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&len, 1, MPI_INT, m_send_buf, m_send_buf_size, &position, MPI_COMM_WORLD);
    //MPI_Pack(data, len, MPI_UNSIGNED_CHAR, m_send_buf, m_send_buf_size, &position, MPI_COMM_WORLD);
    position += len;

    send_message(dest, m_send_buf, position);

    #ifdef DBG_MSG
    #endif

}


//====================================================================
//====================================================================
//! When this function is called, data is already in the buffer.
//!
void Messenger :: send_serial_msg(int dest, int compIndex, int inputIndex,
                                  double sendTime, double recvTime, int len)
{
    unsigned msg_type = Message_s :: M_SERIAL;
    int isTick = 0;  //time unit for sendTime/recvTime is not tick

    int position = 0;

    MPI_Pack(&msg_type, 1, MPI_UNSIGNED, m_send_buf, m_send_buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&compIndex, 1, MPI_INT, m_send_buf, m_send_buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&inputIndex, 1, MPI_INT, m_send_buf, m_send_buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&isTick, 1, MPI_INT, m_send_buf, m_send_buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&sendTime, 1, MPI_DOUBLE, m_send_buf, m_send_buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&recvTime, 1, MPI_DOUBLE, m_send_buf, m_send_buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&len, 1, MPI_INT, m_send_buf, m_send_buf_size, &position, MPI_COMM_WORLD);
    //MPI_Pack(data, len, MPI_UNSIGNED_CHAR, m_send_buf, m_send_buf_size, &position, MPI_COMM_WORLD);
    position += len;

    send_message(dest, m_send_buf, position);

    #ifdef DBG_MSG
    #endif

}


//====================================================================
//====================================================================
void Messenger :: send_proto1_msg(int dest, int data)
{
    const int Buf_size = sizeof(Message_s);

    unsigned char buf[Buf_size];

    unsigned msg_type = Message_s :: M_PROTO1;

    int position = 0;

    MPI_Pack(&msg_type, 1, MPI_UNSIGNED, buf, Buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&data, 1, MPI_UNSIGNED, buf, Buf_size, &position, MPI_COMM_WORLD);

    send_message(dest, buf, position);

    stats_sent_proto1++;

    #ifdef DBG_MSG
    #endif

}


//====================================================================
//====================================================================
void Messenger :: broadcast_proto1(int data, int root)
{
    for(int i=0; i<m_nodeSize; i++) {
        if(i != root)
	    send_proto1_msg(i, data);
    }
}


//====================================================================
//====================================================================
void Messenger :: send_message(int dest, unsigned char* buf, int position)
{
    if(MPI_Send(buf, position, MPI_PACKED, dest, TAG_EVENT, MPI_COMM_WORLD) !=
                                                    MPI_SUCCESS) {
        cerr << "send_message failed!" << endl;
        exit(-1);
    }

    m_txcount[dest]++;
    m_numSent++;
}




//====================================================================
//====================================================================
Message_s& Messenger :: irecv_message(int* received)
{
#ifdef KERNEL_ANY_DATA_SIZE
    *received = 0;

    MPI_Status status;
    int flag;
    MPI_Iprobe(MPI_ANY_SOURCE, TAG_EVENT, MPI_COMM_WORLD, &flag, &status);
    if(flag != 0) {
        *received = 1;

        int size;
	MPI_Get_count(&status, MPI_CHAR, &size);
	if(size > m_recv_buf_size) { // msg size > receiving buffer size
	    m_recv_buf_size = size;
	    delete[] m_recv_buf;
	    m_recv_buf = new unsigned char[m_recv_buf_size];
	}
	MPI_Recv(m_recv_buf, m_recv_buf_size, MPI_PACKED, status.MPI_SOURCE, TAG_EVENT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	unpack_message(m_recv_buf);
	m_rxcount[status.MPI_SOURCE]++;
	m_numReceived++;
    }

    return m_msg;

#else

    *received = 0;

    MPI_Status status;
    int flag;
    MPI_Iprobe(MPI_ANY_SOURCE, TAG_EVENT, MPI_COMM_WORLD, &flag, &status);
    if(flag != 0) {
        *received = 1;

        int size;
	MPI_Get_count(&status, MPI_CHAR, &size);
	assert(size <= m_recv_buf_size);

	MPI_Recv(m_recv_buf, m_recv_buf_size, MPI_PACKED, status.MPI_SOURCE, TAG_EVENT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	unpack_message(m_recv_buf);
	m_rxcount[status.MPI_SOURCE]++;
	m_numReceived++;
    }

    return m_msg;


#endif // KERNEL_ANY_DATA_SIZE
}


//====================================================================
//====================================================================
Message_s& Messenger :: unpack_message(unsigned char* buf)
{
    int position;
    //char buf[MAX_MSG_SIZE];
    //MPI_Status status;
    //message_s msg;

    //MPI_Recv(buf, MAX_MSG_SIZE, MPI_PACKED, MPI_ANY_SOURCE, TAG_EVENT, MPI_COMM_WORLD, &status);
 
    position = 0;
    MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.type), 1, MPI_UNSIGNED, MPI_COMM_WORLD); 

    switch(m_msg.type) {
	case Message_s :: M_UINT32:
	    MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.compIndex), 1, MPI_INT, MPI_COMM_WORLD);
	    MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.inputIndex), 1, MPI_INT, MPI_COMM_WORLD);
	    MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.isTick), 1, MPI_INT, MPI_COMM_WORLD);
	    if(m_msg.isTick == 0) {
		MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.sendTime), 1, MPI_DOUBLE, MPI_COMM_WORLD);
		MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.recvTime), 1, MPI_DOUBLE, MPI_COMM_WORLD);
	    }
	    else {
		MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.sendTick), 1,
		           MPI_UNSIGNED_LONG_LONG, MPI_COMM_WORLD);
		MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.recvTick), 1,
		           MPI_UNSIGNED_LONG_LONG, MPI_COMM_WORLD);
	    }

	    MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.uint32_data), 1, MPI_UNSIGNED,
	                                                             MPI_COMM_WORLD);
	break;
	case Message_s :: M_UINT64:
	    MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.compIndex), 1, MPI_INT, MPI_COMM_WORLD);
	    MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.inputIndex), 1, MPI_INT, MPI_COMM_WORLD);
	    MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.isTick), 1, MPI_INT, MPI_COMM_WORLD);
	    if(m_msg.isTick == 0) {
		MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.sendTime), 1, MPI_DOUBLE, MPI_COMM_WORLD);
		MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.recvTime), 1, MPI_DOUBLE, MPI_COMM_WORLD);
	    }
	    else {
		MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.sendTick), 1,
		           MPI_UNSIGNED_LONG_LONG, MPI_COMM_WORLD);
		MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.recvTick), 1,
		           MPI_UNSIGNED_LONG_LONG, MPI_COMM_WORLD);
	    }

	    MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.uint64_data), 1, MPI_UNSIGNED_LONG_LONG,
	                                                             MPI_COMM_WORLD);
	break;
	case Message_s :: M_SERIAL:
	    MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.compIndex), 1, MPI_INT, MPI_COMM_WORLD);
	    MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.inputIndex), 1, MPI_INT, MPI_COMM_WORLD);
	    MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.isTick), 1, MPI_INT, MPI_COMM_WORLD);
	    if(m_msg.isTick == 0) {
		MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.sendTime), 1, MPI_DOUBLE, MPI_COMM_WORLD);
		MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.recvTime), 1, MPI_DOUBLE, MPI_COMM_WORLD);
	    }
	    else {
		MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.sendTick), 1,
		           MPI_UNSIGNED_LONG_LONG, MPI_COMM_WORLD);
		MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.recvTick), 1,
		           MPI_UNSIGNED_LONG_LONG, MPI_COMM_WORLD);
	    }

	    MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.data_len), 1, MPI_INT, MPI_COMM_WORLD);
	    m_msg.data = &buf[position];
	    //MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.data), m_msg.data_len, MPI_UNSIGNED_CHAR,
	     //                                                        MPI_COMM_WORLD);
	break;
	case Message_s :: M_PROTO1:
	    stats_recv_proto1++;
	    MPI_Unpack(buf, m_recv_buf_size, &position, &(m_msg.uint32_data), 1, MPI_UNSIGNED,
	                                                             MPI_COMM_WORLD);
	break;

	default:
	    cerr << "Unknown message type: " << m_msg.type << endl;
	    exit(1);
    }//switch

    return m_msg;
}


#ifdef KERNEL_ANY_DATA_SIZE
//====================================================================
//====================================================================
//! Resize the send buffer.
//!
//! @arg \c size Size of the data to be sent. Note this does not include the
//! message header.
//!
void Messenger :: resize_send_buffer(int size)
{
    assert(m_header_size + size > m_send_buf_size);

    m_send_buf_size = m_header_size + size;
    delete[] m_send_buf;
    m_send_buf = new unsigned char[m_send_buf_size];
}
#endif


//====================================================================
//====================================================================
void Messenger :: print_stats(std::ostream& out)
{
    out << "  messages sent: " << m_numSent << " (total) " << stats_sent_proto1 << " (protocol) " << m_numSent-stats_sent_proto1 << endl
        << "  messages received: " << m_numReceived << " (total) " << stats_recv_proto1 << " (protocol) " << m_numReceived-stats_recv_proto1 << endl;
}

NullMsg_t* Messenger::RecvPendingNullMsg()
{
  static NullMsg_t msg;
  static std::list<NullMsg_t> nullMsgQueue;

  // First, check queue for pending messages:
  for(std::list<NullMsg_t>::iterator it=nullMsgQueue.begin();it!=nullMsgQueue.end();++it)
  {
    if(it->txCnt<=m_rxcount[it->src])
    {
      msg=*it;
      nullMsgQueue.erase(it);
      return &msg;
    }
  }

  // Then, check MPI for pending messages:
  while(MPI::COMM_WORLD.Iprobe(MPI::ANY_SOURCE, TAG_NULLMSG))
  {
    MPI::COMM_WORLD.Recv(&msg, sizeof(msg), MPI::BYTE, MPI::ANY_SOURCE, TAG_NULLMSG);
    if(msg.txCnt<=m_rxcount[msg.src]) return &msg;
    else nullMsgQueue.push_back(msg);
  }

  return NULL;
}

void Messenger::SendNullMsg(NullMsg_t* msg)
{
  msg->txCnt=m_txcount[msg->dst];
  MPI::COMM_WORLD.Send(msg, sizeof(NullMsg_t), MPI::BYTE, msg->dst, TAG_NULLMSG);
}


Messenger TheMessenger;

} //namespace kernel
} //namespace manifold

#endif //#ifndef NO_MPI





