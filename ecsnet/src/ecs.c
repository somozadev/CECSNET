#include "ecs.h"
#include <stdio.h>

void ecs_init() {
    printf("ECS initialized\n");
}

void ecs_update(float dt) {
    printf("ECS update: %f\n", dt);
}