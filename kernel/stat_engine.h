#ifndef MANIFOLD_KERNEL_STAT_ENGINE
#define MANIFOLD_KERNEL_STAT_ENGINE

#include <ostream>

using namespace std;

namespace manifold {
namespace kernel {

class Stat_engine {
public:
    Stat_engine();
    virtual ~Stat_engine(void);

    virtual void global_stat_merge(Stat_engine *) = 0;
    virtual void print_stats(ostream & out) = 0;
    virtual void clear_stats() = 0;

    //TODO remove start_warmup, end_warmup, save_samples, commit_stats, rollback_stats?
    virtual void start_warmup() = 0;
    virtual void end_warmup() = 0;
    virtual void save_samples() = 0;
};

} // namespace kernel
} // namespace manifold

#endif // MANIFOLD_KERNEL_STAT_ENGINE
