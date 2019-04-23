//
// Created by mho on 4/17/19.
//

#include <readdy/readdy.h>
#include <h5rd/h5rd.h>
#include <readdy/model/observables/io/Types.h>

using namespace readdy;

int main() {
    {
        // create and configure simulation object
        Simulation sim("SingleCPU");
        sim.context().periodicBoundaryConditions() = {{ true, true, true }};
        sim.context().boxSize() = {{ 10., 10., 10. }};
        sim.context().particleTypes().add("A", 1.0, model::particleflavor::NORMAL);
        sim.context().particleTypes().add("B", 1.0, model::particleflavor::NORMAL);
        sim.context().particleTypes().add("TA", 1.0, model::particleflavor::TOPOLOGY);
        sim.context().particleTypes().add("TB", 1.0, model::particleflavor::TOPOLOGY);
        sim.context().topologyRegistry().addType("T");
        sim.context().topologyRegistry().addType("T2");
        sim.context().topologyRegistry().addSpatialReaction("MySuperReaction: T(TA) + (A) -> T(TA--TA)", 1e5, 5.);
        sim.context().topologyRegistry().configureBondPotential("TA", "TA", api::Bond{
                .forceConstant = 10.,
                .length = .1
        });

        sim.context().topologyRegistry().addStructuralReaction(
          "T",
          model::top::reactions::StructuralTopologyReaction(
            [&](model::top::GraphTopology &top)
            {
              std::cout << "Beep Boop" << std::endl;
              model::top::reactions::Recipe recipe(top);
              recipe.changeTopologyType("T2");
              return recipe;
            },
            1e30
          )
        );

        // create a file
        auto file = File::create("test.h5", File::Flag::OVERWRITE);

        // create trajectory observable
        auto trajPtr = sim.observe().flatTrajectory(1);
        // and let it write into file upon evaluation
        trajPtr->enableWriteToFile(*file, "mytraj", 10);
        // attach the observable to the simulation
        auto handle = sim.registerObservable(std::move(trajPtr));

        // add 100 particles
        for (auto i = 0U; i < 100; ++i) {
            sim.addParticle("A", model::rnd::uniform_real(-4., 4.), model::rnd::uniform_real(-4., 4.),
                            model::rnd::uniform_real(-4., 4.));
        }
        // and one topology
        sim.addTopology("T", {model::TopologyParticle(0, 0, 0, sim.context().particleTypes().idOf("TA"))});

        // create simulation loop
        auto loop = sim.createLoop(1e-2);
        // and write (most of the) configuration into the output file
        loop.writeConfigToFile(*file);
        // run the simulation
        loop.run(100);
    }
    {
        // this is the struct that holds the read-back particle data
        struct ParticleData {
            ParticleData(std::string type, std::string flavor, const std::array<readdy::scalar, 3> &pos,
                         readdy::model::Particle::id_type id, readdy::time_step_type t)
                    : type(std::move(type)), flavor(std::move(flavor)), position(pos), id(id), t(t) {}

            std::string type;
            std::string flavor;
            std::array<readdy::scalar, 3> position;
            readdy::model::Particle::id_type id;
            readdy::time_step_type t;
        };

        // if running the script independently, the blosc filter has to be registered
        // here, this is actually not necessary
        io::BloscFilter bloscFilter;
        bloscFilter.registerFilter();

        // open the output file
        auto f = h5rd::File::open("test.h5", File::Flag::READ_ONLY);

        // retrieve the h5 particle type info type (composite type describing how the data is stored in the file)
        auto particleInfoH5Type = model::ioutils::getParticleTypeInfoType(f->ref());

        // get particle types from config
        std::vector<model::ioutils::ParticleTypeInfo> types;
        {
            auto config = f->getSubgroup("readdy/config");
            config.read("particle_types", types, &std::get<0>(particleInfoH5Type), &std::get<1>(particleInfoH5Type));
        }
        std::unordered_map<std::size_t, std::string> typeMapping;
        for (const auto &type : types) {
            typeMapping[type.type_id] = std::string(type.name);
        }

        // read back trajectory
        auto traj = f->getSubgroup("readdy/trajectory/mytraj");

        // limits of length 2T containing [start_ix, end_ix] for each time step
        std::vector<std::size_t> limits;
        traj.read("limits", limits);

        // time of length T containing the time steps of recording the trajectory
        std::vector<time_step_type> time;
        traj.read("time", time);

        // records containing all particles for all times, slicing according to limits
        // yields particles for a particular time step
        std::vector<model::observables::TrajectoryEntry> entries;
        auto trajectoryEntryTypes = model::observables::util::getTrajectoryEntryTypes(f->ref());
        traj.read("records", entries, &std::get<0>(trajectoryEntryTypes), &std::get<1>(trajectoryEntryTypes));

        auto n_frames = limits.size() / 2;
        log::info("got n frames: {}", n_frames);

        // mapping the read-back data to the ParticleData struct
        std::vector<std::vector<ParticleData>> result;
        result.reserve(n_frames);

        auto timeIt = time.begin();
        for (std::size_t frame = 0; frame < limits.size(); frame += 2, ++timeIt) {
            auto begin = limits[frame];
            auto end = limits[frame + 1];
            result.emplace_back();
            auto &currentFrame = result.back();
            currentFrame.reserve(end - begin);

            for (auto it = entries.begin() + begin; it != entries.begin() + end; ++it) {
                currentFrame.emplace_back(typeMapping[it->typeId],
                                          model::particleflavor::particleFlavorToString(it->flavor),
                                          it->pos.data, it->id, *timeIt);
            }
        }

        // outputting parts of what was stored in the file
        for (auto i=0U; i < result.size(); ++i) {
            const auto &particles = result[i];
            log::info("---- T={}, N={} ----", i, particles.size());
            for(const auto &p : particles) {
                log::info("\t type {}, id {}, pos ({}, {}, {})", p.type, p.id,
                          p.position[0], p.position[1], p.position[2]);
            }
        }
    }
    return 0;
}
