#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>

#include "rpcserver.h"
#include "rpc.h"
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_mgmt.h>
#include "wifi_secrets.h"
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/socket.h>

#include "wifi.h"


#define SERVO_MIN  102   // ~0.5 ms
#define SERVO_MID  307   // ~1.5 ms
#define SERVO_MAX  512   // ~2.5 ms

#define MODE1    0x00
#define MODE2    0x01
#define PRESCALE 0xFE
#define PCA9685_ADDR 0x40

extern int pca9685_init(void);
extern void pca9685_set_pwm(uint8_t, uint16_t, uint16_t);

LOG_MODULE_REGISTER(hexapod, LOG_LEVEL_INF);


static int rpc_sys_init(void)
{
    return rpc_server_init();
}

void print_addrinfo(struct zsock_addrinfo **results)
{
    char ipv4[INET_ADDRSTRLEN];
    char ipv6[INET6_ADDRSTRLEN];
    struct sockaddr_in *sa;
    struct sockaddr_in6 *sa6;
    struct zsock_addrinfo *rp;

    // Iterate through the results
    for (rp = *results; rp != NULL; rp = rp->ai_next) {

        // Print IPv4 address
        if (rp->ai_addr->sa_family == AF_INET) {
            sa = (struct sockaddr_in *)rp->ai_addr;
            zsock_inet_ntop(AF_INET, &sa->sin_addr, ipv4, INET_ADDRSTRLEN);
            printk("IPv4: %s\r\n", ipv4);
        }

        // Print IPv6 address
        if (rp->ai_addr->sa_family == AF_INET6) {
            sa6 = (struct sockaddr_in6 *)rp->ai_addr;
            zsock_inet_ntop(AF_INET6, &sa6->sin6_addr, ipv6, INET6_ADDRSTRLEN);
            printk("IPv6: %s\r\n", ipv6);
        }
    }
}
int main(void)
{
    printk("Starting Hexapod RPC Server\r\n");
    printk("Initializing WiFi...\r\n");
    wifi_init();
    
    // Optional: scan first to verify AP is visible
    wifi_scan();
    k_sleep(K_SECONDS(10));
    printk("Connecting to WiFi SSID: %s\r\n", WIFI_SSID);
    int ret = wifi_connect(WIFI_SSID, WIFI_PSK);
    printk("WiFi connect returned: %d\r\n", ret);
    
    if (ret < 0) {
        printk("Error (%d): WiFi connection failed\r\n", ret);
        printk("Check: 1) SSID correct? 2) Password correct? 3) 2.4GHz network?\r\n");
        // Don't return, keep running for debug
    } else {
        printk("WiFi connected, wait for IP address\r\n");
        wifi_wait_for_ip_addr();
        printk("IP address obtained\r\n");
    }
    
    while(1) {
        k_sleep(K_SECONDS(10));
    }
    
    return 0;
}

SYS_INIT(rpc_sys_init, APPLICATION, 90);
SYS_INIT(pca9685_init, APPLICATION, 91);