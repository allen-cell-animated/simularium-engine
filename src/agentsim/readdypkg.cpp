#define USE_HARDCODED 0

#if USE_HARDCODED == 1
#include "readdypkg_hardcoded.cc"
#elif USE_HARDCODED == 0
#include "readdypkg_generalized.cc"
#endif
