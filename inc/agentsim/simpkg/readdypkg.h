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
	virtual ~ReaDDyPkg() {}

	virtual void Setup() override;
	virtual void Shutdown() override;

	virtual void RunTimeStep(
		float timeStep, std::vector<std::shared_ptr<Agent>>& agents) override;

private:
	readdy::Simulation* m_simulation;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_READDYPKG_H
