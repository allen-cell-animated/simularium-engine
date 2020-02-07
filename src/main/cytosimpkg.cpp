#include "simul_prop.h"
#include "exceptions.h"
#include "frame_reader.h"
#include "glossary.h"
#include "messages.h"
#include "parser.h"
#include "simul.h"
#include "stream_func.h"
#include "tictoc.h"
#include <csignal>
#include <stdlib.h>

#include "agentsim/simpkg/cytosimpkg.h"
#include "agentsim/agents/agent.h"
#include "loguru/loguru.hpp"

using std::endl;

/**
*	Simulation API
**/
namespace aics {
namespace agentsim {
    const std::string CytosimPkg::PKG_DIRECTORY = "./cytosimpkg";

    CytosimPkg::CytosimPkg()
    {
        std::string mkdirCmd = "mkdir -p " + CytosimPkg::PKG_DIRECTORY;
        system(mkdirCmd.c_str());
    }

    CytosimPkg::~CytosimPkg()
    {

    }

    void CytosimPkg::Setup()
    {
        LOG_F(INFO, "Cytosim PKG Setup called");
        if(!this->m_reader.get()) {
            this->m_reader.reset(new FrameReader());
        }

        if(!this->m_simul.get()) {
            this->m_simul.reset(new Simul());
        }
    }

    void CytosimPkg::Shutdown()
    {
        LOG_F(INFO, "Cytosim PKG Shutdown called");
        if(this->m_reader.get() && this->m_reader->good()) {
            this->m_reader->clear();
        }

        if(this->m_simul.get()) {
            this->m_simul->erase();
            this->m_simul->prop->clear();
        }

        this->m_hasFinishedStreaming = false;
        this->m_hasLoadedFile = false;
    }

    void CytosimPkg::InitAgents(std::vector<std::shared_ptr<Agent>>& agents, Model& model)
    {
        LOG_F(INFO, "Cytosim PKG Creating 1000 agents");
        for (std::size_t i = 0; i < 1000; ++i) {
            std::shared_ptr<Agent> agent;
            agent.reset(new Agent());
            agent->SetVisibility(false);
            agents.push_back(agent);
        }
    }

    void CytosimPkg::InitReactions(Model& model) { }
    void CytosimPkg::RunTimeStep(
        float timeStep, std::vector<std::shared_ptr<Agent>>& agents)
    {
        LOG_F(ERROR, "Run-Time-Step is currently unimplemented in Cytosim PKG.");
        // @TODO: option to return error code from this function
    }

    void CytosimPkg::UpdateParameter(std::string paramName, float paramValue) { }
    void CytosimPkg::Run(float timeStep, std::size_t nTimeSteps)
    {
        this->m_trajectoryFile = CytosimPkg::TrajectoryFilePath();
        this->m_propertyFile = GetPropertyFileName(this->m_trajectoryFile);

        Simul simul;
        simul.prop->trajectory_file = this->m_trajectoryFile;
        simul.prop->property_file = this->m_propertyFile;
        simul.prop->config_file = this->m_configFile;
        if (Parser(simul, 1, 1, 1, 0, 0).readConfig()) {
                std::cerr << "You must specify a config file\n";
        }

        simul.prepare();

        unsigned     nb_steps = nTimeSteps;
        unsigned     nb_frames  = 0;
        int          solve      = 1;
        bool         prune      = true;
        bool         binary     = true;

        unsigned sss = 0;

        unsigned int  frame = 1;
        real          delta = nb_steps;
        unsigned long check = nb_steps;

        void (Simul::* solveFunc)() = &Simul::solve_not;
        switch ( solve )
        {
            case 1: solveFunc = &Simul::solve;      break;
            case 2: solveFunc = &Simul::solve_auto; break;
            case 3: solveFunc = &Simul::solveX;     break;
        }

        simul.writeProperties(nullptr, prune);
        if ( simul.prop->clear_trajectory )
        {
            simul.writeObjects(simul.prop->trajectory_file, false, binary);
            simul.prop->clear_trajectory = false;
        }
        delta = real(nb_steps) / real(nb_frames);
        check = delta;

        simul.prop->time_step = real(timeStep);
        while ( 1 )
        {
            if ( sss >= check )
            {
                simul.relax();
                simul.writeObjects(simul.prop->trajectory_file, true, binary);
                simul.unrelax();
                if ( sss >= nb_steps )
                    break;
                check = ( ++frame * delta );
            }

            fprintf(stderr, "> step %6i\n", sss);
            (simul.*solveFunc)();
            simul.step();
            ++sss;
        }

        simul.erase();
        simul.prop->clear();
    }

