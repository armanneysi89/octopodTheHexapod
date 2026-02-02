#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

#include "rpc.h"

LOG_MODULE_DECLARE(hexapod);


#define RPC_TCP_PORT 5555

static int recv_all(int fd, void *buf, size_t len)
{
    uint8_t *p = buf;

    while (len > 0) {
        ssize_t r = zsock_recv(fd, p, len, 0);
        if (r <= 0) {
            return -1;
        }
        p += r;
        len -= r;
    }
    return 0;
}

static int send_all(int fd, const void *buf, size_t len)
{
    const uint8_t *p = buf;

    while (len > 0) {
        ssize_t w = zsock_send(fd, p, len, 0);
        if (w <= 0) {
            return -1;
        }
        p += w;
        len -= w;
    }
    return 0;
}

static int tcp_recv_frame(int fd, uint8_t *buf, size_t max, size_t *out_len)
{
    uint16_t be_len;

    if (recv_all(fd, &be_len, sizeof(be_len)) != 0) {
        return -1;
    }

    uint16_t len = sys_be16_to_cpu(be_len);
    if (len > max) {
        return -2;
    }

    if (recv_all(fd, buf, len) != 0) {
        return -1;
    }

    *out_len = len;
    return 0;
}

static int tcp_send_frame(int fd, const uint8_t *buf, size_t len)
{
    uint16_t be_len = sys_cpu_to_be16((uint16_t)len);

    if (send_all(fd, &be_len, sizeof(be_len)) != 0) {
        return -1;
    }
    if (send_all(fd, buf, len) != 0) {
        return -1;
    }
    return 0;
}

static void rpc_tcp_bridge_thread(void)
{
  

      int srv = -1;

    while (srv < 0) {
        srv = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (srv < 0) {
            LOG_ERR("zsock_socket() failed, retry in 1s");
            k_sleep(K_SECONDS(1));
        }
    }

    int opt = 1;
    zsock_setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(RPC_TCP_PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (zsock_bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERR("zsock_bind() failed");
        zsock_close(srv);
        return;
    }

    if (zsock_listen(srv, 1) < 0) {
        LOG_ERR("zsock_listen() failed");
        zsock_close(srv);
        return;
    }

    LOG_INF("RPC TCP bridge listening on port %d", RPC_TCP_PORT);

    while (1) {
        int client = zsock_accept(srv, NULL, NULL);
        if (client < 0) {
            k_sleep(K_MSEC(200));
            continue;
        }

        LOG_INF("client connected");

        while (1) {
            uint8_t rx[sizeof(struct rpc_msg)];
            size_t rx_len = 0;

            int rc = tcp_recv_frame(client, rx, sizeof(rx), &rx_len);
            if (rc != 0) {
                LOG_INF("client disconnected rc=%d", rc);
                break;
            }

            if (rx_len != sizeof(struct rpc_msg)) {
                LOG_WRN("bad frame size %d", (int)rx_len);
                break;
            }

            struct rpc_msg req;
            memcpy(&req, rx, sizeof(req));

            LOG_INF("rpc req id=%u method=%u len=%u",
                    req.id, req.method, req.len);

            /* Send into IPC RPC server */
            rc = k_msgq_put(&rpc_req_q, &req, K_MSEC(200));
            if (rc != 0) {
                LOG_WRN("rpc_req_q full");
                break;
            }

            /* Wait for response */
            struct rpc_msg res;
            rc = k_msgq_get(&rpc_res_q, &res, K_MSEC(500));
            if (rc != 0) {
                LOG_WRN("rpc response timeout");
                break;
            }

            /* Return response */
            uint8_t tx[sizeof(struct rpc_msg)];
            memcpy(tx, &res, sizeof(res));

            rc = tcp_send_frame(client, tx, sizeof(tx));
            if (rc != 0) {
                LOG_WRN("send failed");
                break;
            }
        }

        zsock_close(client);
    }
}

K_THREAD_DEFINE(rpc_tcp_tid, 4096, rpc_tcp_bridge_thread,
                NULL, NULL, NULL, 4, 0, 0);