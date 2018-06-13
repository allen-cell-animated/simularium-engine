#include "gtest/gtest.h"
#include "OpenMM.h"
#include <cstdio>
#include <stdlib.h>
#include <time.h>

namespace aics {
namespace agentsim {
namespace test {

class OpenMMTest : public ::testing::Test
{
		protected:
		// You can remove any or all of the following functions if its body
		// is empty.

		OpenMMTest() {
			// You can do set-up work for each test here.
		}

		virtual ~OpenMMTest() {
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

// Forward declaration of routine for printing one frame of the
// trajectory, defined later in this source file.
void writePdbFrame(int frameNum, const OpenMM::State&);

// This is intended as a verification of OpenMM functionality
//  not an accurate actin dynamics simulation
TEST_F(OpenMMTest, LinearActinSprings) {
		float g_actin_mass = 42000; //42kda
		float binding_angle = 2.87979; // 90 degrees
		float ba_hfc = 1200; // harmonic force constant for angles, kJ/mol/radian^2
		float binding_distance = 7; // 7nm
		float bl_hfc = 1000; // harmonic force constant for bonds, kJ/mol/nm^2

		OpenMM::System system;
		OpenMM::HarmonicAngleForce* monomerHAF = new OpenMM::HarmonicAngleForce();
		OpenMM::HarmonicBondForce* monomerHBF = new OpenMM::HarmonicBondForce();

		// Create 10 monomers
		int numMonomers = 10;
		std::vector<OpenMM::Vec3> initPosInNm(numMonomers);
		for(int i = 0; i < numMonomers; ++i)
		{
				initPosInNm[i] = OpenMM::Vec3(rand() % 5, rand() % 6, rand() % 7);
				system.addParticle(1.0);
		}

		// Attach the monomers by harmonic bonds
		for(int i = 0; i < numMonomers - 1; ++i)
		{
				monomerHBF->addBond(i, i+1, binding_distance, bl_hfc);
		}

		// Twisting? Harmonic angle forces
		for(int i = 0; i < numMonomers - 2; ++i)
		{
				monomerHAF->addAngle(i, i+1, i+2, binding_angle, ba_hfc);
		}

		system.addForce(monomerHAF);
		system.addForce(monomerHBF);

		OpenMM::VerletIntegrator integrator(0.004); // step size in ps

		// Let OpenMM Context choose best platform.
    OpenMM::Context context(system, integrator);
    printf( "REMARK  Using OpenMM platform %s\n",
        context.getPlatform().getName().c_str() );

		// Set starting positions of the atoms. Leave time and velocity zero.
    context.setPositions(initPosInNm);

    // Simulate.
    for (int frameNum=1; ;++frameNum) {
        // Output current state information.
        OpenMM::State state    = context.getState(OpenMM::State::Positions);
        const double  timeInPs = state.getTime();
        writePdbFrame(frameNum, state); // output coordinates

        if (timeInPs >= 10.)
            break;

        // Advance state many steps at a time, for efficient use of OpenMM.
        integrator.step(500); // (use a lot more than this normally)
    }
}

// Handy homebrew PDB writer for quick-and-dirty trajectory output.
void writePdbFrame(int frameNum, const OpenMM::State& state)
{
    // Reference atomic positions in the OpenMM State.
    const std::vector<OpenMM::Vec3>& posInNm = state.getPositions();

    // Use PDB MODEL cards to number trajectory frames
    printf("MODEL     %d\n", frameNum); // start of frame
    for (int a = 0; a < (int)posInNm.size(); ++a)
    {
        printf("ATOM  %5d  AR   AR     1    ", a+1); // atom number
        printf("%8.3f%8.3f%8.3f  1.00  0.00\n",      // coordinates
            // "*10" converts nanometers to Angstroms
            posInNm[a][0]*10, posInNm[a][1]*10, posInNm[a][2]*10);
    }
    printf("ENDMDL\n"); // end of frame
}

} // namespace test
} // namespace agentsim
} // namespace aics
