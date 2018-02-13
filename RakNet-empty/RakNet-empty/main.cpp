
#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"

#include <iostream>
#include <thread>         // std::thread

static int SERVER_PORT = 65000;
static int CLIENT_PORT = 65001;
static int MAX_CONNECTIONS = 4;

RakNet::RakPeerInterface *g_rakPeerInterface = nullptr;

bool isServer = false;
bool isRunning = true;

enum NetWorkStates
{
	NS_Decision = 0, 
	NS_CreateSocket,
	NS_PendingConnections,
	NS_Connected,
	NS_Running
};

NetWorkStates g_networkState = NS_Decision;

void InputHandler()
{
	while (isRunning)
	{
		char userInput[255];
		switch (g_networkState)
		{
		case NS_Decision:
			std::cout << "Press (s) for server, (c) for client" << std::endl;
			std::cin >> userInput;
			isServer = userInput[0] == 's';
			g_networkState = NS_CreateSocket;
			break;
		case NS_CreateSocket:
			if (isServer)
			{
				std::cout << "Server creating socket..." << std::endl;
			}
			else
			{
				std::cout << "Client creating socket..." << std::endl;
			}
			break;
		case NS_Connected:
			break;
		case NS_Running:
			break;
		default:
			break;
		}
	}
}

unsigned char GetPacketIdentifier(RakNet::Packet *packet)
{
	if (packet == nullptr)
		return 255;

	if ((unsigned char)packet->data[0] == ID_TIMESTAMP)
	{
		RakAssert(packet->length > sizeof(RakNet::MessageID) + sizeof(RakNet::Time));
		return (unsigned char)packet->data[sizeof(RakNet::MessageID) + sizeof(RakNet::Time)];
	}
	else
		return (unsigned char)packet->data[0];
}

void PacketHandler()
{
	while (isRunning)
	{
		for (RakNet::Packet* packet = g_rakPeerInterface->Receive(); packet != nullptr; g_rakPeerInterface->DeallocatePacket(packet), packet = g_rakPeerInterface->Receive())
		{
			// We got a packet, get the identifier with our handy function
			unsigned char packetIdentifier = GetPacketIdentifier(packet);

			// Check if this is a network message packet
			switch (packetIdentifier)
			{
			case ID_DISCONNECTION_NOTIFICATION:
				// Connection lost normally
				printf("ID_DISCONNECTION_NOTIFICATION\n");
				break;
			case ID_ALREADY_CONNECTED:
				// Connection lost normally
				printf("ID_ALREADY_CONNECTED with guid %" ,PRINTF_64_BIT_MODIFIER, "u\n", packet->guid);
				break;
			case ID_INCOMPATIBLE_PROTOCOL_VERSION:
				printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
				break;
			case ID_REMOTE_DISCONNECTION_NOTIFICATION: // Server telling the clients of another client disconnecting gracefully.  You can manually broadcast this in a peer to peer enviroment if you want.
				printf("ID_REMOTE_DISCONNECTION_NOTIFICATION\n");
				break;
			case ID_REMOTE_CONNECTION_LOST: // Server telling the clients of another client disconnecting forcefully.  You can manually broadcast this in a peer to peer enviroment if you want.
				printf("ID_REMOTE_CONNECTION_LOST\n");
				break;
			case ID_REMOTE_NEW_INCOMING_CONNECTION: // Server telling the clients of another client connecting.  You can manually broadcast this in a peer to peer enviroment if you want.
				printf("ID_REMOTE_NEW_INCOMING_CONNECTION\n");
				break;
			case ID_CONNECTION_BANNED: // Banned from this server
				printf("We are banned from this server.\n");
				break;
			case ID_CONNECTION_ATTEMPT_FAILED:
				printf("Connection attempt failed\n");
				break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				// Sorry, the server is full.  I don't do anything here but
				// A real app should tell the user
				printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
				break;

			case ID_INVALID_PASSWORD:
				printf("ID_INVALID_PASSWORD\n");
				break;

			case ID_CONNECTION_LOST:
				// Couldn't deliver a reliable packet - i.e. the other system was abnormally
				// terminated
				printf("ID_CONNECTION_LOST\n");
				break;

			case ID_CONNECTION_REQUEST_ACCEPTED:
				// This tells the client they have connected
				printf("ID_CONNECTION_REQUEST_ACCEPTED to %s with GUID %s\n", packet->systemAddress.ToString(true), packet->guid.ToString());
				printf("My external address is %s\n", g_rakPeerInterface->GetExternalID(packet->systemAddress).ToString(true));
				break;
			case ID_CONNECTED_PING:
			case ID_UNCONNECTED_PING:
				printf("Ping from %s\n", packet->systemAddress.ToString(true));
				break;
			default:
				// It's a client, so just show the message
				printf("%s\n", packet->data);
				break;
			}
		}
	}
}

