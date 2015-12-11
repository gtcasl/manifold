#ifndef MANIFOLD_MCP_CACHE_HASH_TABLE_H
#define MANIFOLD_MCP_CACHE_HASH_TABLE_H

#include <list>
#include <map>
#include <vector>
#include <assert.h>

#include "cache_req.h"

#include <iostream>

namespace manifold {
namespace mcp_cache_namespace {

class hash_entry;
class hash_set;
class hash_table;

class hash_entry {
    public:
        hash_entry (hash_set *hset, unsigned idx);
        ~hash_entry (void);

	unsigned get_idx() { return idx; }
	unsigned get_set_idx();

        bool get_have_data() const { return have_data; }
        void set_have_data(bool h) { have_data = h; }

	void set_dirty(bool d) { dirty = d; }
	bool is_dirty() { return dirty; }

	void invalidate ();

	paddr_t get_line_addr();

#ifndef MCP_CACHE_UTEST
    private:
#endif
        friend class hash_set;
        friend class hash_table;

        hash_set * const my_set;
	const unsigned idx; //index within the whole table.
        paddr_t tag;
        bool free;
        bool have_data;
        bool dirty;
};




class hash_set {
   public:
      hash_set (hash_table *table, int assoc, unsigned set_idx);
      ~hash_set (void);

      hash_entry* get_entry (paddr_t tag);
      void replace_entry (hash_entry *incoming, hash_entry *outgoing);

      hash_table* get_table() const { return my_table; }

      void get_entries(std::vector<hash_entry*>&);

      hash_table* get_table() { return my_table; }
      unsigned get_index() { return index; }

      //debug
      void dbg_print(std::ostream&);

#ifndef MCP_CACHE_UTEST
   private:
#endif
      friend class hash_table;

      const unsigned index; //set's index

      void update_lru (hash_entry *entry);

      hash_table *my_table;
      const int assoc;
      std::list<hash_entry *> set_entries;
};



class hash_table {
   public:
      hash_table (const char *name, int size,
                  int assoc, int block_size, int hit_time,
                  int lookup_time, replacement_policy_t rp);
      hash_table (cache_settings my_settings);
      ~hash_table (void);

      int get_size() const { return size; }
      int get_block_size() const { return block_size; }
      int get_hit_time() const { return hit_time; }
      int get_lookup_time() const { return lookup_time; }
      int get_index_bits() const { return num_index_bits; }
      int get_offset_bits() const { return num_offset_bits; }
      paddr_t get_tag_mask() const { return tag_mask; }
      paddr_t get_tag (paddr_t addr);
      paddr_t get_index (paddr_t addr);
      paddr_t get_line_addr (paddr_t addr);

      bool has_match(paddr_t addr);
      bool can_allocate_for (paddr_t addr);
      hash_entry* reserve_block_for (paddr_t addr);


      hash_set* get_set (paddr_t addr);
      hash_entry* get_entry (paddr_t addr);
      hash_entry* get_replacement_entry (paddr_t addr);

      void get_sets(std::vector<hash_set*>&);
      int get_num_entries() const { return sets * assoc; }

      void update_lru (paddr_t addr);

      unsigned get_occupancy() { return occupancy; }
      void increase_occupancy() { occupancy++; }
      void decrease_occupancy()
      { 
          assert(occupancy > 0);
	  occupancy--;
      }

      //debug
      void dbg_print(std::ostream&);

#ifndef MCP_CACHE_UTEST
   private:
#endif

      const char *name;
      const int size;
      const int assoc;
      const int sets;
      const int block_size;
      const int hit_time;
      const int lookup_time;
      const replacement_policy_t replacement_policy;

      int num_index_bits;
      int num_offset_bits;
      paddr_t tag_mask;
      paddr_t index_mask;
      paddr_t offset_mask;

      std::map<paddr_t, hash_set *> my_sets;

      unsigned occupancy; //number of active entries
};

inline void hash_entry :: invalidate ()
{
    free = true;
    have_data = false;
    dirty = false;
    my_set->get_table()->decrease_occupancy();
}



} //namespace mcp_cache_namespace
} //namespace manifold

#endif // MANIFOLD_MCP_CACHE_HASH_TABLE_H
