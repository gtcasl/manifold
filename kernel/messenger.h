#ifndef MANIFOLD_KERNEL_MESSENGER_H
#define MANIFOLD_KERNEL_MESSENGER_H

#ifndef NO_MPI

#define KERNEL_ANY_DATA_SIZE

#include <stdint.h>

#include "message.h"
#include "mpi.h"

namespace manifold {
namespace kernel {

class Messenger {
  private:
    typedef enum{TAG_EVENT, TAG_NULLMSG} msgTag_t;

public:
    Messenger();
    #ifdef KERNEL_ANY_DATA_SIZE
    void init(int argc, char** argv);
    #else
    //if arbitrary data size not defined, we set a max data size, default 16384.
    void init(int argc, char** argv, int max_data_size=16384);
    #endif

    void finalize();

    //! Return the MPI rank, i.e., the ID of the node.
    int get_node_id() const { return m_nodeId; }

    //! Return the number of nodes.
    int get_node_size() const { return m_nodeSize; }

    //! Return the number of messages sent.
    int get_numSent() const { return m_numSent; }

    //! Return the number of messages received.
    int get_numReceived() const { return m_numReceived; }

    //! Enter a synchronization barrier.
    void barrier();

    //! Perform an AllGather.
    void allGather(char* item, int itemSize, char* array);

    void send_uint32_msg(int dest, int compIndex, int inputIndex,
                         Ticks_t sendTick, Ticks_t recvTick, uint32_t data);

    void send_uint32_msg(int dest, int compIndex, int inputIndex,
                         double sendTime, double recvTime, uint32_t data);

    void send_uint64_msg(int dest, int compIndex, int inputIndex,
                         Ticks_t sendTick, Ticks_t recvTick, uint64_t data);

    void send_uint64_msg(int dest, int compIndex, int inputIndex,
                         double sendTime, double recvTime, uint64_t data);

    void send_serial_msg(int dest, int compIndex, int inputIndex,
                         Ticks_t sendTick, Ticks_t recvTick, int len);

    void send_serial_msg(int dest, int compIndex, int inputIndex,
                         double sendTime, double recvTime, int len);

    void send_proto1_msg(int data, int root);
    void broadcast_proto1(int dest, int data);


    Message_s& irecv_message(int* received);

    //! Get the data portion of buffer size.
    #ifdef KERNEL_ANY_DATA_SIZE
    int get_send_data_buffer_size() { return m_send_buf_size - m_header_size; }
    void resize_send_buffer(int size); //allow send buffer to be resized.
    #else
    int get_send_data_buffer_size() { return m_max_data_size; }
    #endif


    //! Return the starting address of the data portion of the send buffer, which
    //! is where the serialized data should go into.
    unsigned char* get_send_buf_data_addr() { return &m_send_buf[m_header_size]; }

    void print_stats(std::ostream&);

    /**
     * Receives a pending null message.
     * If a null message received from MPI cannot be processed yet, it is stored
     * in an internal queue and the next message is returned instead.
     * On later calls of RecvPendingNullMsg(), the queue is checked for pending
     * null messages before messages are retrieved from MPI.
     * @return Pointer to message or NULL if no message is pending.
               ATTENTION: The pointer is only valid till the next call to
                          RecvPendingNullMsg(). Now cleanup is necessary.
     */
    NullMsg_t* RecvPendingNullMsg();

    /**
     * Sends the given message to msg->dst, and adds corresponding txCnt prior to sending.
     * @param msg The message to send
     */
    void SendNullMsg(NullMsg_t* msg);

private:
    void send_message(int dest, unsigned char* buf, int position);
    Message_s& unpack_message(unsigned char*);

    int m_nodeId;
    int m_nodeSize;
    unsigned m_numSent; //number of sent messages.
    unsigned m_numReceived; //number of received messages.
    unsigned stats_sent_proto1;
    unsigned stats_recv_proto1;

    Message_s m_msg; // message holder; avoid allocating/deallocating all the time

    unsigned char* m_recv_buf;

    #ifndef KERNEL_ANY_DATA_SIZE
    int m_max_data_size; //max size of user data
    #endif

    int m_recv_buf_size;

    unsigned char* m_send_buf;
    int m_send_buf_size;
    int m_header_size; //size of the header portion of message.

    uint64_t* m_rxcount;
    uint64_t* m_txcount;
};



extern Messenger TheMessenger;

} //namespace kernel
} //namespace manifold


#endif //#ifndef NO_MPI



#endif
