#pragma once

// Function prototypes
void wifi_init(void);
int wifi_connect(const char *ssid, const char *psk);
void wifi_wait_for_ip_addr(void);
int wifi_disconnect(void);
void wifi_scan(void);
