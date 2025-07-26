#include "ecsnet.h"
#include <stdio.h>

int main() {
    ecs_init();

    entity_t entity_test = ecs_create_entity();
    position_t pos = {.x = 0.0f, .y = 0.0f};
    ecs_add_component(entity_test, COMPONENT_POSITION, &pos);
    position_t *position_out = ecs_get_component(entity_test, COMPONENT_POSITION);
    ++position_out->y;
    printf("Position: (%f, %f)\n", position_out->x, position_out->y);
    ++position_out->x;
    printf("Position: (%f, %f)\n", position_out->x, position_out->y);

    return true;
}
