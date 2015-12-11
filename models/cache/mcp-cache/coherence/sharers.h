#ifndef MANIFOLD_MCP_CACHE_SHARERS_H
#define MANIFOLD_MCP_CACHE_SHARERS_H

#include <vector>
#include <stddef.h> //for size_t

namespace manifold {
namespace mcp_cache_namespace {

// bitsets class that can shrink and grow I didn't want to add a dependancy on boost.
class sharers
{
	public:
		sharers(size_t size = 0);
		~sharers();
		void set(int index);
		void reset(int index);
		bool get(int index);
		void clear();
		int count() const;
		int size() const;
		void ones(std::vector<int>& ret);

#ifdef MCP_CACHE_UTEST
	public:
#else
	private:
#endif
		std::vector<bool> data; //true means the cache represented by the index is a sharer.
};

}
}
#endif
