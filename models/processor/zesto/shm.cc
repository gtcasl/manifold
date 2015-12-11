#ifdef USE_QSIM


#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <assert.h>

#include "shm.h"

using namespace std;
using namespace Qsim;


//#########################################################################
//#########################################################################
//! First 2 parameters are used to create the shared mem segment.
//! size is the size of the segment in bytes.
SharedMemBuffer :: SharedMemBuffer(char* fn, char id, unsigned size)
{
    const unsigned SHARED_VAR_BYTES = 12; //number of bytes reserved for shared variables; must be multiple of 4

    assert(id != 0);
    assert(size > SHARED_VAR_BYTES);

    //ensure size is power of 2.
    int i = 0;
    while ((size >> i) > 1)
        i++;

    if(size != (0x1 << i)) {
        cerr << "size " << size << " is not a power of 2!\n";
	exit(1);
    }


    m_MAX_ENTRIES = (size - SHARED_VAR_BYTES) / sizeof(QueueItem);
    assert(m_MAX_ENTRIES > 1);


    //create the shared mem segment
    key_t key;
    if((key = ftok(fn, id)) == -1) {
        perror("ftok()");
        exit(1);
    }

    int shmid;
    if((shmid = shmget(key, size, 0644 | IPC_CREAT)) == -1) {
        perror("shmget()");
        exit(1);
    }

    m_shm = (char *)shmat(shmid, (void *)0, 0);
    if(m_shm == (char *)(-1)) {
        perror("shmat()");
        exit(1);
    }

    //shared mem layout
    //   ------------------------------
    //   |                            |
    //   |         data entries       |
    //   |                            |
    //   ------------------------------
    //   |       head (4 bytes)       |
    //   ------------------------------
    //   |       tail (4 bytes)       |
    //   ------------------------------
    //   |  head wrap count (1 byte)  |
    //   ------------------------------
    //   |  tail wrap count (1 byte)  |
    //   ------------------------------
    //   |  termination flag (1 byte) |
    //   ------------------------------
    //   |      unused (1 byte)       |
    //   ------------------------------
    //
    //set up the pointers
    int position = size - SHARED_VAR_BYTES;
    m_head_ptr = (int32_t*)(&m_shm[position]);
    position += sizeof(int32_t);

    m_tail_ptr = (int32_t*)(&m_shm[position]);
    position += sizeof(int32_t);

    m_head_wrap_count_ptr = (int8_t*)&m_shm[position];
    position += sizeof(int8_t);

    m_tail_wrap_count_ptr = (int8_t*)&m_shm[position];
    position += sizeof(int8_t);

    m_term_flag_ptr = (uint8_t*)&m_shm[position];
    position += sizeof(uint8_t);

    //m_item_array = (QueueItem*)m_shm;

    cout << "m_shm= " << m_shm << endl;
}


SharedMemBuffer :: ~SharedMemBuffer()
{
    if(shmdt(m_shm) == -1) {
        perror("shmdt()");
        exit(1);
    }
}


//=========================================================================
//=========================================================================
void SharedMemBuffer :: writer_init()
{
    *m_head_ptr = 0;
    *m_tail_ptr = 0;
    *m_head_wrap_count_ptr = 0;
    *m_tail_wrap_count_ptr = 0;
    *m_term_flag_ptr = 0;
}

//=========================================================================
//=========================================================================
void SharedMemBuffer :: reader_init()
{
    *m_tail_ptr = 0;
    *m_tail_wrap_count_ptr = 0;
}

//=========================================================================
//=========================================================================
bool SharedMemBuffer :: is_full()
{
    return (get_head() == get_tail() && get_head_wrap_count() != get_tail_wrap_count());
}


//=========================================================================
//=========================================================================
bool SharedMemBuffer :: is_empty()
{
    return (get_head() == get_tail() && get_head_wrap_count() == get_tail_wrap_count());
}


//=========================================================================
//=========================================================================
int SharedMemBuffer :: get_buf_num_entries()
{
    int num = get_head() - get_tail();
    if(num < 0)
        num += m_MAX_ENTRIES;
    else if(num == 0) {
        if(is_full())
	    num = m_MAX_ENTRIES;
    }
    return num;
}


//=========================================================================
//=========================================================================
void SharedMemBuffer :: write(QueueItem& item)
{
    if(is_full()) {
        cerr << "Attemp to write when SHM is full!\n";
	exit(1);
    }

    int pos = get_head() * sizeof(QueueItem);
    memcpy(&m_shm[pos], &item, sizeof(QueueItem));
    head_inc();
}


//=========================================================================
//=========================================================================
void SharedMemBuffer :: head_inc()
{
    int head = *m_head_ptr;
    head++;
    if(head >= m_MAX_ENTRIES) {
        head = 0;
	//toggle head wrap count
	if(*m_head_wrap_count_ptr == 0)
	    *m_head_wrap_count_ptr = 1;
	else
	    *m_head_wrap_count_ptr = 0;
    }
    *m_head_ptr = head;
}


//=========================================================================
//=========================================================================
QueueItem SharedMemBuffer :: read()
{
    if(is_empty()) {
        cerr << "Attemp to read when SHM is empty!\n";
	exit(1);
    }

    int pos = get_tail() * sizeof(QueueItem);
    QueueItem item(0);
    memcpy(&item, &m_shm[pos], sizeof(QueueItem));
    tail_inc();
    return item;
}


//=========================================================================
//=========================================================================
void SharedMemBuffer :: tail_inc()
{
    int tail = *m_tail_ptr;
    tail++;
    if(tail >= m_MAX_ENTRIES) {
        tail = 0;
	//toggle tail wrap count
	if(*m_tail_wrap_count_ptr == 0)
	    *m_tail_wrap_count_ptr = 1;
	else
	    *m_tail_wrap_count_ptr = 0;
    }
    *m_tail_ptr = tail;
}


//=========================================================================
//=========================================================================
void SharedMemBuffer :: dbg_print()
{

    cout << "head= " << get_head() << " tail= " << get_tail()
         << " head_wrap= " << (int)get_head_wrap_count() << " tail_wrap= " << (int)get_tail_wrap_count()
         << endl;

}


#endif //#ifdef USE_QSIM


