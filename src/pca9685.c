#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#define PCA9685_ADDR 0x40

#define PRESCALE   0xFE
#define LED0_ON_L  0x06

#define SERVO_MIN  102   // ~0.5 ms
#define SERVO_MID  307   // ~1.5 ms
#define SERVO_MAX  512   // ~2.5 ms

#define MODE1    0x00
#define MODE2    0x01
#define PRESCALE 0xFE
#define PCA9685_ADDR 0x40
#define PCA_NODE DT_NODELABEL(pca9685)

static const struct device *i2c_dev;
LOG_MODULE_DECLARE(hexapod);

int pca9685_init(void)
{
    i2c_dev = DEVICE_DT_GET(DT_BUS(PCA_NODE));

    if (!device_is_ready(i2c_dev)) {
        return -ENODEV;
    }
    uint8_t who = 0;
    i2c_write_read(i2c_dev, PCA9685_ADDR, (uint8_t[]){MODE1}, 1, &who, 1);
    LOG_INF("MODE1 = 0x%02x", who);

    uint8_t buf[2];

    /* MODE1 reset */
    buf[0] = MODE1;
    buf[1] = 0x00;
    i2c_write(i2c_dev, buf, 2, PCA9685_ADDR);
    k_sleep(K_MSEC(10));

    /* MODE2: totem pole */
    buf[0] = MODE2;
    buf[1] = 0x04; // OUTDRV
    i2c_write(i2c_dev, buf, 2, PCA9685_ADDR);

    /* sleep */
    buf[0] = MODE1;
    buf[1] = 0x10;
    i2c_write(i2c_dev, buf, 2, PCA9685_ADDR);

    /* prescale 50Hz */
    buf[0] = PRESCALE;
    buf[1] = 121;
    i2c_write(i2c_dev, buf, 2, PCA9685_ADDR);

    /* wake + auto increment */
    buf[0] = MODE1;
    buf[1] = 0x20;   // AI = 1
    i2c_write(i2c_dev, buf, 2, PCA9685_ADDR);
    k_sleep(K_MSEC(1));

    /* restart + auto increment */
    buf[0] = MODE1;
    buf[1] = 0xA0;   // RESTART(0x80) | AI(0x20)
    i2c_write(i2c_dev, buf, 2, PCA9685_ADDR);


    return 0;
}
void pca9685_set_pwm(uint8_t channel, uint16_t on, uint16_t off)
{
    uint8_t buf[5];

    buf[0] = LED0_ON_L + 4 * channel;
    buf[1] = on & 0xFF;
    buf[2] = on >> 8;
    buf[3] = off & 0xFF;
    buf[4] = off >> 8;

    int ret = i2c_write(i2c_dev, buf, 5, PCA9685_ADDR);
    if (ret) {
        LOG_ERR("i2c_write failed: %d", ret);
    }
}
