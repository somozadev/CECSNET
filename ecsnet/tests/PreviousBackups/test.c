#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "test_ecs.h"
#include "test_net.h"

int main()
{
    printf("------------------------------\n");
    printf("--- Running ECS Unit Tests ---\n");
    printf("------------------------------\n");
    bool result = test_ecs();
    assert(result == true);
    printf("------------------------------\n");
    printf("--- Running NET Unit Tests ---\n");
    printf("------------------------------\n");
    result = test_networking();
    assert(result == true);
    printf("------------------------------\n");
    printf("All tests passed.\n");
    getchar(); // Keeps the console open
    return 0;
}

//gcc -Iinclude -o test tests/test.c tests/test_net.c tests/test_ecs.c src/ecs.c src/ecs_builtin.c src/net_socket.c src/connection_manager.c src/protocol_handler.c -lws2_32