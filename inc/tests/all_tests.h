#ifndef ALLTESTS_H
#define ALLTESTS_H

#include "agents/agent_test.h"
#include "agents/subtree_test.h"
#include "simpkg/simplemove_test.h"
#include "interactions/statechange_reaction_test.h"
#include "interactions/combination_reaction_test.h"

#ifdef MAKE_EXT_PKGS
#include "extpkg/openmm_test.h"
#include "extpkg/readdy_test.h"
#endif // MAKE_EXT_PKGS

#endif // ALLTESTS_H
