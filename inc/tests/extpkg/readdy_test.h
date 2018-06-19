#include "gtest/gtest.h"
#include "readdy/readdy.h"

namespace aics {
namespace agentsim {
namespace test {

class ReaDDyTest : public ::testing::Test
{
	protected:
	// You can remove any or all of the following functions if its body
	// is empty.

	ReaDDyTest() {
		// You can do set-up work for each test here.
	}

	virtual ~ReaDDyTest() {
		// You can do clean-up work that doesn't throw exceptions here.
	}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp() {
		// Code here will be called immediately after the constructor (right
		// before each test).
	}

	virtual void TearDown() {
		// Code here will be called immediately after each test (right
		// before the destructor).
	}

	// Objects declared here can be used by all tests in the test case for Foo.
};

// This is intended as a verification of ReaDDy functionality
//  not an accurate simulation
TEST_F(ReaDDyTest, BasicSimulation) {
	double boxSize = 1000; // 1000 nm
	readdy::Simulation simulation;

	simulation.setKernel("SingleCPU");
	simulation.setKBT(300); // probably kelvin?
	simulation.setBoxSize(boxSize, boxSize, boxSize);

	readdy::model::Context context;
	context.topology_registry().addType("actin-polymer");

	// actin binding sites
	context.particle_types().addTopologyType("pointed", 0.);
	context.particle_types().addTopologyType("side1", 0.);
	context.particle_types().addTopologyType("side2", 0.);
	context.particle_types().addTopologyType("barbed", 0.);

	context.particle_types().addTopologyType("actin-monomer", 0.);
}

} // namespace test
} // namespace agentsim
} // namespace aics
