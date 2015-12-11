#ifndef MESI_ENUM_H
#define MESI_ENUM_H

/** Cache states.  */

namespace manifold {
namespace mcp_cache_namespace {

typedef enum {
    MESI_C_I = 1,
    MESI_C_E,
    MESI_C_S,
    MESI_C_M,

    //following are transient states
    MESI_C_IE,
    MESI_C_EI,
    MESI_C_MI,
    MESI_C_SE,
    MESI_C_SIE, //transiting from S to E
} MESI_client_state_t;

typedef enum {
    MESI_MNG_I = 1,
    MESI_MNG_E,
    MESI_MNG_S,

    MESI_MNG_IE,
    MESI_MNG_EE,        /** Exclusive state transfer. */
    MESI_MNG_ES,
    MESI_MNG_SS,
    MESI_MNG_SIE,
    MESI_MNG_EI_PUT,     /** EI from owner client eviction eviction. */
    MESI_MNG_EI_EVICT,        /** EI from manager eviction action. */
    MESI_MNG_SI_EVICT,
} MESI_manager_state_t;

typedef enum {
    /** Client to Manager messages*/
    MESI_CM_I_to_E,
    MESI_CM_I_to_S,
    MESI_CM_E_to_I,
    MESI_CM_M_to_I,
    MESI_CM_UNBLOCK_I,
    MESI_CM_UNBLOCK_I_DIRTY,
    MESI_CM_UNBLOCK_E,   /** optional to differentiate between the kinds of unblocks. */
    MESI_CM_UNBLOCK_S,   /** optional to differentiate between the kinds of unblocks. */
    MESI_CM_CLEAN,
    MESI_CM_WRITEBACK,

    /** Manager to Client messages */
    MESI_MC_GRANT_E_DATA,
    MESI_MC_GRANT_S_DATA,
    MESI_MC_GRANT_I,     /** paired with a client PUT action. */
    MESI_MC_FWD_E,
    MESI_MC_FWD_S,
    MESI_MC_DEMAND_I,    /** paired with a manager eviction; demaning the client to remove its copy. */

    /** Client to Client messages*/
    MESI_CC_E_DATA,
    MESI_CC_M_DATA,
    MESI_CC_S_DATA,

    GET_E,
    GET_S,
    GET_EVICT,
} MESI_messages_t;

}
}
#endif
