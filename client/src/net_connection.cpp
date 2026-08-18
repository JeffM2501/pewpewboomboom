#include "net_connection.h"

#include "enet.h"

namespace NetConnection
{
	ConnectionState CurentState = ConnectionState::Disconnected;
	ENetHost* ClientHost = nullptr;
	ENetPeer* ServerPeer = nullptr;

	void Init()
	{
		enet_initialize();
	}

	void Shutdown()
	{
		Disconnect();
		enet_deinitialize();
	}

	void BeginConnect(const char* address, uint16_t port)
	{
		ClientHost = enet_host_create(nullptr, 1, 2, 0, 0);
		ENetAddress hostAddress = { 0 };
		enet_address_set_host(&hostAddress, address);
		hostAddress.port = port;
		ServerPeer = enet_host_connect(ClientHost, &hostAddress, 2, 0);

		CurentState = ConnectionState::Connecting;
	}

	void Disconnect()
	{
		if (ServerPeer)
		{
			enet_peer_disconnect(ServerPeer, 0);
			ServerPeer = nullptr;
		}

		if (ClientHost)
		{
			enet_host_destroy(ClientHost);
			ClientHost = nullptr;
		}

		CurentState = ConnectionState::Disconnected;
	}

	ConnectionState GetState()
	{
		return CurentState;
	}

	void Update()
	{
		if (CurentState != ConnectionState::Disconnected)
		{
			ENetEvent event;
			if (enet_host_service(ClientHost, &event, 0) > 0)
			{
				if (event.type == ENET_EVENT_TYPE_CONNECT)
				{
					CurentState = ConnectionState::Connected;
				}
				else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
				{
					CurentState = ConnectionState::Disconnected;
				}
			}
		}
	}
}