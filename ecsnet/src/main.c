#include <stdio.h>
#include <unistd.h> // For sleep on Unix/Linux systems
#include "ecs.h"
#include "ecs_builtin.h"

int main()
{
    printf("Initializing ECS...\n");
    ecs_init();
    ecs_register_builtin_components();
    ecs_register_builtin_systems();

    // --- Create entities and assign components ---

    // Entity 1: A static object
    entity_t static_entity = ecs_create_entity();
    position_t static_pos = {10.0f, 20.0f};
    ecs_add_component(static_entity, COMPONENT_POSITION, &static_pos);
    printf("Entity %d created with position at (%.2f, %.2f)\n", static_entity, static_pos.x, static_pos.y);

    // Entity 2: A moving object
    entity_t moving_entity = ecs_create_entity();
    position_t moving_pos = {0.0f, 0.0f};
    velocity_t moving_vel = {5.0f, 3.0f};
    ecs_add_component(moving_entity, COMPONENT_POSITION, &moving_pos);
    ecs_add_component(moving_entity, COMPONENT_VELOCITY, &moving_vel);
    printf("Entity %d created with position at (%.2f, %.2f) and velocity at (%.2f, %.2f)\n", moving_entity, moving_pos.x, moving_pos.y, moving_vel.x, moving_vel.y);

    // --- Simulation loop ---
    const float dt = 1.0f / 60.0f; // 60 FPS
    const int total_ticks = 180;   // 3 seconds of simulation
    printf("\nStarting 3-second simulation...\n");
    for (int i = 0; i < total_ticks; ++i)
    {
        // Executes one tick of the ECS systems
        ecs_update(dt);
        // sleep(1); // Uncomment to see the movement in real time
    }
    printf("Simulation finished.\n");

    // --- Check final state and dirty tracking ---

    printf("\n--- State and Dirty Flags verification ---\n");

    // Final state of entity 1 (static)
    position_t* final_static_pos = ecs_get_component(static_entity, COMPONENT_POSITION);
    printf("Final state of entity %d (static): Position (%.2f, %.2f)\n", static_entity, final_static_pos->x, final_static_pos->y);

    // Check dirty flag for the static entity
    dirty_component_t dirty_components[MAX_COMPONENTS];
    int static_dirty_count = ecs_get_dirty_components(static_entity, dirty_components);
    printf("'Dirty' components for entity %d: %d\n", static_entity, static_dirty_count);

    // Final state of entity 2 (moving)
    position_t* final_moving_pos = ecs_get_component(moving_entity, COMPONENT_POSITION);
    printf("Final state of entity %d (moving): Position (%.2f, %.2f)\n", moving_entity, final_moving_pos->x, final_moving_pos->y);

    // Check dirty flag for the moving entity
    int moving_dirty_count = ecs_get_dirty_components(moving_entity, dirty_components);
    printf("'Dirty' components for entity %d: %d\n", moving_entity, moving_dirty_count);
    if (moving_dirty_count > 0) {
        printf("  -> Dirty component: %s\n", ecs_get_component_name(dirty_components[0].component_id));
    }

    // --- Serialization tests ---
    printf("\n--- Serialization tests ---\n");
    uint8_t buffer[1024];
    size_t serialized_size;

    // Serialize the moving entity
    if (ecs_serialize_entity(moving_entity, buffer, &serialized_size, 1024)) {
        printf("Entity %d serialized successfully. Size: %zu bytes.\n", moving_entity, serialized_size);

        // Deserialize into a new entity
        entity_t new_entity = ecs_deserialize_entity(buffer);
        if (new_entity != (entity_t)-1) {
            position_t* new_pos = ecs_get_component(new_entity, COMPONENT_POSITION);
            printf("Entity %d deserialized. Its position is (%.2f, %.2f).\n", new_entity, new_pos->x, new_pos->y);
            ecs_destroy_entity(new_entity);
        }
    }
    else {
        printf("Error serializing entity.\n");
    }

    // --- Cleanup ---
    ecs_destroy_entity(static_entity);
    ecs_destroy_entity(moving_entity);

    printf("\nTests completed.\n");

    return 0;
}