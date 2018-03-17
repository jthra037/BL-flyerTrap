#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "BitStream.h"

#include <iostream>
#include <sstream>
#include <thread>
#include <time.h>
#include <chrono>
#include <map>
#include <mutex>

const char* CONNECTION_IP = "192.168.0.105";
static unsigned int SERVER_PORT = 65000;
static unsigned int CLIENT_PORT = 65001;
static unsigned int MAX_CONNECTIONS = 4;

enum NetworkState
{
	NS_Init = 0,
	NS_PendingStart,
	NS_Started,
	NS_Lobby,
	NS_Pending,
	NS_InactiveTurn,
	NS_ActiveTurn,
	NS_Dead,
};

bool isServer = false;
bool isRunning = true;

RakNet::RakPeerInterface *g_rakPeerInterface = nullptr;
RakNet::SystemAddress g_serverAddress;

std::mutex g_networkState_mutex;
NetworkState g_networkState = NS_Init;

// Our internal network Packet ID's, somehow synced with RakNet stuff
enum {
	ID_THEGAME_LOBBY_READY = ID_USER_PACKET_ENUM,
	ID_PLAYER_READY,
	ID_THEGAME_START,
	ID_ACTION,
	ID_STAT,
	ID_SERVERNOTIFICATION,
};

enum EServerNotices
{
	Message = 0,
	Death,
	Activation,
};

enum EPlayerClass
{
	Mage = 0,
	Rogue,
	Fighter,
};

struct SPlayer
{
	std::string m_name;
	unsigned int m_health;
	EPlayerClass m_class;

	RakNet::SystemAddress m_systemAddress;

	//function to send a packet with name/health/class etc
	void SendName(RakNet::SystemAddress systemAddress, bool isBroadcast)
	{
		RakNet::BitStream writeBs;
		writeBs.Write((RakNet::MessageID)ID_PLAYER_READY);
		RakNet::RakString name(m_name.c_str());
		writeBs.Write(name);

		//returns 0 when something is wrong
		assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, systemAddress, isBroadcast));
	}

	void Notify(RakNet::SystemAddress systemAddress, bool isBroadcast, EServerNotices notice, const char* msg = "")
	{
		RakNet::BitStream writeBs;
		writeBs.Write((RakNet::MessageID)ID_SERVERNOTIFICATION);
		writeBs.Write(notice);
		RakNet::RakString message(msg);
		writeBs.Write(message);

		//returns 0 when something is wrong
		assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, systemAddress, isBroadcast));
	}

	void Notify(RakNet::SystemAddress systemAddress, bool isBroadcast, EServerNotices notice, std::string msg = "")
	{
		RakNet::BitStream writeBs;
		writeBs.Write((RakNet::MessageID)ID_SERVERNOTIFICATION);
		writeBs.Write(notice);
		RakNet::RakString message(msg.c_str());
		writeBs.Write(message);

		//returns 0 when something is wrong
		assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, systemAddress, isBroadcast));
	}

	void Notify(bool isBroadcast, EServerNotices notice, std::string msg = "")
	{
		RakNet::BitStream writeBs;
		writeBs.Write((RakNet::MessageID)ID_SERVERNOTIFICATION);
		writeBs.Write(notice);
		RakNet::RakString message(msg.c_str());
		writeBs.Write(message);

		//returns 0 when something is wrong
		assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, m_systemAddress, isBroadcast));
	}
};

unsigned long activePlayer = -1;
std::map<unsigned long, SPlayer> m_players;

// Do something with the guid of the packet and a map??
SPlayer& GetPlayer(RakNet::RakNetGUID raknetId)
{
	unsigned long guid = RakNet::RakNetGUID::ToUint32(raknetId);
	std::map<unsigned long, SPlayer>::iterator it = m_players.find(guid);
	assert(it != m_players.end());
	return it->second;
}