int main()
{
	g_rakPeerInterface = RakNet::RakPeerInterface::GetInstance();

	std::thread inputHandler(InputHandler);
	std::thread packetHandler(PacketHandler);

	while (isRunning)
	{
		if (isServer &&
			g_networkState == NS_CreateSocket)
		{
			//opening up the server socket
			RakNet::SocketDescriptor socketDescriptors[1];
			socketDescriptors[0].port = SERVER_PORT;
			socketDescriptors[0].socketFamily = AF_INET; // Test out IPV4
			bool isSuccess = g_rakPeerInterface->Startup(MAX_CONNECTIONS, socketDescriptors, 1) == RakNet::RAKNET_STARTED;
			assert(isSuccess);
			g_rakPeerInterface->SetMaximumIncomingConnections(MAX_CONNECTIONS);
			g_networkState = NS_PendingConnections;
			/*
			std::cout << "server is up and running. press a key and then return key to exit" << std::endl;
			std::cin >> userInput;
			*/
		}
		else if (g_networkState == NS_CreateSocket)
		{
			RakNet::SocketDescriptor socketDescriptor(CLIENT_PORT, nullptr);
			socketDescriptor.socketFamily = AF_INET;
			g_rakPeerInterface->Startup(8, &socketDescriptor, 1);

			// client connections
			RakNet::ConnectionAttemptResult car = g_rakPeerInterface->Connect("127.0.0.1", SERVER_PORT, nullptr, 0);
			//RakAssert(car == RakNet::ConnectionAttemptResult);

		}
	}

	inputHandler.join();
	packetHandler.join();
	return 0;
}