    bool CytosimPkg::IsFinished() { return this->m_hasFinishedStreaming; }

    void CytosimPkg::GetNextFrame(std::vector<std::shared_ptr<Agent>>& agents)
    {
        if(this->m_hasFinishedStreaming)
            return;

        if(agents.size() == 0) {
            LOG_F(INFO, "Cytosim PKG Creating 1000 agents");
            for (std::size_t i = 0; i < 1000; ++i) {
                std::shared_ptr<Agent> agent;
                agent.reset(new Agent());
                agent->SetVisibility(false);
                agents.push_back(agent);
            }
        }

        if(!this->m_hasLoadedFile) {
            TrajectoryFileProperties ignore;
            std::string currentFile = this->m_trajectoryFile;

            LOG_F(INFO, "Loading current trajectory file %s", currentFile.c_str());

            this->LoadTrajectoryFile(
                currentFile,
                ignore
            );
        }

        if(this->m_reader->eof()) {
            LOG_F(INFO, "Finished processing Cytosim Trajectory");
            this->m_hasFinishedStreaming = true;
        }

        if(this->m_reader->good()) {
            int errCode = this->m_reader->loadNextFrame(*(this->m_simul));
            std::size_t currentFrame = this->m_reader->currentFrame();
            if(errCode != 0) {
                LOG_F(ERROR, "Error loading Cytosim Reader: err-no %i", errCode);
                this->m_hasFinishedStreaming = true;
                return;
            } else {
                LOG_F(INFO, "Copying fibers from frame %i", static_cast<int>(currentFrame));
                this->CopyFibers(
                    agents,
                    this->m_reader.get(),
                    this->m_simul.get()
                );
            }
        } else {
            LOG_F(ERROR, "File could not be opened");
            this->m_hasFinishedStreaming = true;
            return;
        }
    }

    void CytosimPkg::LoadTrajectoryFile(
        std::string filePath,
        TrajectoryFileProperties& fileProps
    ) {
        this->m_trajectoryFile = filePath;
        this->m_propertyFile = this->GetPropertyFileName(this->m_trajectoryFile);

        LOG_F(INFO, "Loading Cytosim Trajectory: %s", this->m_trajectoryFile.c_str());
        LOG_F(INFO, "Loading Cytosim Properties File: %s", this->m_propertyFile.c_str());
        this->m_simul->prop->property_file = this->m_propertyFile;
        this->m_simul->loadProperties(); // @BREAK HERE AFTER LUNCH

        if(!this->m_reader->hasFile()) {
            try {
                this->m_reader->openFile(this->m_trajectoryFile);
            } catch (Exception& e) {
                std::cerr << "Aborted: " << e.what() << std::endl;
                return;
            }
        }

        fileProps.fileName = filePath;
        fileProps.numberOfFrames = 1000; //@TODO: How to get # frames from Cytosim File
        fileProps.timeStepSize = real(this->m_simul->prop->time_step);
        fileProps.typeMapping = this->m_typeMapping;
        fileProps.boxX = 100;
        fileProps.boxY = 100;
        fileProps.boxZ = 100;
        this->m_hasLoadedFile = true;
    }

    void CytosimPkg::CopyFibers(
        std::vector<std::shared_ptr<Agent>>& agents,
        FrameReader* reader,
        Simul* simul
    )
    {
        int fiberIndex = 0;
        for (Fiber * fib = simul->fibers.first(); fib; fib = fib->next(), fiberIndex++)
        {
            if (fiberIndex >= agents.size()) {
                LOG_F(ERROR, "Not enough agents to represent fibers");
                return;
            }

            auto agent = agents[fiberIndex];
            agents[fiberIndex]->SetLocation(0,0,0);

            agents[fiberIndex]->SetVisType(vis_type_fiber);
            agents[fiberIndex]->SetVisibility(true);
            agents[fiberIndex]->SetCollisionRadius(0.5);
            agents[fiberIndex]->SetTypeID(CytosimPkg::TypeId::FiberId);
            for(std::size_t p=0; p < fib->nbPoints(); ++p) {
                Vector cytosimPos = fib->posPoint(p);
                float x = cytosimPos[0];
                float y = cytosimPos[1];
                float z = cytosimPos[2];
                agents[fiberIndex]->UpdateSubPoint(p,x,y,z);
            }
        }
    }

} // namespace agentsim
} // namespace aics