void OnLostConnection(RakNet::Packet* packet)
{
	SPlayer& lostPlayer = GetPlayer(packet->guid);
	lostPlayer.SendName(RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
	unsigned long keyVal = RakNet::RakNetGUID::ToUint32(packet->guid);
	m_players.erase(keyVal);
}

//server
void OnIncomingConnection(RakNet::Packet* packet)
{
	//must be server in order to recieve connection
	assert(isServer);
	m_players.insert(std::make_pair(RakNet::RakNetGUID::ToUint32(packet->guid), SPlayer()));
	std::cout << "Total Players: " << m_players.size() << std::endl;
}

//client
void OnConnectionAccepted(RakNet::Packet* packet)
{
	//server should not ne connecting to anybody, 
	//clients connect to server
	assert(!isServer);
	g_networkState_mutex.lock();
	g_networkState = NS_Lobby;
	g_networkState_mutex.unlock();
	g_serverAddress = packet->systemAddress;
}

//this is on the client side
void DisplayPlayerReady(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::RakString userName;
	bs.Read(userName);

	std::cout << userName.C_String() << " has joined" << std::endl;
}

std::string GetPlayerName(unsigned int guid)
{
	return m_players.at(guid).m_name;
}

std::string GetTargetList()
{
	std::string response;
	int i = 1;
	for (std::map<unsigned long, SPlayer>::iterator it = m_players.begin(); it != m_players.end(); ++it)
	{
		std::string index = std::to_string(i);
		std::string name = it->second.m_name;
		response.append(index).append(") ").append(name).append("\n");
		i++;
	}
	
	return response;
}

void NextActivePlayerGUID()
{
	// find the player and move the iterator by one
	std::map<unsigned long, SPlayer>::iterator it = m_players.find(activePlayer); 

	if (++it == m_players.end())
	{
		it = m_players.begin();
		std::cout << "Iterator wrapping back to start for next player!" << std::endl;
	}

	activePlayer = it->first;
}

unsigned long PlayerByIndex(int idx)
{
	unsigned int found = -1;
	int i = 1;
	for (std::map<unsigned long, SPlayer>::iterator it = m_players.begin(); it != m_players.end(); ++it)
	{
		if (i == idx)
		{
			return it->first;
		}

		i++;
	}

	return found;
}

void OnLobbyReady(RakNet::Packet* packet)
{
	// Make a bitstream out of the packet we got
	RakNet::BitStream bs(packet->data, packet->length, false);
	// Read stuff out of the packet in the same order it was written in
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::RakString userName;
	bs.Read(userName);
	int idx;
	bs.Read(idx);

	EPlayerClass classSelection;
	std::string classToPrint;
	switch (idx)
	{
	case 1:
		classSelection = Mage;
		classToPrint = "Mage";
		break;
	case 2:
		classSelection = Rogue;
		classToPrint = "Rogue";
		break;
	case 3:
		classSelection = Fighter;
		classToPrint = "Fighter";
		break;
	default:
		classSelection = Mage;
		classToPrint = "Mage";
		break;
	}

	// Get guid from this packet
	unsigned long guid = RakNet::RakNetGUID::ToUint32(packet->guid);
	SPlayer& player = GetPlayer(packet->guid); // somehow get a reference to the player who sent the message based on the guid
	player.m_name = userName; // Set this players username to the one the user sent in the packet
	player.m_class = classSelection;
	player.m_health = 20;
	player.m_systemAddress = packet->systemAddress;
	std::cout << player.m_name << " the " << classToPrint << " IS READY!!!!!" << std::endl; // Let the user running the server know that this new player is ready

	//notify all other connected players that this plyer has joined the game
	for (std::map<unsigned long, SPlayer>::iterator it = m_players.begin(); it != m_players.end(); ++it)
	{
		//skip over the player who just joined
		if (guid == it->first)
		{
			continue;
		}

		SPlayer& player = it->second;
		player.SendName(packet->systemAddress, false);
	}

	player.SendName(packet->systemAddress, true);

	if (activePlayer == -1 && m_players.size() > 1)
	{
		activePlayer = m_players.begin()->first;
		player.Notify(packet->systemAddress, true, Message, "Game is on!");
		
		std::string msg = "You are the active player.\n";
		msg.append("Please select a target (if you select yourself you heal)\n");
		msg.append(GetTargetList());
		m_players.at(activePlayer).Notify(packet->systemAddress, false, Activation, msg);
		
	}
	else if (activePlayer != -1)
	{
		std::string msg = m_players.at(activePlayer).m_name;
		msg.append(" is the active player.");
		player.Notify(packet->systemAddress, false, Message, msg);
		m_players.at(activePlayer).Notify(false, Message, GetTargetList());
	}
	
}

std::string ConstructStats()
{
	std::string response = "It's ";
	response.append(m_players.at(activePlayer).m_name);
	response.append("'s turn.\n");
	for (const auto& itPair : m_players)
	{
		response.append(itPair.second.m_name);
		response.append(" has ");
		response.append(std::to_string(itPair.second.m_health));
		response.append(" hp.\n");
	}

	return response;
}

void ReturnStats(RakNet::Packet* packet)
{
	// Make a bitstream out of the packet we got
	RakNet::BitStream bs(packet->data, packet->length, false);
	// Read stuff out of the packet in the same order it was written in
	RakNet::MessageID messageId;
	bs.Read(messageId);

	RakNet::RakString query;
	bs.Read(query);

	if (query.C_String()[0] == '?')
	{
		GetPlayer(packet->guid).Notify(false, Message, ConstructStats());
	}
	else
	{
		GetPlayer(packet->guid).Notify(false, Message, "Invalid server command.");
	}
}

/// This will take care of attacks sent to server by players
void HandleAction(RakNet::Packet *packet)
{
	// Make a bitstream out of the packet we got
	RakNet::BitStream bs(packet->data, packet->length, false);
	// Read stuff out of the packet in the same order it was written in
	RakNet::MessageID messageId;
	bs.Read(messageId);
	
	unsigned long guid = RakNet::RakNetGUID::ToUint32(packet->guid);
	
	if (guid != activePlayer)
	{
		ReturnStats(packet);
		return; // go no further
	}

	RakNet::RakString selection;
	bs.Read(selection);

	unsigned long selectedGUID = PlayerByIndex(std::atoi(&selection.C_String()[0]));

	if (selectedGUID == -1)
	{
		m_players.at(guid).Notify(false, Message, "Invalid target!");
	}
	else if (selectedGUID == guid)
	{
		int hp = rand() % 5 + 1;
		m_players.at(selectedGUID).m_health += hp;

		std::string msg = m_players.at(guid).m_name;
		msg.append(" healed by  ");
		msg.append(std::to_string(hp));
		msg.append(" and now has: ");
		msg.append(std::to_string(m_players.at(guid).m_health));
		m_players.at(guid).Notify(true, Message, msg);
		m_players.at(guid).Notify(false, Message, msg);
	}
	else
	{
		int dmg = rand() % 5 + 1;
		m_players.at(selectedGUID).m_health -= dmg;

		std::string msg = m_players.at(selectedGUID).m_name;
		msg.append(" took ");
		msg.append(std::to_string(dmg));
		msg.append(" and now has: ");
		msg.append(std::to_string(m_players.at(selectedGUID).m_health));
		m_players.at(guid).Notify(true, Message, msg);
		m_players.at(guid).Notify(false, Message, msg);

		if (m_players.at(selectedGUID).m_health <= 0 || m_players.at(selectedGUID).m_health > 1000)
		{
			std::string msg = m_players.at(selectedGUID).m_name;
			msg.append(" has died!");
			std::cout << msg << std::endl;
			m_players.at(selectedGUID).Notify(false, Death, "You dead!");
			m_players.at(selectedGUID).Notify(true, Message, msg);
			m_players.erase(selectedGUID);
		}
	}

	NextActivePlayerGUID();
	std::string msg = "You are the active player.\n";
	msg.append("Please select a target (if you select yourself you heal)\n");
	msg.append(GetTargetList());
	m_players.at(activePlayer).Notify(false, Activation, msg);
}

/// This takes messages from the server and does things with them
void HandleServerNotification(RakNet::Packet *packet)
{
	// Make a bitstream out of the packet we got
	RakNet::BitStream bs(packet->data, packet->length, false);
	// Read stuff out of the packet in the same order it was written in
	RakNet::MessageID messageId;
	bs.Read(messageId);
	
	EServerNotices notice;
	bs.Read(notice);
	
	char msg [256];
	bs.Read(msg);

	switch (notice)
	{
	case Message:
		std::cout << msg << std::endl;
		break;
	case Death:
		std::cout << msg << std::endl;
		g_networkState = NS_Dead;
		break;
	case Activation:
		std::cout << msg << std::endl;
		g_networkState = NS_ActiveTurn;
		break;
	default:
		break;
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


void InputHandler()
{
	// While the game is on
	while (isRunning)
	{
		// make a buffer
		char userInput[255];

		// If the network is initializing still
		if (g_networkState == NS_Init)
		{
			// Prompt user for input
			std::cout << "press (s) for server (c) for client" << std::endl;
			std::cin >> userInput;
			isServer = (userInput[0] == 's'); // This instance is requesting to be server
			// Always change states while accounting for asynchronous changes
			g_networkState_mutex.lock();
			g_networkState = NS_PendingStart;
			g_networkState_mutex.unlock();

		}
		// The lobby has been started
		else if (g_networkState == NS_Lobby)
		{
			// Take user input for name
			std::cout << "Enter your name to play or type quit to leave" << std::endl;
			std::cin >> userInput;
			//quitting is not acceptable in our game, create a crash to teach lesson
			assert(strcmp(userInput, "quit"));

			std::cout << "Choose your class:\n1) Mage\n2) Rogue\n3) Fighter\n>> ";
			int idx;
			if (!(std::cin >> idx))
			{
				idx = 1;
				std::cin.clear();
			}

			// Make a bitstream
			RakNet::BitStream bs;
			// Write a RakNet message to the bitstream about an enum value from RakNet somewhere
			bs.Write((RakNet::MessageID)ID_THEGAME_LOBBY_READY);
			// Convert character buffer to RakString?
			RakNet::RakString name(userInput);
			// Also write our users name to the bitstream
			bs.Write(name);
			bs.Write(idx);

			//returns 0 when something is wrong
			assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false)); // Send our bitstream to the server, crash if it doesn't work

			// Change states safely
			g_networkState_mutex.lock();
			g_networkState = NS_Pending;
			g_networkState_mutex.unlock();
		}
		// If we are pending
		else if (g_networkState == NS_Pending)
		{
			// Tell the user we're pending, but only once.
			static bool doOnce = false;
			if (!doOnce)
				std::cout << "Wait for your turn.\nEnter ? any time to get stats.\n>> ";

			doOnce = true;
		}
		else if (g_networkState == NS_ActiveTurn)
		{
			std::cout << ">> ";
			char idx[256];
			if (!(std::cin >> idx))
			{
				std::cin.clear();
			}

			// Make a bitstream
			RakNet::BitStream bs;
			// Write a RakNet message to the bitstream about an enum value from RakNet somewhere
			bs.Write((RakNet::MessageID)ID_ACTION);
			// Convert character buffer to RakString?
			RakNet::RakString thing(idx);
			bs.Write(thing);

			//returns 0 when something is wrong
			assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false)); // Send our bitstream to the server, crash if it doesn't work

			// Change states safely
			//g_networkState_mutex.lock();
			//g_networkState = NS_Pending;
			//g_networkState_mutex.unlock();
		}
		else if (g_networkState == NS_Dead)
		{
			// We gonna figure out if the player is dead
			// Tell the user we're pending, but only once.
			std::cout << "You're dead, goodbye." << std::endl;
			isRunning = false;
			return;
		}
		// Sleep this thread to keep everything responsive
		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
}

