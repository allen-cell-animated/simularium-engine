#include "agentsim/agents/agent.h"
#include <vector>
#include "Eigen/Dense"

namespace aics {
namespace agentsim {

void Agent::AddBoundPartner(Agent* other)
{
		this->m_boundPartners.push_back(other);
}

} // namespace agentsim
} // namespace aics
