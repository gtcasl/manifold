#ifdef USE_QSIM

#include <assert.h>

#include "cache.h"
#include "pipeline.h"
#include "core.h"

using namespace std;
using namespace manifold;
using namespace spx;





// Cache Block
cache_block_t::cache_block_t(uint64_t Tag) :
    tag(Tag),
    dirty(false)
{
}

cache_block_t::~cache_block_t()
{
}

void cache_block_t::access(bool IsWrite)
{
    if(IsWrite) { dirty = true; }
}





// Cache Line
cache_line_t::cache_line_t(int Type, uint64_t Index, uint64_t Assoc, cache_table_t *CacheTable) :
    type(Type),
    index(Index),
    assoc(Assoc),
    cache_table(CacheTable)
{
    cache_block.reserve(assoc);
}

cache_line_t::~cache_line_t()
{
    for(uint64_t i = 0; i < cache_block.size(); i++) {
        delete cache_block[i];
    }
    cache_block.clear();
}

cache_block_t* cache_line_t::access(uint64_t Tag, bool IsWrite)
{
#ifdef LIBEI
    switch(type) {
        case SPX_INST_CACHE: {
            cache_table->pipeline->counter.inst_cache_miss_buffer.search++;
            break;
        }
        case SPX_DATA_CACHE: {
            cache_table->pipeline->counter.data_cache_miss_buffer.search++;
        }
        default: { break; }        
    }
#endif

    for(vector<cache_block_t*>::iterator it = cache_block.begin(); it != cache_block.end(); it++) {
        cache_block_t *block = *it;
        if(block->tag == Tag) {
            // Hit
            block->access(IsWrite);
            
            // LRU
            cache_block.erase(it);
            cache_block.push_back(block);
            
#ifdef LIBEI
            switch(type) {
                case SPX_INST_CACHE: {
                    if(IsWrite) { cache_table->pipeline->counter.inst_cache.write++; }
                    else { cache_table->pipeline->counter.inst_cache.read++; }
                    break;
                }
                case SPX_DATA_CACHE: {
                    if(IsWrite) { cache_table->pipeline->counter.data_cache.write++; }
                    else { cache_table->pipeline->counter.data_cache.read++; }
                    break;
                }
                case SPX_INST_TLB: {
                    cache_table->pipeline->counter.inst_tlb.read++;
                    break;
                }
                case SPX_DATA_TLB: {
                    cache_table->pipeline->counter.data_tlb.read++;
                    break;
                }
                case SPX_L2_TLB: {
                    cache_table->pipeline->counter.l2_tlb.read++;
                    break;
                }
                default: { break; }
            }
#endif
            return block;
        }
    }
    
#ifdef LIBEI
    switch(type) {
        case SPX_INST_CACHE: {
            cache_table->pipeline->counter.inst_cache.read_tag++;
            
            // Miss buffer is scheduled
            cache_table->pipeline->counter.inst_cache_miss_buffer.write++;
            cache_table->pipeline->counter.inst_cache_miss_buffer.search++;
            cache_table->pipeline->counter.inst_cache_miss_buffer.read++;
            
            // Assume some sort of prefetching is performed by a cache miss
            cache_table->pipeline->counter.inst_cache_prefetch_buffer.search++;
            cache_table->pipeline->counter.inst_cache_prefetch_buffer.write++;
            cache_table->pipeline->counter.inst_cache_prefetch_buffer.search++;
            cache_table->pipeline->counter.inst_cache_prefetch_buffer.read++;
            break;
        }
        case SPX_DATA_CACHE: {
            cache_table->pipeline->counter.data_cache.read_tag++;
            
            // Miss buffer is scheduled
            cache_table->pipeline->counter.data_cache_miss_buffer.write++;
            cache_table->pipeline->counter.data_cache_miss_buffer.search++;
            cache_table->pipeline->counter.data_cache_miss_buffer.read++;
            
            // Assume some sort of prefetching is performed by a cache miss
            cache_table->pipeline->counter.data_cache_prefetch_buffer.search++;
            cache_table->pipeline->counter.data_cache_prefetch_buffer.write++;
            cache_table->pipeline->counter.data_cache_prefetch_buffer.search++;
            cache_table->pipeline->counter.data_cache_prefetch_buffer.read++;
            break;
        }
        case SPX_INST_TLB: {
            // TLB miss
            cache_table->pipeline->counter.inst_tlb.read_tag++;
            break;
        }
        case SPX_DATA_TLB: {
            // TLB miss
            cache_table->pipeline->counter.data_tlb.read_tag++;
            break;
        }
        case SPX_L2_TLB: {
            // TLB miss
            cache_table->pipeline->counter.l2_tlb.read_tag++;
            break;
        }
        default: { break; }        
    }
#endif
    
    // Miss
    return NULL;
}

