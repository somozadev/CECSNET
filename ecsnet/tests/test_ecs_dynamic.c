// test_ecs_dynamic.c
// Prueba de expansión dinámica de entidades.

#include <stdio.h>
#include "../include/ecs.h"
#include "../include/ecs_internal.h"
#include "../include/ecs_types.h"


int main(void) {
    ecs_t ecs;
    ecs_init(&ecs);

    //Create more entities than initially allowed to force dynamic expansion
    const int num_entities = INITIAL_ENTITY_CAPACITY + 10;
    int created = 0;
    for (int i = 0; i < num_entities; ++i) {
        entity_t e = ecs_create_entity(&ecs);
        if (e != (entity_t)-1) {
            created++;
        }
    }
    printf("%d entities have been created. Final capacity: %zu.\n",
           created, ecs.entity_capacity);

    // Ensure that the counter matches with the active entities
    printf("Registered entities: %u\n", ecs.registered_entities_count);

    // Add a component to each entity
    position_t p = { .x = 0.0f, .y = 0.0f };
    for (int i = 0; i < created; ++i) {
        ecs_add_component(&ecs, i, COMPONENT_POSITION, &p);
    }
    // Verify that the signature is fitted
    int signature_ok = 1;
    for (int i = 0; i < created; ++i) {
        if (!ecs_has_component(&ecs, i, COMPONENT_POSITION)) {
            signature_ok = 0;
            break;
        }
    }
    printf("Every position signature is correct: %s\n", signature_ok ? "yes" : "no");

    return 0;
}