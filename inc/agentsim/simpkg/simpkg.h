#ifndef AICS_SIMPKG_H
#define AICS_SIMPKG_H

namespace aics {
namespace agentsim {

class SimPkg
{
public:
	virtual void Setup() = 0;
	virtual void RunTimeStep(float timeStep) = 0;
	virtual void Shutdown() = 0;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_SIMPKG_H
