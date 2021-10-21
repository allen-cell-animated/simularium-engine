#ifndef AICS_CYTOSIMPKG_H
#define AICS_CYTOSIMPKG_H

#include "simularium/simpkg/simpkg.h"

#include <string>
#include <vector>

class Simul;
class FrameReader;

namespace aics {
namespace simularium {

    class CytosimPkg : public SimPkg {
    public:
        CytosimPkg();
        virtual ~CytosimPkg();

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
            std::shared_ptr<aics::simularium::fileio::TrajectoryInfo> fileProps) override;
        virtual double GetSimulationTimeAtFrame(std::size_t frameNumber) override { return 0.0; };
        virtual std::size_t GetClosestFrameNumberForTime(double timeNs) override { return 0; };
        virtual bool CanLoadFile(std::string filePath) override
        {
            return filePath.substr(filePath.find_last_of(".") + 1) == "cmo";
        }

        virtual std::vector<std::string> GetFileNames(std::string filePath) override
        {
            return {
                filePath,
                GetPropertyFileName(filePath)
            };
        }

    private:
        void CopyFibers(
            std::vector<std::shared_ptr<Agent>>& agents,
            FrameReader* reader,
            Simul* simul);

        static std::string TrajectoryFilePath()
        {
            return CytosimPkg::PKG_DIRECTORY + "/trajectory.cmo";
        }

        static std::string PropertyFilePath()
        {
            return CytosimPkg::PKG_DIRECTORY + "/properties.cmo";
        }

        std::string GetPropertyFileName(std::string filePath)
        {
            return filePath.substr(0, filePath.find_last_of(".")) + "_properties.cmo";
        }

        std::shared_ptr<FrameReader> m_reader;
        std::shared_ptr<Simul> m_simul;
        bool m_hasFinishedStreaming = false;
        bool m_hasLoadedFile = false;

        static const std::string PKG_DIRECTORY;
        std::string m_configFile = "./dep/cytosim/cym/aster.cym";
        std::string m_trajectoryFile = "";
        std::string m_propertyFile = "";

        enum TypeId {
            FiberId = 0
        };

        std::unordered_map<std::size_t, std::string> m_typeMapping {
            { TypeId::FiberId, "Fiber" }
        };
    };

} // namespace simularium
} // namespace aics

#endif // AICS_CYTOSIMPKG_H
