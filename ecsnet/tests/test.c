#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "test_ecs.h"

int main()
{
    assert(test_ecs() == true);
    getchar();
    return 0;
}