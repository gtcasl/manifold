#include "utils.h"

using namespace std;
using namespace manifold::kernel;

namespace manifold {

static int s_dbglevel = 4; 

void SetDbgLevel(int level) {
  s_dbglevel = level;
}

int getDbgLevel() {
  return s_dbglevel;
}

void DbgPrint(int level, const char *format, ...) {
  char buf[256];
  
  va_list args;
  va_start(args, format);
  if (level <= s_dbglevel) {
    vsnprintf(buf, 256, format, args);
    std::cout << buf;
  }
  va_end(args);
  
  if (s_dbglevel > 1)
    std::cout.flush();
}

unsigned fastLog2(unsigned value) {
  assert(isPow2(value));
  static unsigned b[] = {0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000};
  unsigned r = (value & b[0]) != 0;
  for (unsigned i = 4; i > 0; --i) {
    r |= ((value & b[i]) != 0) << i;
  }
  return r;
}
}
