#ifndef SHM_H
#define SHM_H

#ifdef USE_QSIM

#include "qsim.h"



class SharedMemBuffer {
public:
    SharedMemBuffer(char* fn, char id, unsigned size);
    ~SharedMemBuffer();

    void writer_init();
    void reader_init();

    bool is_full();
    bool is_empty();
    int get_buf_max_entries() { return m_MAX_ENTRIES; } //how many data entries it can hold
    int get_buf_num_entries();
    bool is_writer_done() { return *m_term_flag_ptr != 0; }

    void write(Qsim::QueueItem& item);
    Qsim::QueueItem read();

void dbg_print();

protected:
    int32_t get_head() { return *m_head_ptr; }
    int32_t get_tail() { return *m_tail_ptr; }
    int8_t get_head_wrap_count() { return *m_head_wrap_count_ptr; }
    int8_t get_tail_wrap_count() { return *m_tail_wrap_count_ptr; }

private:
    void head_inc();
    void tail_inc();

    char* m_shm; //shared mem segment
    int m_MAX_ENTRIES; //how many entries the buffer can hold

    Qsim::QueueItem* m_item_array; //interprete the shared mem as a QueueItem array.
    int32_t* m_head_ptr;
    int32_t* m_tail_ptr;
    int8_t* m_head_wrap_count_ptr;
    int8_t* m_tail_wrap_count_ptr;
    uint8_t* m_term_flag_ptr; //termination flag
};





#endif //#ifdef USE_QSIM

#endif
