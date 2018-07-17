#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include "RakString.h"
#include "BitStream.h"
#include "RakNetTypes.h"
#include "RakNetTime.h"
#include "GetTime.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "agentsim/agentsim.h"

#define MAX_CLIENTS 10
#define SERVER_PORT 60000

enum {
      ID_VIS_DATA_ARRIVE = ID_USER_PACKET_ENUM,
      ID_VIS_DATA_REQUEST,
      ID_VIS_DATA_FINISH,
      ID_VIS_DATA_PAUSE,
      ID_VIS_DATA_RESUME,
      ID_VIS_DATA_ABORT,
      ID_UPDATE_TIME_STEP,
      ID_UPDATE_RATE_PARAM
};

enum  {
	DevTest = 1,
	Readdy
};

static const char* SimulatorNames[] = {
  "Undefined",
  "Dev Test",
  "ReaDDy"
};

struct vis_data_request_params
{
	unsigned char simulator;
	float num_time_steps;
	float step_size;
};

using namespace RakNet;
using namespace aics::agentsim;

void deserialize_vis_data_request(
	RakNet::BitStream* bs, vis_data_request_params& v);

void deserialize_timestep_update(
  RakNet::BitStream* bs, float& timeStep);

void deserialize_rate_param_update(
  RakNet::BitStream* bs, std::string& paramName, float& paramValue);

