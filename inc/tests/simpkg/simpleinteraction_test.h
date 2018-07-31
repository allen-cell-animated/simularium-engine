#include "gtest/gtest.h"
#include "agentsim/simpkg/simplemove.h"
#include "agentsim/simpkg/simpleinteraction.h"
#include <time.h>

namespace aics {
namespace agentsim {
namespace test {

class SimpleInteractionTest : public ::testing::Test
{
	protected:
	// You can remove any or all of the following functions if its body
	// is empty.

	SimpleInteractionTest() {
		// You can do set-up work for each test here.
	}

	virtual ~SimpleInteractionTest() {
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

TEST_F(SimpleInteractionTest, CollisionResolution) {
  std::vector<std::shared_ptr<Agent>> agents;
	std::size_t agent_num = 1000;
	int numTimeSteps = 100;

	SimPkg* simpleMove = new SimpleMove();
	simpleMove->Setup();

	SimPkg* simpleInteraction = new SimpleInteraction();
	simpleInteraction->Setup();

	for(std::size_t i = 0; i < agent_num; ++i)
	{
			std::shared_ptr<Agent> temp(new Agent());
			agents.push_back(temp);
	}

	for(int j = 0; j < numTimeSteps; ++j)
	{
			printf("TimeStep %lu Finished\n", j);
			simpleMove->RunTimeStep(100.0, agents);
			simpleInteraction->RunTimeStep(100.0, agents);
	}

	simpleMove->Shutdown();
	delete simpleMove;

	simpleInteraction->Shutdown();
	delete simpleInteraction;
}

} // namespace test
} // namespace agentsim
} // namespace aics