bool HandleLowLevelPackets(RakNet::Packet* packet)
{
	bool isHandled = true;
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
		printf("ID_ALREADY_CONNECTED with guid %" PRINTF_64_BIT_MODIFIER "u\n", packet->guid);
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
	case ID_NEW_INCOMING_CONNECTION:
		//client connecting to server
		OnIncomingConnection(packet);
		printf("ID_NEW_INCOMING_CONNECTION\n");
		break;
	case ID_REMOTE_NEW_INCOMING_CONNECTION: // Server telling the clients of another client connecting.  You can manually broadcast this in a peer to peer enviroment if you want.
		OnIncomingConnection(packet);
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
		OnLostConnection(packet);
		break;

	case ID_CONNECTION_REQUEST_ACCEPTED:
		// This tells the client they have connected
		printf("ID_CONNECTION_REQUEST_ACCEPTED to %s with GUID %s\n", packet->systemAddress.ToString(true), packet->guid.ToString());
		printf("My external address is %s\n", g_rakPeerInterface->GetExternalID(packet->systemAddress).ToString(true));
		OnConnectionAccepted(packet);
		break;
	case ID_CONNECTED_PING:
	case ID_UNCONNECTED_PING:
		printf("Ping from %s\n", packet->systemAddress.ToString(true));
		break;
	default:
		isHandled = false;
		break;
	}
	return isHandled;
}

