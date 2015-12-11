#include "sharers.h"

namespace manifold {
namespace mcp_cache_namespace {


/** @brief sharers
  *
  * @todo: document this function
  */
sharers::sharers(size_t size) : data(size)
{

}

/** @brief ~sharers
  *
  * @todo: document this function
  */
sharers::~sharers()
{

}

/** @brief get
  *
  * Get the status of a cache represented by the given index.
  */
bool sharers::get(int index)
{
    if (index >= data.size()) {
        data.resize(index + 1);
	data[index] = false;
    }
    return data[index];
}

/** @brief size
  *
  * @todo: document this function
  */
int sharers::size() const
{
    return data.size();
}

/** @brief count
  *
  * @todo: counts the number of 1's
  */
int sharers::count() const
{
    int count = 0;
    for (int i = 0; i < data.size(); i++)
    {
    
        if (data[i]) count++;
    }
    return count;
}

/** @brief clear
  *
  * @todo: document this function
  */
void sharers::clear()
{
    data.clear();
}

/** @brief set
  *
  * @todo: document this function
  */
void sharers::set(int index)
{
    if (index >= data.size())
        data.resize(index + 1);
    data[index] = true;
}

/** @brief reset
  *
  * @todo: document this function
  */
void sharers::reset(int index)
{
    if (index >= data.size())
        data.resize(index + 1);
    data[index] = false;
}

/** @brief ones
  *
  * @todo: document this function
  */
void sharers::ones(std::vector<int>& ret)
{
    for (int i = 0; i < data.size(); i++)
    {
        if (data[i]) ret.push_back(i);
    }
}


} //namespace mcp_cache_namespace
} //namespace manifold

