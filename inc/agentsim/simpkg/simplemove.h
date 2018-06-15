#ifndef AICS_SIMPLEMOVE_H
#define AICS_SIMPLEMOVE_H

#include "agentsim/simpkg/simpkg.h"

namespace aics {
namespace agentsim {

class SimpleMove : public SimPkg
{
public:
		virtual void Setup() override;
		virtual void Shutdown() override;

		/**
		* RunTimeStep
		*
		*	@param timeStep		the time to advance simulation, in picoseconds
		* @param agents			a list of Agents to be moved
		*
		* description				this function will randomly place agents,
		*										avoiding spherical collision
		*/
		virtual void RunTimeStep(
			float timeStep, std::vector<Agent*>& agents) override;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_SIMPKG_H
