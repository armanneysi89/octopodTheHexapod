#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>

#include "rpcserver.h"
#include "rpc.h"
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hexapod, LOG_LEVEL_INF);

static void print_ipv4_addr(void)
{
    struct net_if *iface = net_if_get_default();
    if (!iface) {
        LOG_ERR("No default iface");
        return;
    }

    struct in_addr *addr = net_if_ipv4_get_global_addr(iface, NET_ADDR_DHCP);
    if (!addr) {
        LOG_WRN("No DHCP IPv4 yet");
        return;
    }

    char buf[NET_IPV4_ADDR_LEN];
    LOG_INF("IPv4: %s", net_addr_ntop(AF_INET, addr, buf, sizeof(buf)));
}
static int rpc_sys_init(void)
{
    return rpc_server_init();
}

extern int pca9685_init(void);
extern void pca9685_set_pwm(uint8_t, uint16_t, uint16_t);

#define SERVO_MIN  102   // ~0.5 ms
#define SERVO_MID  307   // ~1.5 ms
#define SERVO_MAX  512   // ~2.5 ms

#define MODE1    0x00
#define MODE2    0x01
#define PRESCALE 0xFE
#define PCA9685_ADDR 0x40


SYS_INIT(rpc_sys_init, APPLICATION, 90);
SYS_INIT(pca9685_init, APPLICATION, 91);

int main(void)
{
    k_sleep(K_SECONDS(5));  // warten bis WLAN/DHCP ready
    print_ipv4_addr();
    while(1) {
        k_sleep(K_SECONDS(10));
    }
    return 0;
    
}
