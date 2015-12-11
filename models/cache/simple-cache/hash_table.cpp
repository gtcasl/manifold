#include <assert.h>
//#include <iostream>
#include <math.h>
#include <string.h>

#include "hash_table.h"
#include "cache_req.h"

using namespace std;


namespace manifold {
namespace simple_cache {



// mshr_entry: Constructor
mshr_entry::mshr_entry (Cache_req *request)
{
   creq = request;
}





//! hash_entry: Constructor
//!
//! This is a single cache line
//! associated with one set within the cache. Contains pointers back
//! to the parent table and parent set, the address tag associated with
//! this line, and valid/dirty bits to indicate status.
hash_entry::hash_entry (hash_table *table, hash_set *set, paddr_t tag, bool valid)
{
   this->my_table = table;
   this->my_set = set;
   this->tag = tag;
   this->valid = valid;
   this->dirty = false;
}


//! hash_set: Constructor
//!
//! This is a set of cache lines, the number
//! of which is determined by the cache's associativity. Contains a pointer
//! back to the parent table, the associativity factor, and a list of pointers
//! to the hash_entries it owns.
hash_set::hash_set (hash_table *table)
{
    my_table = table;
    assoc = table->get_assoc();

    // initialize the hash_entry list - all entries initialized to tag
    // 0x0 and marked invalid
    set_entries.clear();
    for (int i = 0; i < assoc; i++)
        set_entries.push_front(new hash_entry (table, this, 0x0, false));
}


hash_set::~hash_set (void)
{
   while (!set_entries.empty ())
   {
      delete set_entries.back();
      set_entries.pop_back();
   }
}


//! Calls get_entry to determine whether the request is a hit or miss. If get_entry
//! returns NULL, we have a miss. Otherwise, we have a hit and we update the LRU
//! status of the member entries.
bool hash_set::process_request (paddr_t addr)
{
   hash_entry *entry = get_entry (my_table->get_tag (addr));

   if (entry)
   {
      update_lru (entry);
      return true;
   }
   return false;
}


//! hash_set: get_entry
//!
//! Loops through each entry in the set to see if there's a tag match. Return NULL
//! if there's no match. Otherwise, return a pointer to the matching entry.
hash_entry* hash_set::get_entry (paddr_t tag)
{
   list<hash_entry *>::iterator it;

   for (it = set_entries.begin(); it != set_entries.end(); it++)
   {
      if ((tag == (*it)->tag) && (*it)->valid)
      {
         return *it;
      }
   }

   return 0;
}

//! hash_set: replace_entry
//!
//! Replaces a line in the set with a tag; Place the entry
//! at the front of the list so that it's ranked as MRU.
void hash_set::replace_entry (paddr_t new_tag)
{
    hash_entry* victim = set_entries.back();
    set_entries.pop_back();
    victim->tag = new_tag;
    victim->valid = true;
    victim->dirty = false;
    set_entries.push_front(victim);

}


//! hash_set: update_lru
//!
//! On a hit, move this entry to the front of the list to indicate
//! MRU status.
void hash_set::update_lru (hash_entry *entry)
{
   set_entries.remove (entry);
   set_entries.push_front (entry);
}









hash_table::hash_table (int sz, int as, int block, int hit, int lookup)
{
    assert( sz % (as*block) == 0); // size/(assoc*block) = num of sets; must be integer

    size = sz;
    assoc = as;
    block_size = block;
    sets = size / (assoc * block_size);
    hit_time = hit;
    lookup_time = lookup;

    num_index_bits = (int) log2 (sets);
    assert(sets == 0x1 << num_index_bits); //sets must be power of 2
    num_offset_bits = (int) log2 (block_size);
    assert(block_size == 0x1 << num_offset_bits); //block must be power of 2

    tag_mask = ~0x0;
    tag_mask = tag_mask << (num_index_bits + num_offset_bits);

    index_mask = ~0x0;
    index_mask = index_mask << num_offset_bits;
    index_mask = index_mask & ~tag_mask;

    offset_mask = ~0x0;
    offset_mask = offset_mask << num_offset_bits;

    for (int i = 0; i < sets; i++)
        my_sets[i] = new hash_set (this);

}


hash_table::~hash_table (void)
{
   for (int i = 0; i < sets; i++)
      delete my_sets[i];
   my_sets.clear();
}

//! hash_table: get_tag
//!
//! Mask out the tag bits from the address and return the tag.
paddr_t hash_table::get_tag (paddr_t addr)
{
   return (addr & tag_mask);
}

//! hash_table: get_index
//!
//! Mask out the index bits from the address and return the index.
paddr_t hash_table::get_index (paddr_t addr)
{
   return ((addr & index_mask) >> num_offset_bits);
}

//! hash_table: get_line_addr
//!
//! Mask out the offset bits from the address and return the tag plus index.
paddr_t hash_table::get_line_addr (paddr_t addr)
{
   return (addr & offset_mask);
}

//! hash_table: get_set
//!
//! Return a pointer to the set addressed.
hash_set* hash_table::get_set (paddr_t addr)
{
   return my_sets[get_index (addr)];
}

//! hash_table: get_entry
//!
//! Return a pointer to the cache line addressed.
hash_entry* hash_table::get_entry (paddr_t addr)
{
   return my_sets[get_index (addr)]->get_entry (get_tag (addr));
}






//! Called when the cache receives a request from the processor or lower-level
//! cache. Returns true if hit, false if miss. In case of hit, the LRU is updated.
bool hash_table::process_request (paddr_t addr)
{
    hash_set* set = get_set (addr);
    return set->process_request (addr);
}




//! hash_table: process_response
//!
//! Called when a LOAD response comes back from memory. Evict a block if
//! necessary.
void hash_table::process_response (paddr_t addr)
{
   hash_set* set = get_set (addr);
   set->replace_entry (get_tag(addr));
}


} //namespace simple_cache
} //namespace manifold


