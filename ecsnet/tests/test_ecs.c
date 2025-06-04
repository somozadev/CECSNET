#include <stdio.h>
#include "ecs.h"
#include "test_ecs.h"

bool test_ecs()
{
    ecs_init();


    entity_t entity_test = ecs_create_entity();


    position_t pos = {.x = 0.0f, .y = 0.0f};
    ecs_add_component(entity_test, COMPONENT_POSITION, &pos);
    position_t* position_out = ecs_get_component(entity_test, COMPONENT_POSITION);
    ++position_out->y;
    printf("Position: (%f, %f)\n", position_out->x, position_out->y);
    ++position_out->x;
    printf("Position: (%f, %f)\n", position_out->x, position_out->y);
 
    return true;
}

//gcc -Iinclude -o test tests/test.c tests/test_ecs.c src/ecs.c src/ecs_builtin.c