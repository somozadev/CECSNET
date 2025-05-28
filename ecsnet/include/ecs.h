#ifndef ECS_H
#define ECS_H

#include <stdint.h>

#define MAX_ENTITIES 1000
#define MAX_COMPONENTS 32

typedef uint32_t Entity;

typedef struct {
    int x, y;
} Position;

typedef struct {
    int health;
} Health;


void ecs_init();
void ecs_init();
Entity ecs_create_entity();
void ecs_add_position(Entity e, int x, int y);
void ecs_add_health(Entity e, int health);
Position* ecs_get_position(Entity e);
Health* ecs_get_health(Entity e);
void ecs_update(float dt);

#endif