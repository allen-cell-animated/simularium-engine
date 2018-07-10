#ifndef AICS_READDYPKG_H
#define AICS_READDYPKG_H

#include <vector>
#include <string>

namespace readdy {
namespace model {
class Context;
} // namespace model

class Simulation;
} // namespace readdy

namespace ext {
namespace readdy {

struct rxd
{

};

} // namespace readdy
} // namespace ext

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

	void InitReactions(std::vector<ext::readdy::rxd> rxs);
	void InitParticles(std::vector<std::shared_ptr<Agent>>& agents);

private:
	float m_minTimeStep = 1.0f;
	readdy::Simulation* m_simulation;
	readdy::model::Context* m_context;

	std::vector<std::string> m_particleTypes;
};

} // namespace agentsim
} // namespace aics
#endif // AICS_READDYPKG_H
