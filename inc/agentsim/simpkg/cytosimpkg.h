#ifndef AICS_CYTOSIMPKG_H
#define AICS_CYTOSIMPKG_H

#include "agentsim/simpkg/simpkg.h"

#include <string>
#include <vector>

namespace aics {
namespace agentsim {

    class CytosimPkg : public SimPkg {
    public:
        CytosimPkg() {}
        virtual ~CytosimPkg() {}

        virtual void Setup() override;
        virtual void Shutdown() override;
        virtual void InitAgents(std::vector<std::shared_ptr<Agent>>& agents, Model& model) override;
        virtual void InitReactions(Model& model) override;

        virtual void RunTimeStep(
            float timeStep, std::vector<std::shared_ptr<Agent>>& agents) override;

        virtual void UpdateParameter(
            std::string param_name, float param_value) override;

        virtual void Run(float timeStep, std::size_t nTimeStep) override;
        virtual void GetNextFrame(
            std::vector<std::shared_ptr<Agent>>& agents) override;

        virtual bool IsFinished() override;
        virtual void LoadTrajectoryFile(
            std::string file_path,
            TrajectoryFileProperties& fileProps
        ) override {};
        virtual double GetSimulationTimeAtFrame(std::size_t frameNumber) override { return 0.0; };
        virtual std::size_t GetClosestFrameNumberForTime(double timeNs) override { return 0; };

    private:
        bool m_hasAlreadyRun = false;
        bool m_hasAlreadySetup = false;
        bool m_hasFinishedStreaming = false;
        bool m_hasLoadedFrameReader = false;
    };

} // namespace agentsim
} // namespace aics

#endif // AICS_CYTOSIMPKG_H
