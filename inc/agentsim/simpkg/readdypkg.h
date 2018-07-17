#ifndef AICS_READDYPKG_H
#define AICS_READDYPKG_H

#include "agentsim/simpkg/simpkg.h"
#include "readdy/readdy.h"

#include <vector>
#include <string>

namespace aics {
namespace agentsim {

class ReaDDyPkg : public SimPkg
{
public:
	ReaDDyPkg() : m_simulation("SingleCPU") {}
	virtual ~ReaDDyPkg() { }

	virtual void Setup() override;
	virtual void Shutdown() override;

	virtual void RunTimeStep(
		float timeStep, std::vector<std::shared_ptr<Agent>>& agents) override;

	virtual void UpdateParameter(
		std::string param_name, float param_value
	) override;

private:
	readdy::Simulation m_simulation;
	bool initialized = false;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_READDYPKG_H
