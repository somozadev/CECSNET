#include "network_architecture.h"
#include "protocol_handler.h"

#include <string.h>


void ecs_network_architecture_init()
{

}

network_architecture_t create_client_server_architecture()
{
    static network_architecture_client_server_t client_server = {0};
    network_architecture_t network_interface = { 
        .initialize = client_server_init,
        .update = client_server_update,
        .send_data=NULL,
        .receive_data=NULL,
        .implemented_architecture = &client_server
    };
    return network_interface;
}