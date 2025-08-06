#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include "../include/ecs.h"
#include "test_ecs.h"

bool test_ecs() {
    printf("Initializing ECS...\n");
    ecs_init();
    ecs_register_builtin_components();
    ecs_register_builtin_systems();

    bool success = true;

    // --- Create entities and assign components ---
    printf("\n--- Entity Creation Test ---\n");
    entity_t static_entity = ecs_create_entity();
    position_t static_pos = {10.0f, 20.0f};
    ecs_add_component(static_entity, COMPONENT_POSITION, &static_pos);
    assert(static_entity != (entity_t)-1 && "Failed to create static entity.");
    printf("Entity %d created with position at (%.2f, %.2f)\n", static_entity, static_pos.x, static_pos.y);

    // Clean initial dirty flags for a proper test (ecs_add_component triggers them on purpose)
    ecs_clear_component_dirty(static_entity, COMPONENT_POSITION);

    entity_t moving_entity = ecs_create_entity();
    position_t moving_pos = {0.0f, 0.0f};
    velocity_t moving_vel = {5.0f, 3.0f};
    ecs_add_component(moving_entity, COMPONENT_POSITION, &moving_pos);
    ecs_add_component(moving_entity, COMPONENT_VELOCITY, &moving_vel);
    assert(moving_entity != (entity_t)-1 && "Failed to create moving entity.");
    printf("Entity %d created with position at (%.2f, %.2f) and velocity at (%.2f, %.2f)\n", moving_entity,
           moving_pos.x, moving_pos.y, moving_vel.x, moving_vel.y);

    // Clean initial dirty flags for a proper test (ecs_add_component triggers them on purpose)
    ecs_clear_component_dirty(moving_entity, COMPONENT_POSITION);
    ecs_clear_component_dirty(moving_entity, COMPONENT_VELOCITY);

    // --- Simulation loop and movement test ---
    printf("\n--- Simulation Test ---\n");
    const float dt = 1.0f; // Simplified for easy calculation
    const int total_ticks = 3;
    printf("Starting %d-tick simulation...\n", total_ticks);
    for (int i = 0; i < total_ticks; ++i) {
        ecs_update(dt);
    }
    printf("Simulation finished.\n");

    position_t *final_moving_pos = ecs_get_component(moving_entity, COMPONENT_POSITION);
    printf("Final position of moving entity: (%.2f, %.2f)\n", final_moving_pos->x, final_moving_pos->y);
    assert(final_moving_pos->x == (3 * 5.0f) && final_moving_pos->y == (3 * 3.0f) && "Movement system failed.");

    // --- Dirty Flags verification ---
    printf("\n--- Dirty Flags Test ---\n");
    dirty_component_t dirty_components[MAX_COMPONENTS];
    int static_dirty_count = ecs_get_dirty_components(static_entity, dirty_components);
    assert(static_dirty_count == 0 && "Static entity should have no dirty components.");
    printf("Static entity dirty count: %d (PASSED)\n", static_dirty_count);

    int moving_dirty_count = ecs_get_dirty_components(moving_entity, dirty_components);
    assert(moving_dirty_count > 0 && "Moving entity should have dirty components.");
    printf("Moving entity dirty count: %d (PASSED)\n", moving_dirty_count);

    // --- Serialization test ---
    printf("\n--- Serialization Test ---\n");
    uint8_t buffer[1024];
    size_t serialized_size;

    success = ecs_serialize_entity(moving_entity, buffer, &serialized_size, 1024);
    assert(success && "Failed to serialize moving entity.");
    assert(serialized_size > 0 && "Serialized size should be greater than 0.");
    printf("Entity %d serialized successfully. Size: %zu bytes (PASSED).\n", moving_entity, serialized_size);

    entity_t new_entity = ecs_deserialize_entity(buffer);
    assert(new_entity != (entity_t)-1 && "Failed to deserialize entity.");
    position_t *new_pos = ecs_get_component(new_entity, COMPONENT_POSITION);
    assert(
        new_pos->x == final_moving_pos->x && new_pos->y == final_moving_pos->y &&
        "Deserialized position is incorrect.");
    printf("New entity %d deserialized correctly from entity %d. Position is (%.2f, %.2f) (PASSED).\n", new_entity, moving_entity, new_pos->x,
           new_pos->y);

    // --- Cleanup ---
    ecs_destroy_entity(static_entity);
    ecs_destroy_entity(moving_entity);
    ecs_destroy_entity(new_entity);

    printf("\nAll ECS tests completed successfully!\n");
    return success;
}

//gcc -Iinclude -o test tests/test.c tests/test_ecs.c src/ecs.c src/ecs_builtin.c
