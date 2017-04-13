#include "bmt.h"
#include "controller.h"

using namespace std;
using namespace manifold::kernel;

namespace manifold {
namespace pagevault {
namespace bmt {

//
// Under BMT, the hash tree only protects counter blocks.
// On read requests, BMT sends memory requests for the block and its MAC.
// Meanwhile, BMT computes a unique pad be using a 64-bit counter value
// stored in on-chip cache. On a miss in the counter cache, BMT sends a memory request 
// to fetch the counter from memory (counter blocks requests require hash tree validation)
// Once the user block arrives, it is decrypted using the unique pad and 
// authenticated using its computed hash together with the MAC returned from memory.
//
// On write requests, BMT computes a unique pad for the block using a 64-bit counter value 
// stored in on-chip cache. The counter value is incremented to produce a unique value.
// On a miss in the counter cache, BMT sends a memory requets to fetch the counter 
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
  // The following table describes the compute stages during a BMT read transaction
  //
  // Step 1
  //
  // Send the block read request to the memory system
  {EA_MEM_REQ,     EA_MEM_DEV,      0, 0, EA_USER_BLOCK | EA_CTR_BLOCK},      
  // Send memory request to fetch the MAC for the given user block 
  {EA_MAC_REQ,     EA_MAC_DEV,      0, 0, EA_USER_BLOCK},   
  // Generate a list of all the hash tree MACs associated with the given counter block
  {EA_HT_LIST,     EA_HTLIST_DEV,   0, 0, EA_CTR_BLOCK},
  // Lookup to the counter cache to generate a new counter value for this user block request
  {EA_CTR_GEN,     EA_CTR_DEV,      0, 0, EA_USER_BLOCK},
  // Counter blocks are not encrypted, the seed doesn't have to be unique
  {EA_SEED_GEN,    EA_SEED_DEV,     0, 0, EA_CTR_BLOCK},
  //
  // Step 2
  //
  // Once the MAC list is built, send memory request to fetch the hash tree MACs associated with the counter block
  {EA_HT_RD,       EA_HTMEM_DEV,    EA_HT_LIST, 0, EA_CTR_BLOCK},   
  // Once the counter is available, generate a unique seed using the block address and counter value
  {EA_SEED_GEN,    EA_SEED_DEV,     EA_CTR_GEN, 0, EA_USER_BLOCK},
  //
  // Step 3
  //
  // Once the block is returned and its seed is ready, generate a hash of the block returned from memory
  {EA_HASH_GEN,    EA_HASH_DEV,     EA_MEM_REQ | EA_SEED_GEN, 0, EA_USER_BLOCK | EA_CTR_BLOCK},  
  // Hash each HT parent node once all their child nodes are returned from memory
  {EA_HT_HASH_RD,  EA_HTHASH_DEV,   EA_HT_LIST, EA_HT_RD, EA_CTR_BLOCK},
  // Once the seed is generated, encrypt the block using AES
  {EA_PAD_GEN,     EA_AES_DEV,      EA_SEED_GEN, 0, EA_USER_BLOCK},  
  //
  // Step 4
  //
  // Once the hash is computed and the MAC is returned, authenticate the user block
  {EA_BLOCK_AUTH,  EA_AUTH_DEV,     EA_MAC_REQ | EA_HASH_GEN, 0, EA_USER_BLOCK},  
  // Once the hash is computed and the MAC is returned, authenticate the counter block
  {EA_BLOCK_AUTH,  EA_AUTH_DEV,     EA_HASH_GEN, EA_HT_RD, EA_CTR_BLOCK},
  // Authenticate each HT parent node once it has been hashed
  {EA_HT_AUTH,     EA_AUTH_DEV,     EA_HT_LIST, EA_HT_HASH_RD, EA_CTR_BLOCK},
  // Once the pad is generated, decrypt the block using the computed pad
  {EA_DECRYPT,     EA_XOR_DEV,      EA_MEM_REQ | EA_PAD_GEN, 0, EA_USER_BLOCK},  
  //
  // Step 5
  //
  // Counter blocks can early-commit once they are authenticated
  {EA_COMMIT0,     EA_COMMIT_DEV,   EA_BLOCK_AUTH, 0, EA_CTR_BLOCK},
  // Once the block is authenticated and decrypted, we are ready to commit the user block request  
  {EA_COMMIT0,     EA_COMMIT_DEV,   EA_BLOCK_AUTH | EA_DECRYPT, 0, EA_USER_BLOCK},
  //
  // Step 6
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
  {EA_CTR_GEN,     EA_CTR_DEV,      0, 0, EA_USER_BLOCK},    
  // Counter blocks are not encrypted, so the seed doesn't have to be unique
  {EA_SEED_GEN,    EA_SEED_DEV,     0, 0, EA_CTR_BLOCK},
  //
  // Step 2
  //
  // Once the MAC list is built, send memory request to fetch other enumerated hash tree MACs associated with the counter block
  {EA_HT_RD,       EA_HTMEM_DEV,    EA_HT_LIST, 0, EA_CTR_BLOCK},
  // Once the counter is available, generate a unique seed using the block address and counter value
  {EA_SEED_GEN,    EA_SEED_DEV,     EA_CTR_GEN, 0, EA_USER_BLOCK},
  //
  // Step 3
  //   
  // Once the counter block seed is available, generate its hash
  {EA_HASH_GEN,    EA_HASH_DEV,     EA_SEED_GEN, 0, EA_CTR_BLOCK},    
  // Hash each HT parent node once all their child nodes are returned from memory
  {EA_HT_HASH_RD,  EA_HTHASH_DEV,   EA_HT_LIST, EA_HT_RD, EA_CTR_BLOCK},
  // Once the seed is generated, encrypt the block using AES  
  {EA_PAD_GEN,     EA_AES_DEV,      EA_SEED_GEN, 0, EA_USER_BLOCK},
  //
  // Step 4
  //
  // Authenticate each HT parent node once it has been hashed
  {EA_HT_AUTH,     EA_AUTH_DEV,     EA_HT_LIST, EA_HT_HASH_RD, EA_CTR_BLOCK},
  // Once the seed is generated, encrypt the block using AES
  {EA_ENCRYPT,     EA_XOR_DEV,      EA_PAD_GEN, 0, EA_USER_BLOCK},
  //
  // Step 5
  //
  // Once the block is encrypted, generate its hash
  {EA_HASH_GEN,    EA_HASH_DEV,     EA_ENCRYPT, 0, EA_USER_BLOCK},
  //
  // Step 6
  //
  // Once the block's hash is computed, send the block to memory
  {EA_MEM_REQ,     EA_MEM_DEV,      EA_HASH_GEN, 0, EA_USER_BLOCK | EA_CTR_BLOCK},  
  // Once the block's hash is computed, send its MAC value to memory
  {EA_MAC_REQ,     EA_MAC_DEV,      EA_HASH_GEN, 0, EA_USER_BLOCK},  
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
};

EACore::EACore(Controller* ctrl)
  : EACoreBase(ctrl,
           std::vector<ea_stage_t>(sc_rd_stages, sc_rd_stages + __countof(sc_rd_stages)),
           std::vector<ea_stage_t>(sc_wb_stages, sc_wb_stages + __countof(sc_wb_stages))) {      
  config_t* config = get_config();
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
    //given: 
    //   number HT nodes = (ary * leafs - 1) / (ary - 1))
    //
    // BMT: (HT protects counter blocks)
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
    
    uint64_t num_macs = user_blocks;
    uint64_t mac_blocks  = __iceildiv(num_macs, macs_in_block);
    
    uint64_t num_ctrs = user_blocks;
    uint64_t ctr_blocks  = __iceildiv(num_ctrs, CTR_PER_BLOCK);
      
    // the hash tree only covers the counter blocks
    m_hashtree = new ktree_t(ctr_blocks, MTREE_ARY);
    
    // evaluate memory footprint
    uint64_t ht_nodes = m_hashtree->nodes;
    uint64_t ht_blocks = __iceildiv(ht_nodes - 1, macs_in_block); // remove root node
    uint64_t mem_blocks = user_blocks + mac_blocks + ctr_blocks + ht_blocks;
    m_stats.mem_footprint = mem_blocks << EA_LOG2_BLOCK_SIZE;
    assert(__iceildiv(m_stats.mem_footprint, 1<<20) <= config->mem_capacity); // MB comparison
    
    // create memory layout mapper 
    m_mem_layout = new mem_layout_t(user_blocks, 
                                    num_macs,
                                    user_blocks + mac_blocks, 
                                    num_ctrs,
                                    user_blocks + mac_blocks + ctr_blocks,
                                    ht_nodes,
                                    log2_macs_in_block);  
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
  
  // mac blocks driver
  m_devices[EA_MAC_DEV] = new EAMacReq(this, config->mc.entries, config->mc.assoc, config->mc.cycles);
  
  // hash tree driver
  m_devices[EA_HTMEM_DEV] = new EAHtReq(this);
  
  // counter cache
  m_devices[EA_CTR_DEV] = new EACtrGen(this, config->cc.entries, config->cc.assoc, config->cc.cycles);
  
  // hash tree MACs list generator
  m_devices[EA_HTLIST_DEV] = new EAHTListGen(this);
  
  // hash tree authentication
  m_devices[EA_AUTH_DEV] = new EAHtAuth(this);
  
  // end of transaction
  m_devices[EA_COMMIT_DEV] = new EACommit(this);
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
  uint32_t node_idx = this->get_hashtree()->node_from_leaf(ctr_block);
  return node_idx;
}

}
}
}
