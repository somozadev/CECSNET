// test_ecs_basic.c
// Prueba básica del ECS: inicialización, creación y destrucción de entidades,
// registro de componentes y sistemas, adición y recuperación de componentes.

#include <stdio.h>
#include <string.h>
#include "../include/ecs.h"
#include "../include/ecs_internal.h"
#include "../include/ecs_types.h"

// Example system , prints entities positions
static void system_print_positions(ecs_t *ecs, float dt) {
    (void)dt;
    for (entity_t e = 0; e < ecs->entity_capacity; ++e) {
        if (!ecs->entities[e].in_use) continue;
        if (ecs_has_component(ecs, e, COMPONENT_POSITION)) {
            position_t *pos = ecs_get_component(ecs, e, COMPONENT_POSITION);
            printf("Entity %u: position (%f, %f)\n", e, pos->x, pos->y);
        }
    }
}

int main(void) {
    ecs_t ecs;
    ecs_init(&ecs);

    // Register an additional system to print positions
    ecs_register_system(&ecs, system_print_positions);

    // Create two entities and add to them position and velocity components
    entity_t e1 = ecs_create_entity(&ecs);
    entity_t e2 = ecs_create_entity(&ecs);
    position_t p1 = { .x = 0.0f, .y = 0.0f };
    velocity_t v1 = { .x = 1.0f, .y = 1.0f };
    ecs_add_component(&ecs, e1, COMPONENT_POSITION, &p1);
    ecs_add_component(&ecs, e1, COMPONENT_VELOCITY, &v1);

    position_t p2 = { .x = 10.0f, .y = 10.0f };
    ecs_add_component(&ecs, e2, COMPONENT_POSITION, &p2);

    printf("\n--- PRE UPDATE ---\n");
    ecs_update(&ecs, 0.0f);

    // Update with dt=1.0 for the movement system to modify position
    ecs_update(&ecs, 1.0f);

    printf("\n--- POST UPDATE (dt=1.0) ---\n");
    ecs_update(&ecs, 0.0f);

    // Check has_component and remove_component
    if (ecs_has_component(&ecs, e2, COMPONENT_VELOCITY)) {
        printf("Entity %u has velocity (wrong)\n", e2);
    } else {
        printf("Entity %u don't have velocity (correct)\n", e2);
    }

    ecs_remove_component(&ecs, e1, COMPONENT_VELOCITY);
    printf("\n--- After removing velocity from the entity %u ---\n", e1);
    ecs_update(&ecs, 0.0f);

    // Destroy entity
    ecs_destroy_entity(&ecs, e2);
    printf("\n--- After destroying the entity %u ---\n", e2);
    ecs_update(&ecs, 0.0f);

    return 0;
}