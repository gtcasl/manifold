#ifndef MANIFOLD_ZESTO_FETCH_DPM_H
#define MANIFOLD_ZESTO_FETCH_DPM_H

#include "../zesto-fetch.h"

class core_fetch_DPM_t:public core_fetch_t
{
  enum fetch_stall_t {FSTALL_byteQ_FULL, /* byteQ is full */
                      FSTALL_TBR,        /* predicted taken */
                      FSTALL_EOL,        /* hit end of cache line */
                      FSTALL_SPLIT,      /* instruction split across two lines (special case of EOL) */
                      FSTALL_BOGUS,      /* encountered invalid inst on wrong-path */
                      FSTALL_SYSCALL,    /* syscall waiting for pipe to clear */
                      FSTALL_ZPAGE,      /* fetch request from zeroth page of memory */
                      FSTALL_num
                     };

  public:

  /* constructor, stats registration */
  core_fetch_DPM_t(struct core_t * const core);
  virtual void reg_stats(struct stat_sdb_t * const sdb);
  virtual void update_occupancy(void);

  /* simulate one cycle */
  virtual void step(void);

  /* decode interface */
  virtual bool Mop_available(void);
  virtual struct Mop_t * Mop_peek(void);
  virtual void Mop_consume(void);

  /* enqueue a jeclear event into the jeclear_pipe */
  virtual void jeclear_enqueue(struct Mop_t * const Mop, const md_addr_t New_PC);
  /* recover the front-end after the recover request actually
     reaches the front end. */
  virtual void recover(const md_addr_t new_PC); 

  //for debug
  void dbg_dump_IQ(ostream& out);
  void dbg_dump_IQ() { dbg_dump_IQ(cout); }
  void dbg_dump_predecode(ostream& out);
  void dbg_dump_predecode() { dbg_dump_predecode(cout); }
  void dbg_dump_byteQ(ostream& out);
  void dbg_dump_byteQ() { dbg_dump_byteQ(cout); }

#ifdef ZESTO_UTEST
  public:
#else
  protected:
#endif
  int byteQ_num;
  int IQ_num;
  int IQ_uop_num;
  int IQ_eff_uop_num;

  /* The byteQ contains the raw bytes (well, the addresses thereof).
     This is similar to the old IFQ in that the contents of the I$
     are delivered here.  Each entry contains raw bytes, and so
     this may correspond to many, or even less than one, instruction
     per entry. */
  struct byteQ_entry_t {
    void print(ostream&);
    md_addr_t addr;
    md_addr_t paddr;
    tick_t when_fetch_requested;
    tick_t when_fetched;
    tick_t when_translation_requested;
    tick_t when_translated;
    int num_Mop;
    int MopQ_first_index;
    seq_t action_id;
  } * byteQ;
  int byteQ_linemask; /* for masking out line offset */
  int byteQ_head;
  int byteQ_tail;

  /* buffer/queue between predecode and the main decode pipeline */
  struct Mop_t ** IQ;
  int IQ_head;
  int IQ_tail;

  /* predecode pipe: finds instruction boundaries and separates
     them out so that each entry corresponds to a single inst. */
  struct Mop_t *** pipe;

  /* branch misprediction recovery pipeline (or other pipe flushes)
     to model non-zero delays between the back end misprediction
     detection and the actual front-end recovery. */
  struct jeclear_pipe_t {
    md_addr_t New_PC;
    struct Mop_t * Mop;
    seq_t action_id;
  } * jeclear_pipe;

  static const char *fetch_stall_str[FSTALL_num];

  bool predecode_enqueue(struct Mop_t * const Mop);
  void byteQ_request(const md_addr_t lineaddr, const md_addr_t lineaddr_phys);
  bool byteQ_is_full(void);
  bool byteQ_already_requested(const md_addr_t addr);
/*
  static void IL1_callback(void * const op);
  static void ITLB_callback(void * const op);
  static bool translated_callback(void * const op, const seq_t action_id);
  static seq_t get_byteQ_action_id(void * const op);
*/
};

#endif //MANIFOLD_ZESTO_FETCH_DPM_H


