#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
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
      ID_VIS_DATA_FINISH
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
  readdySimPkg->InitParticles();

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

	// Server Run-Time Loop
	while (1)
	{
		if(isRunningSimulation)
		{
      if(timeStepCounter >= requestData.num_time_steps)
      {
        isRunningSimulation = false;
        printf("Finished running requested simulation\n");

        RakNet::BitStream bs;
        bs.Write((MessageID)ID_VIS_DATA_FINISH);
        peer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, clientAddress, false);
      }
      else
      {
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

  			peer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, clientAddress, false);
        timeStepCounter++;
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
						printf("A client lost the connection.\n");
					break;
				case ID_VIS_DATA_ARRIVE:
				  {
				    printf("Simulation data has arrived/\n");
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
            clientAddress = packet->systemAddress;
						isRunningSimulation = true;
						timeStepCounter = 0;
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
