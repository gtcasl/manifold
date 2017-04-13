#include "bmt.h"
#include "pmt.h"
#include "controller.h"

using namespace std;
using namespace manifold::kernel;

namespace manifold {

namespace mcp_cache_namespace {
  uint64_t l2_get_page_onchip_mask(uint64_t addr, int nodeid);
}

namespace pagevault {

pmt::EACore* g_PMTCore = nullptr;

void ea_get_partition_list(std::vector<uint64_t>& out, uint64_t addr) {
  if (g_PMTCore) {
    g_PMTCore->get_partition_list(out, addr);
  } else {
    out.push_back(addr);
  }
}

namespace pmt {

//
// Under PMT, the hash tree only protects counter blocks.
// On read requests, PMT sends memory requests for the block and its MAC.
// Meanwhile, BMT computes a unique pad be using a 64-bit counter value
// stored in on-chip cache. On a miss in the counter cache, BMT sends a memory request 
// to fetch the counter from memory (counter blocks requests require hash tree validation)
// Once the user block arrives, it is decrypted using the unique pad and 
// authenticated using its computed hash together with the MAC returned from memory.
//
// On write requests, PMT computes a unique pad for the block using a 64-bit counter value 
// stored in on-chip cache. The counter value is incremented to produce a unique value.
// On a miss in the counter cache, PMT sends a memory requets to fetch the counter 
// from memory (counter blocks requests require hash tree validation)
// When the unique pad is ready, the block is encrypted and sent to memory
// Also, the block's hash is computed and sent to memory.
//
// Note: the block hash generation uses the block seed (ctr+addr) 
//       a generative counter is used to compute the hash the non-leaf nodes of the mash tree 
//       this allows concurrent hashing of parent nodes when their child-nodes are available
//

// EA stages in execution order
static const ea_stage_t sc_rd_stages[] = {
  //
  // The following table describes the compute stages during a PMT read transaction
  //
  // Step 1
  //   
  // generate partition blocks requests
  {EA_PMT_REQ,     EA_PART_DEV,     0, 0, EA_USER_BLOCK}, 
  // Lookup to the counter cache to generate a new counter value for this block request
  {EA_CTR_GEN,     EA_CTR_DEV,      0, 0, EA_USER_BLOCK},
  // Send the block read request to the memory system
  {EA_MEM_REQ,     EA_MEM_DEV,      0, 0, EA_CTR_BLOCK},     
  // Generate a list of all the hash tree MACs associated with the given counter block
  {EA_HT_LIST,     EA_HTLIST_DEV,   0, 0, EA_CTR_BLOCK},     
  // Counter blocks are not encrypted, the seed doesn't have to be unique
  {EA_SEED_GEN,    EA_SEED_DEV,     0, 0, EA_CTR_BLOCK},
  //  
  // Step 2
  //      
  // Once the MAC list is built, send memory request to fetch the hash tree MACs associated with the counter block
  {EA_HT_RD,       EA_HTMEM_DEV,    EA_HT_LIST, 0, EA_CTR_BLOCK},   
  // Lookup to the partition MAC cache, if not found send request to fetch it from memory
  {EA_MAC_REQ,     EA_MAC_DEV,      EA_CTR_GEN, 0, EA_USER_BLOCK},      
  // Once the counter is available, generate a unique seed using the block address and counter value
  {EA_SEED_GEN,    EA_SEED_DEV,     EA_CTR_GEN, 0, EA_USER_BLOCK},  
  //
  // Step 3
  //
  // Send partition blocks read requests to the memory system
  {EA_MEM_REQ,     EA_MEM_DEV,      EA_PMT_REQ, 0, EA_USER_BLOCK},    
  // Hash each HT parent node once all their child nodes are returned from memory
  {EA_HT_HASH_RD,  EA_HTHASH_DEV,   EA_HT_LIST, EA_HT_RD, EA_CTR_BLOCK},    
  // Once the counter is available, generate a unique seed using the block address and counter value
  {EA_PAD_GEN,     EA_AES_DEV,      EA_SEED_GEN, 0, EA_USER_BLOCK},  
  //
  // Step 4
  //
  // Once the block is returned and its seed is ready, generate a hash of the block returned from memory
  {EA_HASH_GEN,    EA_HASH_DEV,     EA_MEM_REQ | EA_SEED_GEN, 0, EA_USER_BLOCK | EA_CTR_BLOCK},  
  // Authenticate each HT parent node once it has been hashed
  {EA_HT_AUTH,     EA_AUTH_DEV,     EA_HT_LIST, EA_HT_HASH_RD, EA_CTR_BLOCK},
  // Once the pad is generated, decrypt the block using the computed pad
  {EA_DECRYPT,     EA_XOR_DEV,      EA_MEM_REQ | EA_PAD_GEN, 0, EA_USER_BLOCK},   
  //
  // Step 5
  //
  // Once the hash is computed and the page MAC has arrived, authenticate the user block
  {EA_BLOCK_AUTH,  EA_AUTH_DEV,     EA_MAC_REQ | EA_HASH_GEN, 0, EA_USER_BLOCK},  
  // Once the hash is computed and the MAC is returned, authenticate the counter block
  {EA_BLOCK_AUTH,  EA_AUTH_DEV,     EA_HASH_GEN, EA_HT_RD, EA_CTR_BLOCK},  
  //
  // Step 6
  //
  // Counter blocks can early-commit once they are authenticated
  {EA_COMMIT0,     EA_COMMIT_DEV,   EA_BLOCK_AUTH, 0, EA_CTR_BLOCK},
  // Once the block is authenticated and decrypted, we are ready to commit the user block request  
  {EA_COMMIT0,     EA_COMMIT_DEV,   EA_BLOCK_AUTH | EA_DECRYPT, 0, EA_USER_BLOCK},
  //
  // Step 7
  //    
  // Once the hash tree is authenticated, we are ready to commit the counter block request
  {EA_COMMIT,      EA_COMMIT_DEV,   EA_COMMIT0 | EA_HT_AUTH, 0, EA_CTR_BLOCK},
  // Commit the user block request
  {EA_COMMIT,      EA_COMMIT_DEV,   EA_COMMIT0, 0, EA_USER_BLOCK}, 
};

static const ea_stage_t sc_wb_stages[] = {
  //
    // The following table describes the compute stages during a BMT write transaction
    //
    // Step 1
    //
    // Generate a list of all the hash tree MACs associated with the given counter block 
    {EA_HT_LIST,     EA_HTLIST_DEV,   0, 0, EA_CTR_BLOCK}, 
    // Lookup to the counter cache to generate a new counter value for this user block request
    {EA_CTR_GEN,     EA_CTR_DEV,      0, 0, EA_USER_BLOCK | EA_EVICT_BLOCK},    
    // Counter blocks are not encrypted, so the seed doesn't have to be unique
    {EA_SEED_GEN,    EA_SEED_DEV,     0, 0, EA_CTR_BLOCK},
    //
    // Step 2
    //
    // Once the MAC list is built, send memory request to fetch other enumerated hash tree MACs associated with the counter block
    {EA_HT_RD,       EA_HTMEM_DEV,    EA_HT_LIST, 0, EA_CTR_BLOCK},
    // Once the counter is available, generate a unique seed using the block address and counter value
    {EA_SEED_GEN,    EA_SEED_DEV,     EA_CTR_GEN, 0, EA_USER_BLOCK | EA_EVICT_BLOCK},
    //
    // Step 3
    //   
    // Once the counter block seed is available, generate its hash
    {EA_HASH_GEN,    EA_HASH_DEV,     EA_SEED_GEN, 0, EA_CTR_BLOCK},    
    // Hash each HT parent node once all their child nodes are returned from memory
    {EA_HT_HASH_RD,  EA_HTHASH_DEV,   EA_HT_LIST, EA_HT_RD, EA_CTR_BLOCK},
    // Once the seed is generated, encrypt the block using AES  
    {EA_PAD_GEN,     EA_AES_DEV,      EA_SEED_GEN, 0, EA_USER_BLOCK | EA_EVICT_BLOCK},
    //
    // Step 4
    //
    // Authenticate each HT parent node once it has been hashed
    {EA_HT_AUTH,     EA_AUTH_DEV,     EA_HT_LIST, EA_HT_HASH_RD, EA_CTR_BLOCK},
    // Once the seed is generated, encrypt the block using AES
    {EA_ENCRYPT,     EA_XOR_DEV,      EA_PAD_GEN, 0, EA_USER_BLOCK | EA_EVICT_BLOCK},
    //
    // Step 5
    //
    // Once the block is encrypted and the page MAC has arrived, generate its hash
    {EA_HASH_GEN,    EA_HASH_DEV,     EA_ENCRYPT, 0, EA_USER_BLOCK | EA_EVICT_BLOCK},
    //
    // Step 6
    //
    // Once the block's hash is computed, send the block to memory
    {EA_MEM_REQ,     EA_MEM_DEV,      EA_HASH_GEN, 0, EA_USER_BLOCK | EA_CTR_BLOCK},  
    // Once the block's hash is computed, send its MAC value to memory
    {EA_MAC_REQ,     EA_MAC_DEV,      EA_HASH_GEN, 0, EA_USER_BLOCK | EA_EVICT_BLOCK},   
    // Hash each HT parent node after all children are authenticated
    {EA_HT_HASH_WR,  EA_HTHASH_DEV,   EA_HT_LIST | EA_HASH_GEN, EA_HT_AUTH, EA_CTR_BLOCK},  
    //
    // Step 7
    //  
    // Send each HT parent node's MAC to memory after their hash is updated
    {EA_HT_WR,       EA_HTMEM_DEV,    EA_HASH_GEN, EA_HT_RD | EA_HT_HASH_WR, EA_CTR_BLOCK},   
    // We can early-commit blocks once they are written back to memory
    {EA_COMMIT0,     EA_COMMIT_DEV,   EA_MEM_REQ, 0, EA_USER_BLOCK | EA_CTR_BLOCK}, 
    //
    // Step 8
    //  
    // Once the counter block is written and the hash tree is updated, we can commit the request 
    {EA_COMMIT,      EA_COMMIT_DEV,   EA_COMMIT0 | EA_HT_WR, 0, EA_CTR_BLOCK},  
    // Once the user block is written and its MAC updated, we can commit the request 
    {EA_COMMIT,      EA_COMMIT_DEV,   EA_COMMIT0 | EA_MAC_REQ, 0, EA_USER_BLOCK},  
    // Commit evicted blocks once the MAC is updated
    {EA_COMMIT,      EA_COMMIT_DEV,   EA_MAC_REQ, 0, EA_EVICT_BLOCK}, 
};

EACore::EACore(Controller* ctrl)
  : EACoreBase(ctrl,
           std::vector<ea_stage_t>(sc_rd_stages, sc_rd_stages + __countof(sc_rd_stages)),
           std::vector<ea_stage_t>(sc_wb_stages, sc_wb_stages + __countof(sc_wb_stages))) {
  config_t* config = get_config();
  m_log2_macs_per_page = fastLog2(EA_BLOCKS_PER_PAGE / config->pmt_size);
  m_dynamic_partitioning = config->pmt_mode == "dynamic";
  {
    //
    // configuration: 
    // + 4GB DRAM
    // + 64B per blocks
    // + 8-bit counter => 64 counters per block
    // + 128-bit MAC   => 4 MACs per block
    // + 4-ary hash tree
    //
    // user data = 2^32 / 2^6 = 2^26 blocks
    // ctr data  = 2^26 / 2^6 = 2^20 blocks
    // total blocks = 68,157,440 blocks
    //
    // given: 
    //   number HT nodes = (ary * leafs - 1) / (ary - 1))
    //
    // PMT: (HT protects counter blocks + page level MAC)
    // user data macs = 2^26 / 4 = 2^24 blocks
    // number HT nodes = 1,398,101 nodes
    // HT storage = 1M/4 = 349,525 blocks
    // Total storage = 85,284,181 blocks = 5.4GB
    // Storage overhead = 27% 
    //  
    
    uint8_t log2_macs_in_block = log2((EA_BLOCK_SIZE * 8) / config->mac_bits);
    uint32_t macs_in_block = 1 << log2_macs_in_block;
     
    assert((config->sys_mem % EA_BLOCK_SIZE) == 0);    
    uint64_t user_blocks = __iceildiv(config->sys_mem, EA_BLOCK_SIZE);
    assert((user_blocks % EA_BLOCKS_PER_PAGE) == 0);
    
    uint64_t log2_blocks_per_mac = 
        m_dynamic_partitioning ? 0 : (EA_LOG2_BLOCKS_PER_PAGE - m_log2_macs_per_page);      
    uint64_t num_macs = user_blocks >> log2_blocks_per_mac;        
    uint64_t mac_blocks = __iceildiv(num_macs, macs_in_block);
       
    uint64_t num_ctrs = user_blocks;
    uint64_t ctr_blocks = __iceildiv(num_ctrs, CTR_PER_BLOCK);
      
    // the hash tree covers the counter blocks only
    m_hashtree = new ktree_t(ctr_blocks, MTREE_ARY);
    
    // evaluate memory footprint
    uint64_t ht_nodes = m_hashtree->nodes;
    uint64_t ht_blocks = __iceildiv(ht_nodes - 1, macs_in_block); // remove root node
    uint64_t mem_blocks = user_blocks + ctr_blocks + ht_blocks + mac_blocks;
    m_stats.mem_footprint = mem_blocks << EA_LOG2_BLOCK_SIZE;
    assert(__iceildiv(m_stats.mem_footprint, 1<<20) <= config->mem_capacity); // MB comparison
    
    // create memory layout mapper 
    m_mem_layout = new mem_layout_t(user_blocks + ctr_blocks + ht_blocks, 
                                    num_macs,
                                    user_blocks, 
                                    num_ctrs,
                                    user_blocks + ctr_blocks,
                                    ht_nodes,
                                    log2_macs_in_block,
                                    log2_blocks_per_mac);  
    
  }  
  
  // AES engine
  m_devices[EA_AES_DEV] = new EADevice("AES", config->aes.cycles, config->aes.ports, AES_CALLS_PER_BLOCK);
    
  // SHA-1 engine
  m_devices[EA_HASH_DEV] = new EADevice("HashGen", config->sha1.cycles, config->sha1.ports, HASH_CALLS_PER_BLOCK);
  m_devices[EA_HTHASH_DEV] = new HtHashGen(this, config->sha1.cycles, config->sha1.ports, HASH_CALLS_PER_BLOCK);
  
  // encrypt/decrypt XOR
  m_devices[EA_XOR_DEV] = new EADevice0();
  
  // seed generator
  m_devices[EA_SEED_DEV] = new EADevice0();
  
  // user blocks driver
  m_devices[EA_MEM_DEV] = new EAMemReq(this);
  
  // counter cache
  m_devices[EA_CTR_DEV] = new EACtrGen(this, config->cc.entries, config->cc.assoc, config->cc.cycles);
  
  // mac blocks driver
  m_devices[EA_MAC_DEV] = new EAMacReq(this, config->mc.entries, config->mc.assoc, config->mc.cycles);
  
  // hash tree driver
  m_devices[EA_HTMEM_DEV] = new EAHtReq(this);
  
  // partition device
  m_devices[EA_PART_DEV] = new EAPartGen(this);
  
  // hash tree MACs list generator
  m_devices[EA_HTLIST_DEV] = new EAHTListGen(this);
  
  // hash tree authentication
  m_devices[EA_AUTH_DEV] = new EAHtAuth(this);
  
  // end of transaction
  m_devices[EA_COMMIT_DEV] = new EACommit(this);
  
  // set global handle
  g_PMTCore = this;
}

EACore::~EACore() {
  //--
}

uint32_t EACore::get_hashtree_nodeidx(ea_mrq_t* mrq) {
  // the counter index is the same as the user block index
  // in BMT, only counter blocks are protected by HT
  assert(mrq->dt_type == EA_CTR_BLOCK);
  uint64_t usr_block = mrq->parent->mem_addr >> EA_LOG2_BLOCK_SIZE;
  uint64_t ctr_block = usr_block >> LOG2_CTR_PER_BLOCK;
  uint32_t node_idx  = this->get_hashtree()->node_from_leaf(ctr_block);
  return node_idx;
}

uint64_t EACore::get_page_onchip_mask(uint64_t addr, int nodeid) {
  return mcp_cache_namespace::l2_get_page_onchip_mask(addr, nodeid);
}

uint32_t EACore::get_log2_macs_per_page(uint64_t addr) {
  return reinterpret_cast<EACtrGen*>(m_devices[EA_CTR_DEV])->get_log2_macs_per_page(addr);
}

void EACore::get_partition_list(std::vector<uint64_t>& out, uint64_t addr) {
  // generate the partition list for a given block address
  uint64_t block_addr = addr >> EA_LOG2_BLOCK_SIZE;   
  uint32_t log2_chunk_size = EA_LOG2_BLOCKS_PER_PAGE - this->get_log2_macs_per_page(addr);
  uint64_t chunk_addr = block_addr >> log2_chunk_size;
  uint32_t chunk_size = 1 << log2_chunk_size;
 
  for (uint32_t i = 0; i < chunk_size; ++i) {
    uint64_t _addr = ((chunk_addr << log2_chunk_size) + i) << EA_LOG2_BLOCK_SIZE;
    out.push_back(_addr);
  }
}

int EACore::submit_request(
    uint64_t addr, 
    bool is_read, 
    uint32_t onchip_mask,
    EA_BLOCK_TYPE dt,
    int src_nodeid,
    ea_mrq_t* parent,
    IReqCompleteCB* cb) {
  
  // limit per cyle access to EA queue
  if (m_ea_bus_arbitration)
    return -1;
  m_ea_bus_arbitration = true;
    
  if (is_read && dt == EA_USER_BLOCK && parent == nullptr) {
    uint32_t log2_part_size = EA_LOG2_BLOCKS_PER_PAGE - this->get_log2_macs_per_page(addr);
    uint64_t block_addr = addr >> EA_LOG2_BLOCK_SIZE;
    uint64_t part_addr = block_addr >> log2_part_size;
    
    // lookup existing request to same partition for prefech requests
    if (0 != m_pending_rd_parts.count(part_addr)) {
      assert(onchip_mask = EA_PREFETCH_MASK); // ensure prefetch request
      return m_pending_rd_parts[part_addr];
    }
    
    // A prefetch request with no pending partition
    // The request came too late
    if (EA_PREFETCH_MASK == onchip_mask)
      return -1;
  }  
    
  // ensure that the request queue is not full
  if (m_mrqs.size() >= m_queue_depth) {
    ++m_stats.queue_stalls;
    return -1;
  }
      
  // create a new request tag
  int tag = this->allocate_tag();
  
  if (is_read && dt == EA_USER_BLOCK && parent == nullptr) {
    uint32_t log2_part_size = EA_LOG2_BLOCKS_PER_PAGE - this->get_log2_macs_per_page(addr);
    uint64_t block_addr = addr >> EA_LOG2_BLOCK_SIZE;
    uint64_t part_addr = block_addr >> log2_part_size;
    
    // register the partition
    m_pending_rd_parts[part_addr] = tag;
  }

  // create a new reauest
  if (parent)
    DBG(2, "EA: %s request for %s block 0x%lx from parent#%d (req#%d)\n", toRWString(is_read), toString(dt), addr, parent->tag, tag);
  else
    DBG(2, "EA: %s request for %s block 0x%lx (req#%d)\n", toRWString(is_read), toString(dt), addr, tag);
  ea_mrq_t* const mrq = new ea_mrq_t(addr, tag, dt, is_read, onchip_mask, src_nodeid, cb, parent);
  if (parent) { 
    // insert the request right before its parent
    auto iter = std::find(m_mrqs.begin(), m_mrqs.end(), parent);
    m_mrqs.insert(iter, mrq);
  } else {    
    m_mrqs.push_back(mrq);
  }
  
  // udpate stats
  if (m_mrqs.size() > m_stats.max_queue_size) {
    m_stats.max_queue_size = m_mrqs.size();
  }
  
  return tag;
}

void EACore::release(ea_mrq_t* mrq) {
  DBG(2, "EA: committed request req#%d\n", mrq->tag);
   
  // free the partition entry
  if (mrq->is_read && mrq->dt_type == EA_USER_BLOCK && mrq->parent == nullptr) {    
    uint32_t log2_part_size = EA_LOG2_BLOCKS_PER_PAGE - this->get_log2_macs_per_page(mrq->mem_addr);
    uint64_t block_addr = mrq->mem_addr >> EA_LOG2_BLOCK_SIZE;
    uint64_t part_addr = block_addr >> log2_part_size;
    size_t removed = m_pending_rd_parts.erase(part_addr);
    assert(removed != 0);
  }
  
  // free the request
  auto iter = std::find(m_mrqs.begin(), m_mrqs.end(), mrq);
  assert(iter != m_mrqs.end());
  m_mrqs.erase(iter);
  mrq->release();
}

void EACore::print_stats(std::ostream& out) {
  EACoreBase::print_stats(out);
}

///////////////////////////////////////////////////////////////////////////////

EAPartGen::EAPartGen(EACore* eacore)
  : m_eacore(eacore)
{}

bool EAPartGen::tick(ea_mrq_t* mrq, EA_STAGE stage) { 
  assert(stage == EA_PMT_REQ);  
  assert(mrq->is_read);  
  
  uint32_t log2_part_size = EA_LOG2_BLOCKS_PER_PAGE - m_eacore->get_log2_macs_per_page(mrq->mem_addr);
  uint32_t part_size  = 1 << log2_part_size;
  uint64_t block_addr = mrq->mem_addr >> EA_LOG2_BLOCK_SIZE;
  uint64_t part_addr  = block_addr >> log2_part_size;
  
  for (int i = 0; i < part_size; ++i) {
    uint64_t _block_addr = ((part_addr << log2_part_size) + i);
    uint64_t _addr = _block_addr << EA_LOG2_BLOCK_SIZE;
    // only fetch offchip blocks
    if (0 == ((mrq->onchip_mask >> i) & 0x1)) {
      mrq->part_list.push_back(_addr);
    }
  } 
  
  // terminate execution
  mrq->stages_pending &= ~stage;
  mrq->stages_completed |= stage;  
  DBG(3, "*** stage %s completed for req#%d\n", toString(stage), mrq->tag);
  return true;
}

}
}
}
