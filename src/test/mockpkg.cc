#include "simularium/simularium.h"

namespace aics {
namespace simularium {
namespace test {

    class MockSimPkg : public SimPkg {
    public:
      MockSimPkg(TrajectoryFileProperties& tfp) {
        this->m_fileProps = tfp;
      }
      virtual ~MockSimPkg() {};

      virtual void Setup() override {};
      virtual void Shutdown() override {};
      virtual void InitAgents(
        std::vector<std::shared_ptr<Agent>>& agents, Model& model
      ) override {};
      virtual void InitReactions(Model& model) override {};

      virtual void RunTimeStep(
          float timeStep, std::vector<std::shared_ptr<Agent>>& agents
        ) override {};

      virtual void UpdateParameter(
          std::string paramName, float paramValue
        ) override {};

      // Cached Sim API
      virtual void GetNextFrame(
          std::vector<std::shared_ptr<Agent>>& agents
        ) override {};

      virtual bool IsFinished() override { return true; };
      virtual void Run(float timeStep, std::size_t nTimeStep) override {};

      virtual void LoadTrajectoryFile(
          std::string file_path,
          TrajectoryFileProperties& fileProps
      ) override {
        fileProps = this->m_fileProps;
      };

      virtual double GetSimulationTimeAtFrame(std::size_t frameNumber) override
          { return 0; };

      virtual std::size_t GetClosestFrameNumberForTime(double timeNs) override
          { return 0; };

      virtual bool CanLoadFile(std::string filePath) override
          { return true; }

      virtual std::vector<std::string> GetFileNames(std::string filePath) override
          { return { filePath }; }

      private:
        TrajectoryFileProperties m_fileProps;
    };

} // namespace test
} // namespace simularium
} // namespace aics
