#ifndef AICS_REACTION_H
#define AICS_REACTION_H

#include <string>
#include <vector>

namespace aics {
namespace simularium {

    struct rxChildState {
        std::string name;
        std::string state;
        bool isBound;
    };

    struct rxParentState {
        std::string name;
        std::vector<rxChildState> children;
    };

    struct rxComplex {
    public:
        std::string name;
        std::vector<rxParentState> parents;
    };

    struct rxCenter {
        int parent_1, parent_2;
        std::string child_1, child_2;
    };

    /**
     *	A reaction stores information to be passed to a simulation package
     * 	Reactions occur between two adjacent agent hiearchy levels, say L1 && L2 with L1 > L2
     *		The product will be an agent either at L1 or one higher than L1
     */
    struct Reaction {
    public:
        std::string name;
        std::string type;
        std::string description;
        float rate;
        float reverse_rate;
        float distance;
        std::vector<rxComplex> complexes;
        std::vector<rxCenter> centers;
        std::string product;
    };

} // namespace simularium
} // namespace aics

#endif // AICS_REACTION_H
