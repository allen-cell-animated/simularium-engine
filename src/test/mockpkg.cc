#include "simularium/simularium.h"
#include "simularium/network/trajectory_properties.h"

namespace aics {
namespace simularium {
    namespace test {

        class MockSimPkg : public SimPkg {
        public:
            MockSimPkg(std::shared_ptr<aics::simularium::fileio::TrajectoryInfo> fileProps)
            {
                this->m_fileProps = fileProps;
            }
            virtual ~MockSimPkg() {};

            virtual void Setup() override {};
            virtual void Shutdown() override {};
            virtual void InitAgents(
                std::vector<std::shared_ptr<Agent>>& agents, Model& model) override {};
            virtual void InitReactions(Model& model) override {};

            virtual void RunTimeStep(
                float timeStep, std::vector<std::shared_ptr<Agent>>& agents) override {};

            virtual void UpdateParameter(
                std::string paramName, float paramValue) override {};

            // Cached Sim API
            virtual void GetNextFrame(
                std::vector<std::shared_ptr<Agent>>& agents) override {};

            virtual bool IsFinished() override { return true; };
            virtual void Run(float timeStep, std::size_t nTimeStep) override {};

            virtual void LoadTrajectoryFile(
                std::string file_path,
                std::shared_ptr<aics::simularium::fileio::TrajectoryInfo> fileProps) override
            {
                auto json = this->m_fileProps->GetJSON();
                fileProps->ParseJSON(json);
            };

            virtual double GetSimulationTimeAtFrame(std::size_t frameNumber) override
            {
                return 0;
            };

            virtual std::size_t GetClosestFrameNumberForTime(double timeNs) override
            {
                return 0;
            };

            virtual bool CanLoadFile(std::string filePath) override
            {
                return true;
            }

            virtual std::vector<std::string> GetFileNames(std::string filePath) override
            {
                return { filePath };
            }

        private:
            std::shared_ptr<aics::simularium::fileio::TrajectoryInfo> m_fileProps;
        };

    } // namespace test
} // namespace simularium
} // namespace aics
