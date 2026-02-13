#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include "wifi.h"
#include <zephyr/net/net_compat.h>

// Event callbacks
static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;

// Semaphores
static K_SEM_DEFINE(sem_wifi, 0, 1);
static K_SEM_DEFINE(sem_ipv4, 0, 1);
LOG_MODULE_DECLARE(hexapod);

// Called when the WiFi is connected
static void on_wifi_connection_event(struct net_mgmt_event_callback *cb,
                                     uint64_t mgmt_event,
                                     struct net_if *iface)
{
    printk("on_wifi_connection_event: event %llu\n", mgmt_event);
    const struct wifi_status *status = (const struct wifi_status *)cb->info;

    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        if (status->status) {
            printk("Error (%d): Connection request failed\r\n", status->status);
        } else {
            printk("Connected!\r\n");
            k_sem_give(&sem_wifi);
        }
    } else if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
        if (status->status) {
            printk("Error (%d): Disconnection request failed\r\n", status->status);
        } else {
            printk("Disconnected\r\n");
            k_sem_take(&sem_wifi, K_NO_WAIT);
        }
    }
}
void wifi_scan(void)
{
    struct net_if *iface = net_if_get_default();
    
    LOG_INF("Starting WiFi scan...");
    if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0)) {
        LOG_ERR("Scan request failed");
        return;
    }
    
    LOG_INF("Scan started, results will appear in events");
    k_sleep(K_SECONDS(5)); // Wait for scan to complete
}
// Event handler for WiFi management events
static void on_ipv4_obtained(struct net_mgmt_event_callback *cb,
                             uint64_t mgmt_event,
                             struct net_if *iface)
{
    // Signal that the IP address has been obtained
    if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
        k_sem_give(&sem_ipv4);
    }
}

static void handle_wifi_scan_result(struct net_mgmt_event_callback *cb,
                                    long long unsigned int mgmt_event,
                                     struct net_if *iface)
{
    const struct wifi_scan_result *entry = (const struct wifi_scan_result *)cb->info;
    
    LOG_INF("Scan result: SSID=%-32s, Channel=%d, RSSI=%d, Security=%d",
            entry->ssid, entry->channel, entry->rssi, entry->security);
}

static void handle_wifi_scan_done(struct net_mgmt_event_callback *cb,
                                   long long unsigned int mgmt_event,
                                   struct net_if *iface)
{
    LOG_INF("WiFi scan completed");
}

static struct net_mgmt_event_callback wifi_scan_result_cb;
static struct net_mgmt_event_callback wifi_scan_done_cb;
// Initialize the WiFi event callbacks
void wifi_init(void)
{

    struct net_if *iface = net_if_get_default();
    if (!iface) {
        LOG_ERR("No default interface found");
        return;
    }

    // Set WiFi region/country code
    struct wifi_reg_domain reg_domain = {
        .country_code = "DE",.oper = 0
    };
    
    int ret = net_mgmt(NET_REQUEST_WIFI_REG_DOMAIN, iface,
                       &reg_domain, sizeof(reg_domain));
    if (ret) {
        LOG_WRN("Failed to set region: %d", ret);
    } else {
        LOG_INF("WiFi region set to: %s", reg_domain.country_code);
    }
    // Initialize the event callbacks
    net_mgmt_init_event_callback(&wifi_cb,
                                 on_wifi_connection_event,
                                 NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_init_event_callback(&ipv4_cb,
                                 on_ipv4_obtained,
                                 NET_EVENT_IPV4_DHCP_BOUND | NET_EVENT_IPV4_ADDR_ADD);
    
    net_mgmt_init_event_callback(&wifi_scan_result_cb,
                                 handle_wifi_scan_result,
                                 NET_EVENT_WIFI_SCAN_RESULT);
    
    net_mgmt_init_event_callback(&wifi_scan_done_cb,
                                 handle_wifi_scan_done,
                                 NET_EVENT_WIFI_SCAN_DONE);
    
    net_mgmt_add_event_callback(&wifi_scan_result_cb);
    net_mgmt_add_event_callback(&wifi_scan_done_cb);
    // Add the event callbacks
    net_mgmt_add_event_callback(&wifi_cb);
    net_mgmt_add_event_callback(&ipv4_cb);
}

int wifi_connect(const char *ssid, const char *psk)
{
    struct net_if *iface = net_if_get_default();
    struct wifi_connect_req_params params = {0};
    
    if (!iface) {
        LOG_ERR("No default interface");
        return -ENODEV;
    }

    // Set connection parameters
    params.ssid = (uint8_t *)ssid;
    params.ssid_length = strlen(ssid);
    params.psk = (uint8_t *)psk;
    params.psk_length = strlen(psk);
    params.channel = WIFI_CHANNEL_ANY;
    params.security = WIFI_SECURITY_TYPE_PSK; // or WIFI_SECURITY_TYPE_PSK_SHA256
    params.band = WIFI_FREQ_BAND_2_4_GHZ;
    params.mfp = WIFI_MFP_OPTIONAL;
    
    LOG_INF("Connecting to SSID: %s (len=%d)", ssid, params.ssid_length);
    
    return net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
                    &params, sizeof(struct wifi_connect_req_params));
}

// Wait for IP address (blocking)
void wifi_wait_for_ip_addr(void)
{
    struct wifi_iface_status status;
    struct net_if *iface;
    char ip_addr[NET_IPV4_ADDR_LEN];
    char gw_addr[NET_IPV4_ADDR_LEN];

    // Get interface
    iface = net_if_get_default();

    // Wait for the IPv4 address to be obtained
    k_sem_take(&sem_ipv4, K_FOREVER);

    // Get the WiFi status
    if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS,
                 iface,
                 &status,
                 sizeof(struct wifi_iface_status))) {
        printk("Error: WiFi status request failed\r\n");
    }

    // Get the IP address
    memset(ip_addr, 0, sizeof(ip_addr));
    if (net_addr_ntop(AF_INET,
                      &iface->config.ip.ipv4->unicast[0].ipv4.address.in_addr,
                      ip_addr,
                      sizeof(ip_addr)) == NULL) {
        printk("Error: Could not convert IP address to string\r\n");
    }

    // Get the gateway address
    memset(gw_addr, 0, sizeof(gw_addr));
    if (net_addr_ntop(AF_INET,
                      &iface->config.ip.ipv4->gw,
                      gw_addr,
                      sizeof(gw_addr)) == NULL) {
        printk("Error: Could not convert gateway address to string\r\n");
    }

    // Print the WiFi status
    printk("WiFi status:\r\n");
    if (status.state >= WIFI_STATE_ASSOCIATED) {
        printk("  SSID: %-32s\r\n", status.ssid);
        printk("  Band: %s\r\n", wifi_band_txt(status.band));
        printk("  Channel: %d\r\n", status.channel);
        printk("  Security: %s\r\n", wifi_security_txt(status.security));
        printk("  IP address: %s\r\n", ip_addr);
        printk("  Gateway: %s\r\n", gw_addr);
    }
}

// Disconnect from the WiFi network
int wifi_disconnect(void)
{
    int ret;
    struct net_if *iface = net_if_get_default();

    ret = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);

    return ret;
}