void PacketHandler()
{
	// While this instance is running
	while (isRunning)
	{
		// Read available packets?
		for (RakNet::Packet* packet = g_rakPeerInterface->Receive(); packet != nullptr; g_rakPeerInterface->DeallocatePacket(packet), packet = g_rakPeerInterface->Receive())
		{
			// If this packet was not for RakNet itself, it's probably for our network
			if (!HandleLowLevelPackets(packet))
			{
				//our game specific packets
				unsigned char packetIdentifier = GetPacketIdentifier(packet);

				// Go different places and do things based on what the packet was identified as
				switch (packetIdentifier)
				{
				case ID_THEGAME_LOBBY_READY:
					OnLobbyReady(packet);
					break;
				case ID_PLAYER_READY:
					DisplayPlayerReady(packet);
					break;
				case ID_ACTION:
					HandleAction(packet);
					break;
				case ID_STAT:
					ReturnStats(packet);
					break;
				case ID_SERVERNOTIFICATION:
					HandleServerNotification(packet);
					break;
				default:
					break;
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
}

int main()
{
	// Make a raknet interface
	g_rakPeerInterface = RakNet::RakPeerInterface::GetInstance();

	// start threads for input, and packet reading
	std::thread inputHandler(InputHandler);
	std::thread packetHandler(PacketHandler);

	srand(time(NULL));

	// until we turn off the game...
	while (isRunning)
	{
		// Is the network pending?
		if (g_networkState == NS_PendingStart)
		{
			// Am I the server?
			if (isServer)
			{
				// Set up the socket 
				RakNet::SocketDescriptor socketDescriptors[1];
				socketDescriptors[0].port = SERVER_PORT;
				socketDescriptors[0].socketFamily = AF_INET; // Test out IPV4

				// Connection on that socket worked
				bool isSuccess = g_rakPeerInterface->Startup(MAX_CONNECTIONS, socketDescriptors, 1) == RakNet::RAKNET_STARTED;
				assert(isSuccess); // maybe it didn't work...

				//ensures we are server
				g_rakPeerInterface->SetMaximumIncomingConnections(MAX_CONNECTIONS);
				std::cout << "server started" << std::endl; // Let the user running server know it's on
				
				// Lock the network, changes its state, then unlock it again to prevent asynchronous data changes
				g_networkState_mutex.lock();
				g_networkState = NS_Started;
				g_networkState_mutex.unlock();
			}
			// This instance is a client
			else
			{
				// Set up socket
				RakNet::SocketDescriptor socketDescriptor(CLIENT_PORT, 0);
				socketDescriptor.socketFamily = AF_INET;

				// Set up each client instance with a new unused port
				while (RakNet::IRNS2_Berkley::IsPortInUse(socketDescriptor.port, socketDescriptor.hostAddress, socketDescriptor.socketFamily, SOCK_DGRAM) == true)
					socketDescriptor.port++;

				// Check the result of connection on this socket 
				RakNet::StartupResult result = g_rakPeerInterface->Startup(8, &socketDescriptor, 1);
				assert(result == RakNet::RAKNET_STARTED); // Maybe it didn't work

				// Lock and change state to prevent asynchronous changed, then unlock again
				g_networkState_mutex.lock();
				g_networkState = NS_Started;
				g_networkState_mutex.unlock();

				// A setting to check once in a while if this instance is still connected to network
				g_rakPeerInterface->SetOccasionalPing(true);
				//"127.0.0.1" = local host = your machines address
				// Try to connect to server (using home local IP or generic IP, whichever seems to work)
				RakNet::ConnectionAttemptResult car = g_rakPeerInterface->Connect(CONNECTION_IP, SERVER_PORT, nullptr, 0);
				RakAssert(car == RakNet::CONNECTION_ATTEMPT_STARTED); // hopefully this works
				std::cout << "client attempted connection..." << std::endl; // Let the user know what is happening

			}
		}

	}

	//std::cout << "press q and then return to exit" << std::endl;
	//std::cin >> userInput;


	// Join the threads so we can exit cleanly
	inputHandler.join();
	packetHandler.join();
	return 0;
}