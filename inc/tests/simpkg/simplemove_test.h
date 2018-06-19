#include "gtest/gtest.h"
#include "agentsim/simpkg/simplemove.h"
#include <time.h>

namespace aics {
namespace agentsim {
namespace test {

class SimpleMoveTest : public ::testing::Test
{
	protected:
	// You can remove any or all of the following functions if its body
	// is empty.

	SimpleMoveTest() {
		// You can do set-up work for each test here.
	}

	virtual ~SimpleMoveTest() {
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

TEST_F(SimpleMoveTest, CollisionResolution) {
  std::vector<Agent*> agents;
	std::size_t agent_num = 1000;
	int numTimeSteps = 100;
	SimPkg* simpleMove = new SimpleMove();
	simpleMove->Setup();

	for(std::size_t i = 0; i < agent_num; ++i)
	{
			Agent* temp = new Agent();
			agents.push_back(temp);
	}

	for(int j = 0; j < numTimeSteps; ++j)
	{
			simpleMove->RunTimeStep(1.0, agents);

			for(std::size_t k = 0; k < agent_num; ++k)
			{
					for(std::size_t l = k; l > 0; --l)
					{
							printf("simple move iteration %lu\n", k);
							bool overlapping = agents[k]->IsCollidingWith(*(agents[l]));
							ASSERT_FALSE(overlapping);
					}
			}
	}

	for(std::size_t i = 0; i < agent_num; ++i)
	{
			delete agents[i];
	}

	simpleMove->Shutdown();
	delete simpleMove;
}

} // namespace test
} // namespace agentsim
} // namespace aics
