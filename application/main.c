#include <stdio.h>
#include <pigpio.h>

int main()
{

    int res = gpioInitialise();
    if (res < 0)
    {
        return 1;
    }

    while (1)
    {
        time_sleep(1);
        printf("toggle\n");
        gpioWrite(17, 1);
        time_sleep(1);
        printf("toggle\n");
        gpioWrite(17, 0);
    }

    return 0;
}
