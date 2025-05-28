#include <stdio.h>
#include "ecs.h"
#include "net.h"

int main() {
    ecs_init();
    net_init();

    while (1) {
        ecs_update(0.016f); // 60 FPS
        net_update();
    }

    return 0;
}