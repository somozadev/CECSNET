#include <stdio.h>
#include <string.h>
#include "ecs.h"

static Position positions[MAX_ENTITIES];
static int has_position[MAX_ENTITIES];

static Health healths[MAX_ENTITIES];
static int has_health[MAX_ENTITIES];

static Entity next_entity = 0;

void ecs_init() {
    memset(has_position, 0, sizeof(has_position));
    memset(has_health, 0, sizeof(has_health));
    next_entity = 0;
    printf("ECS initialized\n");
}

Entity ecs_create_entity() {
    if (next_entity >= MAX_ENTITIES) {
        printf("Max entities reached!\n");
        return (Entity)-1;
    }
    Entity e = next_entity++;
    return e;
}

void ecs_add_position(Entity e, int x, int y) {
    if (e >= MAX_ENTITIES) return;
    positions[e].x = x;
    positions[e].y = y;
    has_position[e] = 1;
}

void ecs_add_health(Entity e, int health) {
    if (e >= MAX_ENTITIES) return;
    healths[e].health = health;
    has_health[e] = 1;
}

Position* ecs_get_position(Entity e) {
    if (e >= MAX_ENTITIES || !has_position[e]) return NULL;
    return &positions[e];
}

Health* ecs_get_health(Entity e) {
    if (e >= MAX_ENTITIES || !has_health[e]) return NULL;
    return &healths[e];
}

void ecs_update(float dt) {
    printf("ECS update: %f seconds\n", dt);
    for (Entity e = 0; e < next_entity; e++) {
        if (has_position[e] && has_health[e]) {
            printf("Entity %d - Position(%d,%d) Health: %d\n",
                e,
                positions[e].x,
                positions[e].y,
                healths[e].health);
        }
    }
}
