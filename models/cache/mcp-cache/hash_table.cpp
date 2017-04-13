#include <assert.h>
#include <iostream>
#include <math.h>
#include <string.h>

#include "hash_table.h"

using namespace std;
using namespace manifold::mcp_cache_namespace;

namespace manifold {
namespace pagevault {
int ea_get_partition_size();
}
}

//! hash_entry: Constructor
//!
//! This is a single cache line
//! associated with one set within the cache. Contains pointers back
//! to the parent table and parent set, the address tag associated with
//! this line, and valid/dirty bits to indicate status.
//!
//! @param \c hset  The hash set that holds this entry.
//! @param \c idx  The hash entry's index within the whole table; 0-based.
hash_entry::hash_entry (hash_set * const hset, unsigned indx) :
    my_set(hset),
    idx(indx)
{
    this->tag = 0x0;
    this->free = true;
    this->have_data = false;
    this->dirty = false;
}

// hash_entry: Destructor
hash_entry::~hash_entry (void)
{
}


unsigned hash_entry :: get_set_idx()
{
    return my_set->get_index();
}

paddr_t hash_entry :: get_line_addr()
{
    return my_set->get_line_addr(tag);
}



//! hash_set: Constructor
//!
//! This is a set of cache lines, the number
//! of which is determined by the cache's associativity. Contains a pointer
//! back to the parent table, the associativity factor, and a list of pointers
//! to the hash_entries it owns.
//!
//! @param \c table  The hash table that holds this set.
//! @param \c asoc  The associativity (number of entries in the set).
//! @param \c set_idx  The hash set's index within the table; 0-based.
hash_set::hash_set (hash_table *table, int asoc, unsigned set_idx) :
    index(set_idx), assoc(asoc)
{
    this->my_table = table;

    set_entries.clear();
    for (int i = 0; i < assoc; i++)
    {
        set_entries.push_front(new hash_entry (this, set_idx * assoc + i));
    }
}

// hash_set: Destructor
hash_set::~hash_set (void)
{
    while (!set_entries.empty ())
    {
        delete set_entries.back();
        set_entries.pop_back();
    }
}


void hash_set :: dbg_print(ostream& out)
{
    for(list<hash_entry *>::iterator it = set_entries.begin(); it != set_entries.end(); ++it)
    {
	if((*it)->free)
	    out << (*it)->get_idx() << "  " << (*it) << "  " << "free\n";
	else
	    out << (*it)->get_idx() << "  " << (*it) << "  " <<hex<< (*it)->tag <<dec<< "\n";
    }

}


//! hash_set: get_entry
//!
//! Loops through each entry in the set to see if there's a tag match. Return NULL
//! if there's no match. Otherwise, return a pointer to the matching entry.
hash_entry* hash_set::get_entry (paddr_t tag)
{
    hash_entry *entry = NULL;
    list<hash_entry *>::iterator it;

    for (it = set_entries.begin(); it != set_entries.end(); ++it)
    {
        if ((tag == (*it)->tag) && !(*it)->free)
        {
            entry = *it;
            break;
        }
    }

    return entry;
}

paddr_t hash_set::get_line_addr(paddr_t tag) {
  return my_table->get_line_addr(tag, index);
}

//! hash_set: replace_entry
//!
//! Replaces a line in the set with a new one fetched from lower level. Places
//! it at the front of the list so that it's ranked as MRU.
void hash_set::replace_entry (hash_entry *incoming, hash_entry *outgoing)
{
    list<hash_entry *>::iterator it;

    assert (incoming != NULL);
    assert (outgoing != NULL);


    for (it = set_entries.begin(); it != set_entries.end(); it++)
    {
        if ((*it)->tag == outgoing->tag)
        {
            set_entries.remove(outgoing);
            set_entries.push_front(incoming);
            break;
        }
    }
}

//! hash_set: get_replacement_entry
//!
//! Returns the least recently used entry in the set. This is always the entry
//! at the back of the list container.
hash_entry* hash_table::get_replacement_entry (paddr_t addr)
{
    return (get_set(addr)->set_entries.back());
} 

