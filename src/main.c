#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>

extern int pca9685_init(void);
extern void pca9685_set_pwm(uint8_t, uint16_t, uint16_t);
LOG_MODULE_REGISTER(hexapod, LOG_LEVEL_INF);

#define SERVO_MIN  102   // ~0.5 ms
#define SERVO_MID  307   // ~1.5 ms
#define SERVO_MAX  512   // ~2.5 ms

#define MODE1    0x00
#define MODE2    0x01
#define PRESCALE 0xFE
#define PCA9685_ADDR 0x40

int main(void)
{
    LOG_INF("Init PCA9685");
    pca9685_init();
    LOG_INF("Starting servo test");
   

    while (1) {

        LOG_INF("Setting servo to minimum position");
        pca9685_set_pwm(0, 0, SERVO_MIN);
        k_sleep(K_SECONDS(1));

        LOG_INF("Setting servo to maximum position");
        pca9685_set_pwm(0, 0, SERVO_MAX);
        k_sleep(K_SECONDS(1));
    }
    return 0;
}

