#ifndef AICS_OPENMM_SIMULATOR_H
#define AICS_OPENMM_SIMULATOR_H

#include <vector>

namespace aics {
namespace agentsim {

class Agent;

class OpenMMSimulator
{
public:
		virtual void RunTimeStep(float time, std::vector<Agent*> agents);
};

}
}


#endif // AICS_OPENMM_SIMULATOR_H