//! hash_set: get_replacement_entry
hash_entry *hash_table::get_replacement_entry(paddr_t addr, const std::vector<uint64_t>& skiplist) {
  //
  // traverse the cache backward and return the first block 
  // that doesn't belong to the input list
  //
  hash_set* set = get_set(addr);  
  if (skiplist.empty())
    return set->set_entries.back();
  
  for (auto iter = set->set_entries.rbegin(), 
       iterE = set->set_entries.rend(); iter != iterE; ++iter) {
    
    hash_entry* he = *iter;        
    bool found = false;    
    if (!he->free) {
      for (uint64_t pa : skiplist) {
        int idx = get_index(pa);
        paddr_t tag = get_tag(pa);
        if (set->index == idx 
         && he->tag == tag) {
          found = true;
          break;
        }
      }
    }
    
    if (!found) {
      return he;
    }
  }
  
  abort();
  return nullptr;  
}

//! hash_set: update_lru
//!
//! On a hit, move this entry to the front of the list container to indicate
//! MRU status.
void hash_set::update_lru (hash_entry *entry)
{
    set_entries.remove (entry);
    set_entries.push_front (entry);
}


void hash_set :: get_entries(vector<hash_entry*>& entries)
{
    for(list<hash_entry*>::iterator it = set_entries.begin(); it != set_entries.end(); ++it) {
        entries.push_back(*it);
    }
}




// hash_table: Constructor
hash_table::hash_table (const char *nm, int sz,
                        int asoc, int blok_sz, int hit_t,
                        int lookup_t, replacement_policy_t rp) :
    name(nm),
    size(sz),
    assoc(asoc),
    sets((sz) / (asoc * blok_sz)),
    block_size(blok_sz),
    hit_time(hit_t),
    lookup_time(lookup_t),
    replacement_policy(rp)
{
    num_index_bits = (int)log2(sets);
    num_offset_bits = (int)log2(block_size);
    num_index_shift = num_offset_bits;
  
    offset_mask = (1 << num_offset_bits) - 1;
    index_mask = (paddr_t)((1 << num_index_bits) - 1) << num_index_shift;
    tag_mask = ~(index_mask | offset_mask);

    for (int i = 0; i < this->sets; i++)
        my_sets[i] = new hash_set (this, assoc, i);
}

// hash_table: Constructor
hash_table::hash_table (cache_settings my_settings) :
    name(my_settings.name),
    size(my_settings.size),
    assoc(my_settings.assoc),
    sets((my_settings.size) / (my_settings.assoc * my_settings.block_size)),
    block_size(my_settings.block_size),
    hit_time(my_settings.hit_time),
    lookup_time(my_settings.lookup_time),
    replacement_policy(my_settings.replacement_policy),
    occupancy(0)
{
    num_index_bits = (int)log2(sets);
    num_offset_bits = (int)log2(block_size);
    num_index_shift = num_offset_bits;
  
    offset_mask = (1 << num_offset_bits) - 1;
    index_mask = (paddr_t)((1 << num_index_bits) - 1) << num_index_shift;
    tag_mask = ~(index_mask | offset_mask);

    for (int i = 0; i < this->sets; i++)
        my_sets[i] = new hash_set (this, assoc, i);
}

// hash_table: Destructor
hash_table::~hash_table ()
{
    for (int i = 0; i < sets; i++) {
        delete my_sets[i];
    }
    my_sets.clear();
}


void hash_table :: dbg_print(ostream& out)
{
    for (int i = 0; i < sets; i++) {
        my_sets[i]->dbg_print(out);
    }
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
    return ((addr & index_mask) >> num_index_shift);
}

paddr_t hash_table::get_line_addr(paddr_t tag, unsigned index) {
  return tag | ((paddr_t)index << num_index_shift);
}

//! hash_table: get_line_addr
//!
//! Mask out the offset bits from the address and return the tag plus index.
paddr_t hash_table::get_line_addr (paddr_t addr)
{
    return (addr & ~offset_mask);
}

