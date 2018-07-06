#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include "RakString.h"
#include "BitStream.h"
#include "RakNetTime.h"
#include "GetTime.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"

#define MAX_CLIENTS 10
#define SERVER_PORT 60000

enum {
      ID_VIS_DATA_FILE = ID_USER_PACKET_ENUM,
      ID_VIS_DATA_REQUEST
};

struct vector3
{
  float x;
  float y;
  float z;
};

struct agent
{
  vector3 location;
  vector3 rotation;
  RakNet::RakString id;
  RakNet::RakString name;
};

struct data
{
  std::vector<agent> agents;
};

bool serialize(data d, RakNet::BitStream* b);
bool deserialize(RakNet::BitStream* b, data& d);

using namespace RakNet;

int main(void)
{
  	char str[512];
	RakNet::RakPeerInterface *peer = RakNet::RakPeerInterface::GetInstance();
	bool isServer;
	RakNet::Packet *packet;

	data server_data;
	data client_data;

	printf("(C) or (S)erver?\n");
	fgets(str, sizeof(str), stdin);

	if ((str[0]=='c')||(str[0]=='C'))
	{
		RakNet::SocketDescriptor sd;
		peer->Startup(1,&sd, 1);
		isServer = false;
	} else {
		RakNet::SocketDescriptor sd(SERVER_PORT,0);
		peer->Startup(MAX_CLIENTS, &sd, 1);
		isServer = true;
	}


	if (isServer)
	{
		printf("Starting the server.\n");
		// We need to let the server accept incoming connections from the clients
		peer->SetMaximumIncomingConnections(MAX_CLIENTS);
	} else {
		printf("Enter server IP or hit enter for 127.0.0.1\n");
		fgets(str, sizeof(str), stdin);
		if (strlen(str) < 2){
			strcpy(str, "127.0.0.1");
		}
		printf("Starting the client.\n");
		peer->Connect(str, SERVER_PORT, 0,0);

	}

	if(isServer)
	  {
	    for(std::size_t i = 0; i < 100; ++i)
	      {
		agent a;
		a.id = i;
		a.name = "Agent" + i;

		a.location.x = i;
		a.location.y = i * 2;
		a.location.z = i * 3;

		a.rotation.x = i;
		a.rotation.y = i / 2;
		a.rotation.z = i / 3;

		server_data.agents.push_back(a);
	      }
	  }

	while (1)
	{
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
					RakNet::BitStream bs;
					bs.Write((RakNet::MessageID)ID_VIS_DATA_REQUEST);
					peer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
				  }	break;
				case ID_NEW_INCOMING_CONNECTION:
					printf("A connection is incoming.\n");
					break;
				case ID_NO_FREE_INCOMING_CONNECTIONS:
					printf("The server is full.\n");
					break;
				case ID_DISCONNECTION_NOTIFICATION:
					if (isServer){
						printf("A client has disconnected.\n");
					} else {
						printf("We have been disconnected.\n");
					}
					break;
				case ID_CONNECTION_LOST:
					if (isServer){
						printf("A client lost the connection.\n");
					} else {
						printf("Connection lost.\n");
					}
					break;
				case ID_VIS_DATA_FILE:
				  {
				    if(!isServer) {
				      printf("Simulation data has arrived.\n");
				      RakNet::BitStream bs(packet->data, packet->length, false);
				      deserialize(&bs, client_data);
				    }
				  } break;
				case ID_VIS_DATA_REQUEST:
				  {
				    if(isServer)
				      {
					printf("Simulation data was requested.\n");
					RakNet::BitStream bs;
					serialize(server_data, &bs);
					peer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
				      }
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


bool serialize(data d, RakNet::BitStream* bs)
{
  bs->Write((RakNet::MessageID)ID_VIS_DATA_FILE);
  bs->Write((unsigned short)d.agents.size());

  for(std::size_t i = 0; i < d.agents.size(); ++i)
    {
      bs->Write(d.agents[i].location.x);
      bs->Write(d.agents[i].location.y);
      bs->Write(d.agents[i].location.z);

      bs->Write(d.agents[i].rotation.x);
      bs->Write(d.agents[i].rotation.y);
      bs->Write(d.agents[i].rotation.z);

      bs->Write(d.agents[i].name);
      bs->Write(d.agents[i].id);
    }

  return true;
}

bool deserialize(RakNet::BitStream* bs, data& d)
{
  RakNet::MessageID typeId;
  bs->Read(typeId);

  unsigned short agents_size;
  bs->Read(agents_size);

  for(std::size_t i = 0; i < agents_size; ++i)
    {
      agent newAgent;
      bs->Read(newAgent.location.x);
      bs->Read(newAgent.location.y);
      bs->Read(newAgent.location.z);

      bs->Read(newAgent.rotation.x);
      bs->Read(newAgent.rotation.y);
      bs->Read(newAgent.rotation.z);

      bs->Read(newAgent.name);
      bs->Read(newAgent.id);
      d.agents.push_back(newAgent);
    }

  return true;
}
