// test_serialization.c
// Verifica la serialización y deserialización de entidades y componentes.

#include <stdio.h>
#include <string.h>
#include "../include/ecs.h"
#include "../include/ecs_internal.h"
#include "../include/ecs_types.h"

int main(void) {
    ecs_t ecs;
    ecs_init(&ecs);

    // Creates an entity and adds position and rotation to it
    entity_t e = ecs_create_entity(&ecs);
    position_t pos = { .x = 3.5f, .y = -1.2f };
    rotation_t rot = { .x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f };
    ecs_add_component(&ecs, e, COMPONENT_POSITION, &pos);
    ecs_add_component(&ecs, e, COMPONENT_ROTATION, &rot);

    // Serialize the entity into a buffer
    uint8_t buffer[256];
    size_t out_size;
    if (!ecs_serialize_entity(&ecs, e, buffer, &out_size, sizeof(buffer))) {
        printf("Error serializing the entity\n");
        return 1;
    }
    printf("Entity %u serialized in %zu bytes\n", e, out_size);

    // Destroy the original entity and deserialize a new one
    ecs_destroy_entity(&ecs, e);
    entity_t e2 = ecs_deserialize_entity(&ecs, buffer);
    printf("Entity deserialized with ID %u\n", e2);

    // Check the components match
    position_t *pos2 = ecs_get_component(&ecs, e2, COMPONENT_POSITION);
    rotation_t *rot2 = ecs_get_component(&ecs, e2, COMPONENT_ROTATION);
    if (pos2 && rot2) {
        printf("Deserialized position: (%f, %f)\n", pos2->x, pos2->y);
        printf("Deserialized rotation: (%f, %f, %f, %f)\n",
               rot2->x, rot2->y, rot2->z, rot2->w);
    } else {
        printf("Error: components not restored after deserialization\n");
    }

    return 0;
}