//! hash_table: get_set
//!
//! Return a pointer to the set addressed.
hash_set* hash_table::get_set (paddr_t addr)
{
    return my_sets[get_index(addr)];
}

//! hash_table: get_entry
//!
//! Return a pointer to the cache line addressed.
hash_entry* hash_table::get_entry (paddr_t addr)
{
    return my_sets[get_index(addr)]->get_entry (get_tag(addr));
}


bool hash_table::has_match(paddr_t addr)
{
    return (get_entry (addr) == NULL ? false : true);
}


#if 0
bool hash_table::can_allocate_for (paddr_t addr)
{
    hash_set *hs = my_sets[get_index (addr)];

    list<hash_entry *>::iterator it;

    for (it = hs->set_entries.begin(); it != hs->set_entries.end(); it++)
    {
        if ((*it)->free == true)
        {
            return true;
        }
    }

    return false;
}
#endif



hash_entry* hash_table::reserve_block_for (paddr_t addr)
{
    hash_entry* entry = 0;

    hash_set *hs = my_sets[get_index (addr)];

    paddr_t tag = get_tag (addr);

    list<hash_entry *>::iterator it;

    for (it = hs->set_entries.begin(); it != hs->set_entries.end(); it++)
    {
        if ((*it)->free == true)
        {
            (*it)->free = false;
            (*it)->tag = tag;
            entry = (*it);
	    occupancy++;
	    break;
        }
    }

    if(entry != 0)
        hs->update_lru(entry);

    return entry;
}





void hash_table :: get_sets(vector<hash_set*>& sets)
{
    for(map<paddr_t, hash_set*>::iterator it=my_sets.begin(); it != my_sets.end(); ++it)
        sets.push_back((*it).second);
}


void hash_table :: update_lru(paddr_t addr)
{
    hash_entry* entry = get_entry(addr);
    assert(entry != 0);

    hash_set *hs = my_sets[get_index (addr)];

    hs->update_lru(entry);
}

///////////////////////////////////////////////////////////////////////////////

// shash_table: Constructor
shash_table::shash_table(const char *nm, int sz, int asoc, int blok_sz,
                         int hit_t, int lookup_t, replacement_policy_t rp) {
  name = nm;
  size = sz;
  assoc = asoc;
  sets = ((sz) / (asoc * blok_sz));
  block_size = blok_sz;
  hit_time = hit_t;
  lookup_time = lookup_t;
  replacement_policy = rp;
  occupancy = 0;
  
  num_index_bits = (int)log2(sets);  
  num_offset_bits = (int)log2(block_size);
  int partition_bits = (int)log2(pagevault::ea_get_partition_size());  
  
  num_index_shift = num_offset_bits + partition_bits;
  
  offset_mask = (1 << num_offset_bits) - 1;
  index_mask = (paddr_t)((1 << num_index_bits) - 1) << num_index_shift;
  tag_mask = ~(index_mask | offset_mask);

  for (int i = 0; i < this->sets; ++i)
    my_sets[i] = new hash_set(this, assoc, i);
}

// shash_table: Constructor
shash_table::shash_table(cache_settings my_settings) {
  name = my_settings.name;
  size = my_settings.size;
  assoc = my_settings.assoc;
  sets = ((my_settings.size) / (my_settings.assoc * my_settings.block_size));
  block_size = my_settings.block_size;
  hit_time = my_settings.hit_time;
  lookup_time = my_settings.lookup_time;
  replacement_policy = (my_settings.replacement_policy);
  occupancy = 0;

  num_index_bits = (int)log2(sets);  
  num_offset_bits = (int)log2(block_size);
  int partition_bits = (int)log2(pagevault::ea_get_partition_size());  
  
  num_index_shift = num_offset_bits + partition_bits;
  
  offset_mask = (1 << num_offset_bits) - 1;
  index_mask = (paddr_t)((1 << num_index_bits) - 1) << num_index_shift;
  tag_mask = ~(index_mask | offset_mask);

  for (int i = 0; i < this->sets; ++i)
    my_sets[i] = new hash_set(this, assoc, i);
}