//#include "RakNetStatistics.h"
//#include "RakNetTypes.h"
//#include "BitStream.h"
//#include "RakSleep.h"
//#include "PacketLogger.h"
//#include <assert.h>
//#include <cstdio>
//#include <cstring>
//#include <stdlib.h>
//#include "Kbhit.h"
//#include <stdio.h>
//#include <string.h>
//#include "Gets.h"
/*

#if LIBCAT_SECURITY==1
#include "SecureHandshake.h" // Include header for secure handshake
#endif

#if defined(_CONSOLE_2)
#include "Console2SampleIncludes.h"
#endif

// We copy this from Multiplayer.cpp to keep things all in one file for this example
unsigned char GetPacketIdentifier(RakNet::Packet *p);

#ifdef _CONSOLE_2
_CONSOLE_2_SetSystemProcessParams
#endif

int main(void)
{
// Pointers to the interfaces of our server and client.
// Note we can easily have both in the same program
RakNet::RakPeerInterface *server = RakNet::RakPeerInterface::GetInstance();
RakNet::RakNetStatistics *rss;
server->SetIncomingPassword("Rumpelstiltskin", (int)strlen("Rumpelstiltskin"));
server->SetTimeoutTime(30000, RakNet::UNASSIGNED_SYSTEM_ADDRESS);
//	RakNet::PacketLogger packetLogger;
//	server->AttachPlugin(&packetLogger);

#if LIBCAT_SECURITY==1
cat::EasyHandshake handshake;
char public_key[cat::EasyHandshake::PUBLIC_KEY_BYTES];
char private_key[cat::EasyHandshake::PRIVATE_KEY_BYTES];
handshake.GenerateServerKey(public_key, private_key);
server->InitializeSecurity(public_key, private_key, false);
FILE *fp = fopen("publicKey.dat", "wb");
fwrite(public_key, sizeof(public_key), 1, fp);
fclose(fp);
#endif

// Holds packets
RakNet::Packet* p;

// GetPacketIdentifier returns this
unsigned char packetIdentifier;

// Record the first client that connects to us so we can pass it to the ping function
RakNet::SystemAddress clientID = RakNet::UNASSIGNED_SYSTEM_ADDRESS;

// Holds user data
char portstring[30];

printf("This is a sample implementation of a text based chat server.\n");
printf("Connect to the project 'Chat Example Client'.\n");
printf("Difficulty: Beginner\n\n");

// A server
puts("Enter the server port to listen on");
Gets(portstring, sizeof(portstring));
if (portstring[0] == 0)
strcpy(portstring, "1234");

puts("Starting server.");
// Starting the server is very simple.  2 players allowed.
// 0 means we don't care about a connectionValidationInteger, and false
// for low priority threads
// I am creating two socketDesciptors, to create two sockets. One using IPV6 and the other IPV4
RakNet::SocketDescriptor socketDescriptors[2];
socketDescriptors[0].port = atoi(portstring);
socketDescriptors[0].socketFamily = AF_INET; // Test out IPV4
socketDescriptors[1].port = atoi(portstring);
socketDescriptors[1].socketFamily = AF_INET6; // Test out IPV6
bool b = server->Startup(4, socketDescriptors, 2) == RakNet::RAKNET_STARTED;
server->SetMaximumIncomingConnections(4);
if (!b)
{
printf("Failed to start dual IPV4 and IPV6 ports. Trying IPV4 only.\n");

// Try again, but leave out IPV6
b = server->Startup(4, socketDescriptors, 1) == RakNet::RAKNET_STARTED;
if (!b)
{
puts("Server failed to start.  Terminating.");
exit(1);
}
}
server->SetOccasionalPing(true);
server->SetUnreliableTimeout(1000);

DataStructures::List< RakNet::RakNetSocket2* > sockets;
server->GetSockets(sockets);
printf("Socket addresses used by RakNet:\n");
for (unsigned int i = 0; i < sockets.Size(); i++)
{
printf("%i. %s\n", i + 1, sockets[i]->GetBoundAddress().ToString(true));
}

printf("\nMy IP addresses:\n");
for (unsigned int i = 0; i < server->GetNumberOfAddresses(); i++)
{
RakNet::SystemAddress sa = server->GetInternalID(RakNet::UNASSIGNED_SYSTEM_ADDRESS, i);
printf("%i. %s (LAN=%i)\n", i + 1, sa.ToString(false), sa.IsLANAddress());
}

printf("\nMy GUID is %s\n", server->GetGuidFromSystemAddress(RakNet::UNASSIGNED_SYSTEM_ADDRESS).ToString());
puts("'quit' to quit. 'stat' to show stats. 'ping' to ping.\n'pingip' to ping an ip address\n'ban' to ban an IP from connecting.\n'kick to kick the first connected player.\nType to talk.");
char message[2048];

// Loop for input
while (1)
{

// This sleep keeps RakNet responsive
RakSleep(30);

if (kbhit())
{
// Notice what is not here: something to keep our network running.  It's
// fine to block on gets or anything we want
// Because the network engine was painstakingly written using threads.
Gets(message, sizeof(message));

if (strcmp(message, "quit") == 0)
{
puts("Quitting.");
break;
}

if (strcmp(message, "stat") == 0)
{
rss = server->GetStatistics(server->GetSystemAddressFromIndex(0));
StatisticsToString(rss, message, 2);
printf("%s", message);
printf("Ping %i\n", server->GetAveragePing(server->GetSystemAddressFromIndex(0)));

continue;
}

if (strcmp(message, "ping") == 0)
{
server->Ping(clientID);

continue;
}

if (strcmp(message, "pingip") == 0)
{
printf("Enter IP: ");
Gets(message, sizeof(message));
printf("Enter port: ");
Gets(portstring, sizeof(portstring));
if (portstring[0] == 0)
strcpy(portstring, "1234");
server->Ping(message, atoi(portstring), false);

continue;
}

if (strcmp(message, "kick") == 0)
{
server->CloseConnection(clientID, true, 0);

continue;
}

if (strcmp(message, "getconnectionlist") == 0)
{
RakNet::SystemAddress systems[10];
unsigned short numConnections = 10;
server->GetConnectionList((RakNet::SystemAddress*) &systems, &numConnections);
for (int i = 0; i < numConnections; i++)
{
printf("%i. %s\n", i + 1, systems[i].ToString(true));
}
continue;
}

if (strcmp(message, "ban") == 0)
{
printf("Enter IP to ban.  You can use * as a wildcard\n");
Gets(message, sizeof(message));
server->AddToBanList(message);
printf("IP %s added to ban list.\n", message);

continue;
}


// Message now holds what we want to broadcast
char message2[2048];
// Append Server: to the message so clients know that it ORIGINATED from the server
// All messages to all clients come from the server either directly or by being
// relayed from other clients
message2[0] = 0;
const static char prefix[] = "Server: ";
strncpy(message2, prefix, sizeof(message2));
strncat(message2, message, sizeof(message2) - strlen(prefix) - 1);

// message2 is the data to send
// strlen(message2)+1 is to send the null terminator
// HIGH_PRIORITY doesn't actually matter here because we don't use any other priority
// RELIABLE_ORDERED means make sure the message arrives in the right order
// We arbitrarily pick 0 for the ordering stream
// RakNet::UNASSIGNED_SYSTEM_ADDRESS means don't exclude anyone from the broadcast
// true means broadcast the message to everyone connected
server->Send(message2, (const int)strlen(message2) + 1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}

// Get a packet from either the server or the client

for (p = server->Receive(); p; server->DeallocatePacket(p), p = server->Receive())
{
// We got a packet, get the identifier with our handy function
packetIdentifier = GetPacketIdentifier(p);

// Check if this is a network message packet
switch (packetIdentifier)
{
case ID_DISCONNECTION_NOTIFICATION:
// Connection lost normally
printf("ID_DISCONNECTION_NOTIFICATION from %s\n", p->systemAddress.ToString(true));;
break;


case ID_NEW_INCOMING_CONNECTION:
// Somebody connected.  We have their IP now
printf("ID_NEW_INCOMING_CONNECTION from %s with GUID %s\n", p->systemAddress.ToString(true), p->guid.ToString());
clientID = p->systemAddress; // Record the player ID of the client

printf("Remote internal IDs:\n");
for (int index = 0; index < MAXIMUM_NUMBER_OF_INTERNAL_IDS; index++)
{
RakNet::SystemAddress internalId = server->GetInternalID(p->systemAddress, index);
if (internalId != RakNet::UNASSIGNED_SYSTEM_ADDRESS)
{
printf("%i. %s\n", index + 1, internalId.ToString(true));
}
}

break;

case ID_INCOMPATIBLE_PROTOCOL_VERSION:
printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
break;

case ID_CONNECTED_PING:
case ID_UNCONNECTED_PING:
printf("Ping from %s\n", p->systemAddress.ToString(true));
break;

case ID_CONNECTION_LOST:
// Couldn't deliver a reliable packet - i.e. the other system was abnormally
// terminated
printf("ID_CONNECTION_LOST from %s\n", p->systemAddress.ToString(true));;
break;

default:
// The server knows the static data of all clients, so we can prefix the message
// With the name data
printf("%s\n", p->data);

// Relay the message.  We prefix the name for other clients.  This demonstrates
// That messages can be changed on the server before being broadcast
// Sending is the same as before
sprintf(message, "%s", p->data);
server->Send(message, (const int)strlen(message) + 1, HIGH_PRIORITY, RELIABLE_ORDERED, 0, p->systemAddress, true);

break;
}

}
}

server->Shutdown(300);
// We're done with the network
RakNet::RakPeerInterface::DestroyInstance(server);

return 0;
}

// Copied from Multiplayer.cpp
// If the first byte is ID_TIMESTAMP, then we want the 5th byte
// Otherwise we want the 1st byte
unsigned char GetPacketIdentifier(RakNet::Packet *p)
{
if (p == 0)
return 255;

if ((unsigned char)p->data[0] == ID_TIMESTAMP)
{
RakAssert(p->length > sizeof(RakNet::MessageID) + sizeof(RakNet::Time));
return (unsigned char)p->data[sizeof(RakNet::MessageID) + sizeof(RakNet::Time)];
}
else
return (unsigned char)p->data[0];
}
*/