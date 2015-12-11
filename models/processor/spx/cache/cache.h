#ifdef USE_QSIM

#ifndef __SPX_CACHE_H__
#define __SPX_CACHE_H__

#include <stdint.h>
#include <vector>

namespace manifold {
namespace spx {

class pipeline_t; 

enum SPX_CACHE_TYPE {
    SPX_INST_CACHE = 0,
    SPX_DATA_CACHE,
    SPX_L2_CACHE,
    SPX_L3_CACHE,
    SPX_INST_TLB,
    SPX_DATA_TLB,
    SPX_L2_TLB
};





class cache_config_t
{
public:
    cache_config_t() : cache_size(0), assoc(0), block_size(0) {}
    ~cache_config_t() {}
    
    uint64_t cache_size;
    uint64_t assoc;
    uint64_t block_size;
};





class cache_block_t
{
public:
    cache_block_t(uint64_t Tag);
    ~cache_block_t();
    
    void access(bool IsWrite);

    uint64_t tag;
    bool dirty;    
};





class cache_table_t;
class cache_line_t
{
public:
    cache_line_t(int Type, uint64_t Index, uint64_t Assoc, cache_table_t *CacheTable);
    ~cache_line_t();
    
    cache_block_t* access(uint64_t Tag, bool IsWrite);
    cache_block_t* update(cache_block_t *block);

    int type;    
private:
    std::vector<cache_block_t*> cache_block;
    uint64_t index;
    uint64_t assoc;
    cache_table_t *cache_table;
};





class cache_table_t
{
public:
    cache_table_t(int Type, uint64_t CacheLevel, cache_config_t CacheConfig, pipeline_t *Pipeline);
    ~cache_table_t();
    
    cache_block_t* access(uint64_t Address, bool IsWrite);
    void writeback(cache_block_t *Block);
    void add_next_level(cache_table_t *NextLevelCache);

private:
    int type;
    uint64_t cache_level;
    cache_config_t config;
    
    uint64_t num_sets;
    
    uint64_t index_bits;
    uint64_t offset_bits;
    
    uint64_t tag_mask;
    uint64_t index_mask;
    uint64_t offset_mask;

    std::vector<cache_line_t*> cache_line;
    cache_table_t *next_level_cache;
    pipeline_t *pipeline;
    friend class cache_line_t;
};

} // namespace spx
} // namespace manifold

#endif

#endif // USE_QSIM
