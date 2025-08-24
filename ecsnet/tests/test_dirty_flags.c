// test_dirty_flags.c
// Comprueba el funcionamiento de los flags de suciedad (dirty flags).

#include <stdio.h>
#include "../include/ecs.h"
#include "../include/ecs_internal.h"
#include "../include/ecs_types.h"

static void print_dirty(ecs_t *ecs, entity_t e) {
    dirty_component_t dirty[16];
    int count = ecs_get_dirty_components(ecs, e, dirty);
    printf("Entity %u has %d dirty components:\n", e, count);
    for (int i = 0; i < count; ++i) {
        const char *name = ecs_get_component_name(ecs, dirty[i].component_id);
        printf("  %s\n", name);
    }
}

int main(void) {
    ecs_t ecs;
    ecs_init(&ecs);

    // Creates an entity and adds a position component to it
    entity_t e = ecs_create_entity(&ecs);
    position_t pos = { .x = 0.0f, .y = 0.0f };
    ecs_add_component(&ecs, e, COMPONENT_POSITION, &pos);
    // Dirty flags should be active after adding the component
    print_dirty(&ecs, e);

    // Clear dirty flags and verify
    ecs_clear_component_dirty(&ecs, e, COMPONENT_POSITION);
    print_dirty(&ecs, e);

    // Modify component: velocity
    velocity_t vel = { .x = 5.0f, .y = -2.0f };
    ecs_add_component(&ecs, e, COMPONENT_VELOCITY, &vel);
    print_dirty(&ecs, e);

    // Use ecs_mark_component_dirty
    pos.x = 2.0f;
    ecs_mark_component_dirty(&ecs, e, COMPONENT_POSITION);
    print_dirty(&ecs, e);

    return 0;
}