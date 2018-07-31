#include "agentsim/simpkg/simpleinteraction.h"
#include "agentsim/agents/agent.h"
#include "agentsim/interactions/reaction.h"

namespace aics {
namespace agentsim {

void SimpleInteraction::Setup()
{

}

void SimpleInteraction::Shutdown()
{

}

void SimpleInteraction::RunTimeStep(
	float timeStep, std::vector<std::shared_ptr<Agent>>& agents)
{
	this->EvaluateInteractions(agents, this->m_interactions, this->m_reactions);
}

void SimpleInteraction::EvaluateInteractions(
	std::vector<std::shared_ptr<Agent>>& agents,
	std::vector<SimpleInteraction::InteractionEvent>& interactions,
	std::unordered_map<std::string, std::vector<std::shared_ptr<Reaction>>>& reactions)
{
	interactions.clear();

	for(std::size_t i = 0; i < agents.size(); ++i)
	{
			for(std::size_t j = 0; j < i; ++j)
			{
					if(agents[i]->CanInteractWith(*(agents[j].get())))
					{
							InteractionEvent interaction;
							interaction.a1 = i;
							interaction.a2 = j;
							interactions.push_back(interaction);
					}
			}
	}

	for(std::size_t i = 0; i < interactions.size(); ++i)
	{
		auto interaction = interactions[i];
		std::string key = agents[interaction.a1]->GetName() + agents[interaction.a2]->GetName();
		std::string rkey = agents[interaction.a1]->GetName() + agents[interaction.a2]->GetName();

		std::vector<std::shared_ptr<Reaction>> rxs;
		if(reactions.count(key) != 0)
		{
			rxs = reactions[key];
		}
		else if (reactions.count(rkey))
		{
			rxs = reactions[rkey];
		}
		else
		{
			continue;
		}

		for(std::size_t j = 0; j < rxs.size(); ++j)
		{
			auto rx = rxs[j];
			if(!rx->IsReactant(agents[interaction.a1].get()) ||
				!rx->IsReactant(agents[interaction.a2].get()))
			{
				continue;
			}

			rx->React(agents[interaction.a1], agents[interaction.a2]);
			agents.push_back(rx->GetProduct());

			agents.erase(agents.begin()+i);
			agents.erase(agents.begin()+j);
		}
	}
}

void SimpleInteraction::AddReaction(std::shared_ptr<Reaction> rx)
{
	this->m_reactions[rx->GetKey()].push_back(rx);
}

} // namespace agentsim
} // namespace aics
