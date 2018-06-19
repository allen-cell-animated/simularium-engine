#include "agentsim/simpkg/simplemove.h"
#include "agentsim/agents/agent.h"

#include <stdlib.h>
#include <time.h>

namespace aics {
namespace agentsim {

void SimpleMove::Setup()
{
	srand(time(NULL));
}

void SimpleMove::Shutdown()
{

}

void SimpleMove::RunTimeStep(float timeStep, std::vector<Agent*>& agents)
{
	for(std::size_t i = 0; i < agents.size(); ++i)
	{
			Agent* agent = agents[i];
			Eigen::Vector3d newLocation;
			SampleDiffusionStep(timeStep, agent, newLocation);
			agent->SetLocation(newLocation);

			Agent* collidingAgent = nullptr;
			for(std::size_t j = i; j > 0; --j)
			{
					if(agent->IsCollidingWith(*(agents[j])))
					{
							collidingAgent = agents[j];
							break;
					}
			}

			if(collidingAgent != nullptr)
			{
					Eigen::Vector3d dir =
						collidingAgent->GetLocation() - newLocation;
					dir.normalize();
					dir = dir * agent->GetCollisionRadius();

					agent->SetLocation(agent->GetLocation() - dir);
					collidingAgent->SetLocation(collidingAgent->GetLocation() + dir);
			}
	}
}

void SimpleMove::SampleDiffusionStep(
	float timeStep, Agent* agent, Eigen::Vector3d& newLocation)
{
	double ed = this->m_exp_dist(this->m_rng);
	double dc = agent->GetDiffusionCoefficient();
	double msd = 6 * dc * timeStep;

	double x = msd * ed * (double)(rand() % 101 - 50) / 100.0;
	double y = msd * ed * (double)(rand() % 101 - 50) / 100.0;
	double z = msd * ed * (double)(rand() % 101 - 50) / 100.0;

	Eigen::Vector3d old = agent->GetLocation();
	newLocation << x + old(0), y + old(1), z + old(2);
}

} // namespace agentsim
} // namespace aics
