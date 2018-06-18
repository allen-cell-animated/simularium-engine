#include "agentsim/agentsim.h"

using namespace aics::agentsim;

int main(int argc, char **argv) {
		std::vector<std::shared_ptr<SimPkg>> simPkgs;

		std::shared_ptr<SimPkg> mvptr;
		mvptr.reset(new SimpleMove());
		simPkgs.push_back(mvptr);

		std::shared_ptr<SimPkg> interptr;
		interptr.reset(new SimpleMove());
		simPkgs.push_back(interptr);

		Agent* a = new Agent();

		Simulation simulation(simPkgs);
		simulation.AddAgent(a);
		simulation.RunTimeStep(1.0f);

		delete a;
}