cache_block_t* cache_line_t::update(cache_block_t *Block)
{
    cache_block_t *victim_block = NULL;
    if(cache_block.size() >= assoc) {
        // Evict the victim block
        victim_block = *cache_block.begin();
        cache_block.erase(cache_block.begin());
    }

    // Insert a new cache block        
    cache_block.push_back(Block);
    assert(cache_block.size() <= assoc);
    
#ifdef LIBEI
    switch(type) {
        case SPX_INST_CACHE: {
            cache_table->pipeline->counter.inst_cache.write++;
            if(victim_block) cache_table->pipeline->counter.inst_cache.read_tag++; // find and invalidate victim block
            break;
        }
        case SPX_DATA_CACHE: {
            cache_table->pipeline->counter.data_cache.write++;
            if(victim_block) {
                if(victim_block->dirty) { 
                    cache_table->pipeline->counter.data_cache.read++;
                    cache_table->pipeline->counter.data_cache_writeback_buffer.search++;
                    cache_table->pipeline->counter.data_cache_writeback_buffer.write++;
                    cache_table->pipeline->counter.data_cache_writeback_buffer.read++;
                }
                else { 
                    cache_table->pipeline->counter.data_cache.read_tag++;
                }
            }
            break;
        }
        case SPX_INST_TLB: {
            cache_table->pipeline->counter.inst_tlb.write++;
            if(victim_block) cache_table->pipeline->counter.inst_tlb.read_tag++; // find and invalidate victim block
            break;
        }
        case SPX_DATA_TLB: {
            cache_table->pipeline->counter.data_tlb.write++;
            if(victim_block) cache_table->pipeline->counter.data_tlb.read_tag++; // find and invalidate victim block
            break;
        }
        case SPX_L2_TLB: {
            cache_table->pipeline->counter.l2_tlb.write++;
            if(victim_block) cache_table->pipeline->counter.l2_tlb.read_tag++; // find and invalidate victim block
            break;
        }
        default: { break; }        
    }
#endif
    
    return victim_block;
}





// Cache Table
cache_table_t::cache_table_t(int Type, uint64_t CacheLevel, cache_config_t CacheConfig, pipeline_t *Pipeline) :
    type(Type),
    cache_level(CacheLevel),
    config(CacheConfig),
    next_level_cache(NULL),
    pipeline(Pipeline)
{
    num_sets = config.cache_size/config.block_size/config.assoc;
    
    index_bits = log2(num_sets);
    assert(((uint64_t)0x1<<index_bits) == num_sets);
    
    offset_bits = log2(config.block_size);
    assert(((uint64_t)0x1<<offset_bits) == config.block_size);
    
    tag_mask = ~0x0;
    tag_mask = tag_mask << (offset_bits+index_bits);
    
    index_mask = ~0x0;
    index_mask = index_mask << offset_bits;
    index_mask = index_mask & ~tag_mask;
    
    offset_mask = ~0x0;
    offset_mask = offset_mask << offset_bits;
    
    cache_line.reserve(num_sets);
    for(uint64_t i = 0; i < num_sets; i++) {
        cache_line[i] = new cache_line_t(type,i,config.assoc,this);
    }
}

cache_table_t::~cache_table_t()
{
    for(uint64_t i = 0; i < num_sets; i++) {
        delete cache_line[i];
    }
    cache_line.clear();
}

cache_block_t* cache_table_t::access(uint64_t Address, bool IsWrite)
{
    uint64_t block_tag = Address & offset_mask;
    uint64_t index = (block_tag & index_mask) >> offset_bits;
    assert(index < num_sets);
    
    cache_block_t *block = cache_line[index]->access(block_tag,IsWrite);
    
    if(block == NULL) { // Cache Miss
        if(next_level_cache) {
            block = next_level_cache->access(Address,IsWrite);
            cache_block_t *victim_block = cache_line[index]->update(block);
            
            switch(type) {
                case SPX_INST_CACHE: {
                    if(victim_block&&victim_block->dirty) next_level_cache->writeback(victim_block);
                    break;
                }
                case SPX_DATA_CACHE: {
                    if(victim_block&&victim_block->dirty) next_level_cache->writeback(victim_block);
                    break;   
                }
                default: { break; }
            }
        }
        else { // This is the last level
            block = new cache_block_t(block_tag);
            cache_block_t *victim_block = cache_line[index]->update(block);
            delete victim_block;
            return block;
        }
    }

    return block;
}

void cache_table_t::writeback(cache_block_t *Block)
{
    assert(Block->dirty == true);
    // Nothing to do unless it's L2 or lower level cache
}

void cache_table_t::add_next_level(cache_table_t *NextLevelCache)
{
    next_level_cache = NextLevelCache;
}

#endif // USE_QSIM
