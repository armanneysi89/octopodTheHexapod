#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hexapod, LOG_LEVEL_INF);

int main(void)
{
    LOG_INF("Hexapod läuft auf ESP32 🚀");

    while (1) {
        k_sleep(K_SECONDS(1));
    }
    return 0;
}

