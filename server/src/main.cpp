
#include <stdio.h>
#include <chrono>

#include "enet.h"
#include "external/fix_win32_compatibility.h"

bool Running = true;

std::chrono::time_point;

ENetHost* ServerHost = nullptr;


void ServerSetup()
{
    ENetAddress address = { 0 };


    address.host = ENET_HOST_ANY;
    address.port = 2401;
    enet_initialize();

    ServerHost = enet_host_create(&address /* the address to bind the server host to */,
        32      /* allow up to 32 clients and/or outgoing connections */,
        2      /* allow up to 2 channels to be used, 0 and 1 */,
        0      /* assume any amount of incoming bandwidth */,
        0      /* assume any amount of outgoing bandwidth */);
}

void ServerCleanup()
{
    if (ServerHost)
    {
        enet_host_destroy(ServerHost);
        ServerHost = nullptr;
    }

    enet_deinitialize();
}

void ServerNetUpdate()
{

}

int main(int argc, char* argv[])
{
    ServerSetup();

    while (!Running && ServerHost)
    {
        ENetEvent event;

        /* Wait up to 1000 milliseconds for an event. */
        while (enet_host_service(ServerHost, &event, 1000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                printf("A new client connected from %x:%u.\n",
                    event.peer->address.host,
                    event.peer->address.port);

                break;

            case ENET_EVENT_TYPE_RECEIVE:
                printf("A packet of length %u containing %s was received from %s on channel %u.\n",
                    event.packet->dataLength,
                    event.packet->data,
                    event.peer->data,
                    event.channelID);

                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(event.packet);

                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                printf("%s disconnected.\n", event.peer->data);

                /* Reset the peer's client information. */

                event.peer->data = nullptr;
            }
        }
    }

    ServerCleanup();
    return 0;
}