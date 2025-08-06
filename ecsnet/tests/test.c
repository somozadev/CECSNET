#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "test_ecs.h"

int main()
{
    printf("--- Running ECS Unit Tests ---\n");
    bool result = test_ecs();
    assert(result == true);
    printf("------------------------------\n");
    printf("All tests passed.\n");
    getchar(); // Keeps the console open
    return 0;
}