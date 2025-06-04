#include "ecsnet.h"
#include <stdio.h>

int main() {
    ecsnet_initialize();

    for (int i = 0; i < 60; ++i) {
        ecsnet_tick(1.0f / 60.0f);
        ecsnet_send();
        ecsnet_receive();
    }

    return 0;
}
