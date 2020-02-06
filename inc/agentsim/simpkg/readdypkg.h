#ifndef AICS_READDYPKG_H
#define AICS_READDYPKG_H

#include "agentsim/simpkg/simpkg.h"
#include <string>
#include <vector>

namespace readdy {
class Simulation;

namespace io {
class BloscFilter;
} // namespace io
} // namespace readdy

namespace aics {
namespace agentsim {
    struct ReaDDyFileInfo;

    class ReaDDyPkg : public SimPkg {
    public:
        ReaDDyPkg();
        virtual ~ReaDDyPkg();

        virtual void Setup() override;
        virtual void Shutdown() override;
        virtual void InitAgents(std::vector<std::shared_ptr<Agent>>& agents, Model& model) override;
        virtual void InitReactions(Model& model) override;

        virtual void RunTimeStep(
            float timeStep, std::vector<std::shared_ptr<Agent>>& agents) override;

        virtual void UpdateParameter(
            std::string param_name, float param_value) override;

        // Cached Sim API
        virtual void GetNextFrame(
            std::vector<std::shared_ptr<Agent>>& agents) override;

        virtual bool IsFinished() override;
        virtual void Run(float timeStep, std::size_t nTimeStep) override;

        virtual void LoadTrajectoryFile(
            std::string file_path,
            TrajectoryFileProperties& fileProps
        ) override;
        virtual double GetSimulationTimeAtFrame(std::size_t frameNumber) override;
        virtual std::size_t GetClosestFrameNumberForTime(double timeNs) override;

        virtual bool CanLoadFile(std::string filePath) override
            { return filePath.substr(filePath.find_last_of(".") + 1) == "h5"; }

        virtual std::vector<std::string> GetFileNames(std::string filePath) override
            { return { filePath }; }

    private:
        readdy::Simulation* m_simulation;
        bool m_agents_initialized = false;
        bool m_reactions_initialized = false;
        int m_timeStepCount = 0;

        bool m_hasAlreadyRun = false;
        bool m_hasLoadedRunFile = false;
        bool m_hasFinishedStreaming = false;

        // The compression filter used by ReaDDy (needed to read/write files)
        //  that are compatible with the default ReaDDy Distribution
        std::shared_ptr<readdy::io::BloscFilter> m_bloscFilter;

        // Used to store FileIO data parsed from ReaDDy trajectories (*.h5 files)
        std::shared_ptr<ReaDDyFileInfo> m_fileInfo;
    };

} // namespace agentsim
} // namespace aics

#endif // AICS_READDYPKG_H
