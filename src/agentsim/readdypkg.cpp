#define USE_HARDCODED 1
#include "agentsim/agents/agent.h"
#include "agentsim/simpkg/readdypkg.h"
#include "readdypkg_fileio.cc"

namespace aics {
namespace agentsim {
    ReaDDyPkg::ReaDDyPkg() {
        this->m_simulation = new readdy::Simulation("SingleCPU");
        this->m_bloscFilter.registerFilter();
    }

    ReaDDyPkg::~ReaDDyPkg() {
        delete this->m_simulation;
        this->m_simulation = nullptr;
    }

} // namespace agentsim
} // namespace aics

#if USE_HARDCODED == 1
#include "readdypkg_hardcoded.cc"
#elif USE_HARDCODED == 0
#include "readdypkg_generalized.cc"
#endif
