#ifndef MANIFOLD_SIMPLE_CACHE_HASH_TABLE_H_
#define MANIFOLD_SIMPLE_CACHE_HASH_TABLE_H_

#include <list>
#include <map>
#include <stdint.h>


namespace manifold {
namespace simple_cache {

typedef uint64_t paddr_t;

class hash_entry;
class hash_set;
class hash_table;

class Cache_req;

class mshr_entry {
public:
    mshr_entry (Cache_req *request);

    Cache_req *creq;
};


class hash_entry {
   public:
      hash_entry (hash_table *table, hash_set *set, paddr_t tag, bool valid);

      hash_table *my_table;
      hash_set *my_set;
      paddr_t tag;
      bool valid;
      bool dirty;
};

class hash_set {
   public:
      hash_set (hash_table *table);
      ~hash_set (void);

      bool process_request (paddr_t addr);
      hash_entry* get_entry (paddr_t tag);
      void replace_entry (paddr_t addr);
      //hash_entry* get_replacement_entry (void);

#ifndef SIMPLE_CACHE_UTEST
   private:
#endif
      void update_lru (hash_entry *entry);

      hash_table *my_table;
      int assoc;
      std::list<hash_entry *> set_entries;
};


class hash_table {
   public:
      hash_table (int size,
                  int assoc, int block_size, int hit_time,
                  int lookup_time);
      ~hash_table (void);

      int get_assoc() { return assoc; }
      int get_hit_time() const { return hit_time; }
      int get_lookup_time() const { return lookup_time; }
      int get_index_bits() const { return num_index_bits; }
      int get_offset_bits() const { return num_offset_bits; }
      paddr_t get_tag_mask() const { return tag_mask; }
      paddr_t get_tag (paddr_t addr);
      paddr_t get_index (paddr_t addr);
      paddr_t get_line_addr (paddr_t addr);

      bool process_request (paddr_t addr);
      void process_response (paddr_t addr);

#ifndef SIMPLE_CACHE_UTEST
   private:
#endif
      hash_set* get_set (paddr_t addr);
      hash_entry* get_entry (paddr_t addr);

      int size;
      int assoc;
      int sets;
      int block_size;
      int hit_time;
      int lookup_time;
      //replacement_policy_t replacement_policy;

      int num_index_bits;
      int num_offset_bits;
      paddr_t tag_mask;
      paddr_t index_mask;
      paddr_t offset_mask;

      std::map<paddr_t, hash_set *> my_sets;
      //std::list<mshr_entry *> mshr_buffer;
};

} //namespace simple_cache
} //namespace manifold

#endif // MANIFOLD_SIMPLE_CACHE_HASH_TABLE_H_
