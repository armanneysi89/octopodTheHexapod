#pragma once
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <stdint.h>

#define RPC_MAX_PAYLOAD   32
#define RPC_QUEUE_DEPTH   8
#define RPC_MAX_PENDING   8   /* max parallel async calls */

/* Methods */
typedef enum {
    RPC_PING = 1,
    RPC_ADD  = 2,
} rpc_method_t;

/* Status */
typedef enum {
    RPC_OK = 0,
    RPC_ERR_UNKNOWN_METHOD = -1,
    RPC_ERR_BAD_ARGS       = -2,
} rpc_status_t;

/* Message */
struct rpc_msg {
    uint32_t id;
    uint16_t method;
    int16_t  status;
    uint16_t len;
    uint8_t  payload[RPC_MAX_PAYLOAD];
};

/* IPC queues */
extern struct k_msgq rpc_req_q;
extern struct k_msgq rpc_res_q;

/* ===== Async client API ===== */
typedef void (*rpc_cb_t)(uint32_t id,
                         uint16_t method,
                         int16_t status,
                         const uint8_t *payload,
                         uint16_t len,
                         void *user);

/* fire & forget call (response triggers callback) */
int rpc_async_call(uint16_t method,
                   const void *payload, uint16_t len,
                   rpc_cb_t cb, void *user);
