#include "agentsim/simpkg/simpleinteraction.h"
#include "agentsim/agents/agent.h"

namespace aics {
namespace agentsim {

void SimpleInteraction::Setup()
{

}

void SimpleInteraction::Shutdown()
{

}

void SimpleInteraction::RunTimeStep(
	float timeStep, std::vector<Agent*>& agents)
{
	this->EvaluateInteractions(agents, this->m_interactions);
}

void SimpleInteraction::EvaluateInteractions(
	std::vector<Agent*>& agents,
	std::vector<SimpleInteraction::InteractionEvent>& interactions)
{
	for(std::size_t i = 0; i < agents.size(); ++i)
	{
			for(std::size_t j = i; j > 0; --j)
			{
					if(agents[i]->CanInteractWith(*(agents[j])))
					{
							InteractionEvent interaction;
							interaction.a1 = agents[i];
							interaction.a2 = agents[j];
							interactions.push_back(interaction);
					}
			}
	}
}



} // namespace agentsim
} // namespace aics