int main(void)
{
	// Server setup
	RakNet::RakPeerInterface *peer = RakNet::RakPeerInterface::GetInstance();
	RakNet::Packet *packet;

  RakNet::SystemAddress clientAddress;

	RakNet::SocketDescriptor sd(SERVER_PORT,0);
	peer->Startup(MAX_CLIENTS, &sd, 1);

	printf("Starting the server.\n");
	peer->SetMaximumIncomingConnections(MAX_CLIENTS);

	// Simulation setup
  ReaDDyPkg* readdySimPkg = new ReaDDyPkg();

  std::shared_ptr<SimPkg> readdyPkg;
  readdyPkg.reset(readdySimPkg);

	std::vector<std::shared_ptr<SimPkg>> simulators;
  simulators.push_back(readdyPkg);

  std::vector<std::shared_ptr<Agent>> agents;
	Simulation simulation(simulators, agents);

	// Sim request setup
	vis_data_request_params requestData;
	bool isRunningSimulation = false;
	int timeStepCounter = 0;

  // timer setup
  auto start = std::chrono::steady_clock::now();

	// Server Run-Time Loop
	while (1)
	{
		if(isRunningSimulation)
		{
      if(requestData.num_time_steps >= 0 &&
        timeStepCounter >= requestData.num_time_steps)
      {
        isRunningSimulation = false;
        printf("Finished running requested simulation\n");

        RakNet::BitStream bs;
        bs.Write((MessageID)ID_VIS_DATA_FINISH);
        peer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, clientAddress, false);
      }
      else
      {
        // Limit packet sending to 120 pps
        auto now = std::chrono::steady_clock::now();
        auto diff = now - start;
        if(diff >= std::chrono::milliseconds(8))
        {
          start = now;

          RakNet::BitStream bs;
    			simulation.RunTimeStep(requestData.step_size);
    			auto simData = simulation.GetData();

          bs.Write((MessageID)ID_VIS_DATA_ARRIVE);
          bs.Write((float)simData.size());

          for(std::size_t i = 0; i < simData.size(); ++i)
          {
            auto agentData = simData[i];
            bs.Write(agentData.type);

            bs.Write(agentData.x);
            bs.Write(agentData.y);
            bs.Write(agentData.z);

            bs.Write(agentData.xrot);
            bs.Write(agentData.yrot);
            bs.Write(agentData.zrot);
          }

    			peer->Send(&bs, LOW_PRIORITY, UNRELIABLE_SEQUENCED, 0, clientAddress, false);
          timeStepCounter++;
        }
      }
		}

		for (packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
		{
			switch (packet->data[0])
				{
				case ID_REMOTE_DISCONNECTION_NOTIFICATION:
					printf("Another client has disconnected.\n");
					break;
				case ID_REMOTE_CONNECTION_LOST:
					printf("Another client has lost the connection.\n");
					break;
				case ID_REMOTE_NEW_INCOMING_CONNECTION:
					printf("Another client has connected.\n");
					break;
				case ID_CONNECTION_REQUEST_ACCEPTED:
				  {
						printf("Our connection request has been accepted.\n");
				  }	break;
				case ID_NEW_INCOMING_CONNECTION:
					printf("A connection is incoming.\n");
					break;
				case ID_NO_FREE_INCOMING_CONNECTIONS:
					printf("The server is full.\n");
					break;
				case ID_DISCONNECTION_NOTIFICATION:
						printf("A client has disconnected.\n");
					break;
				case ID_CONNECTION_LOST:
          {
  						printf("A client lost the connection.\n");
              isRunningSimulation = false;
  				}	break;
				case ID_VIS_DATA_ARRIVE:
				  {
				    printf("Simulation data has arrived\n");
				  } break;
				case ID_VIS_DATA_REQUEST:
				  {
						printf("Simulation data was requested.\n");
						RakNet::BitStream bs(packet->data, packet->length, false);
						deserialize_vis_data_request(&bs, requestData);

            printf("Running a simulation for %i timesteps using %s\n",
                (int)requestData.num_time_steps,
                SimulatorNames[(int)requestData.simulator]);

						// Start simulation
            simulation.Reset();
            clientAddress = packet->systemAddress;
						isRunningSimulation = true;
						timeStepCounter = 0;
            start = std::chrono::steady_clock::now();
				  } break;
        case ID_VIS_DATA_PAUSE:
				  {
				    printf("Simulation data streaming paused\n");
            isRunningSimulation = false;
				  } break;
        case ID_VIS_DATA_RESUME:
				  {
				    printf("Simulation data streaming resuming\n");
            isRunningSimulation = true;
				  } break;
        case ID_VIS_DATA_ABORT:
				  {
				    printf("Simulation Aborted\n");
            isRunningSimulation = true;
            requestData.num_time_steps = 0;
				  } break;
        case ID_UPDATE_TIME_STEP:
        {
          printf("TimeStep update arrived\n");
          RakNet::BitStream bs(packet->data, packet->length, false);
          deserialize_timestep_update(&bs, requestData.step_size);
        } break;
        case ID_UPDATE_RATE_PARAM:
        {
          printf("Rate param update arrived\n");
          RakNet::BitStream bs(packet->data, packet->length, false);

          std::string pname;
          float pval;

          deserialize_rate_param_update(&bs, pname, pval);
          simulation.UpdateParameter(pname, pval);

        } break;
				default:
					printf("Message with identifier %i has arrived.\n", packet->data[0]);
					break;
				}
		}
	}

	RakNet::RakPeerInterface::DestroyInstance(peer);
	return 0;
}

void deserialize_vis_data_request(
	RakNet::BitStream* bs, vis_data_request_params& v)
{
  printf("Deserializing vis data request \n");
	RakNet::MessageID id;
	bs->Read(id);
	bs->Read(v.simulator);
	bs->Read(v.num_time_steps);
	bs->Read(v.step_size);

  printf("Message ID: %i\n", (int)id);
  printf("Simulator: %i\n", (int)v.simulator);
  printf("Simulator: %s\n", SimulatorNames[(int)v.simulator]);
  printf("Num TimeSteps: %i\n", (int)v.num_time_steps);
  printf("Timestep size: %f\n", v.step_size);
}

void deserialize_timestep_update(
  RakNet::BitStream* bs, float& timeStep)
{
  RakNet::MessageID id;
	bs->Read(id);
	bs->Read(timeStep);
}

void deserialize_rate_param_update(
  RakNet::BitStream* bs, std::string& paramName, float& paramValue)
{
  RakNet::MessageID id;
	bs->Read(id);

  RakNet::RakString rs;
	bs->Read(rs);
  paramName = rs;

	bs->Read(paramValue);
}
