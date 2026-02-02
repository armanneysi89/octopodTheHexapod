#include "rpc.h"
#include <string.h>

static atomic_t rpc_id_counter = ATOMIC_INIT(1);

struct pending_call {
    bool used;
    uint32_t id;
    uint16_t method;
    rpc_cb_t cb;
    void *user;
};

static struct pending_call pending[RPC_MAX_PENDING];
static struct k_mutex pending_lock;

static struct pending_call *pending_alloc(uint32_t id, uint16_t method,
                                          rpc_cb_t cb, void *user)
{
    k_mutex_lock(&pending_lock, K_FOREVER);
    for (int i = 0; i < RPC_MAX_PENDING; i++) {
        if (!pending[i].used) {
            pending[i].used = true;
            pending[i].id = id;
            pending[i].method = method;
            pending[i].cb = cb;
            pending[i].user = user;
            k_mutex_unlock(&pending_lock);
            return &pending[i];
        }
    }
    k_mutex_unlock(&pending_lock);
    return NULL;
}

static struct pending_call *pending_take(uint32_t id)
{
    struct pending_call *out = NULL;

    k_mutex_lock(&pending_lock, K_FOREVER);
    for (int i = 0; i < RPC_MAX_PENDING; i++) {
        if (pending[i].used && pending[i].id == id) {
            out = &pending[i];
            break;
        }
    }
    k_mutex_unlock(&pending_lock);
    return out;
}

static void pending_free(struct pending_call *p)
{
    k_mutex_lock(&pending_lock, K_FOREVER);
    p->used = false;
    k_mutex_unlock(&pending_lock);
}

int rpc_async_call(uint16_t method,
                   const void *payload, uint16_t len,
                   rpc_cb_t cb, void *user)
{
    if (len > RPC_MAX_PAYLOAD) return -EINVAL;
    if (cb == NULL) return -EINVAL;

    struct rpc_msg msg = {0};
    msg.id = (uint32_t)atomic_inc(&rpc_id_counter);
    msg.method = method;
    msg.status = 0;
    msg.len = len;

    if (len && payload) {
        memcpy(msg.payload, payload, len);
    }

    if (!pending_alloc(msg.id, method, cb, user)) {
        return -ENOMEM;
    }

    int rc = k_msgq_put(&rpc_req_q, &msg, K_NO_WAIT);
    if (rc != 0) {
        /* rollback pending allocation */
        struct pending_call *p = pending_take(msg.id);
        if (p) pending_free(p);
        return -EAGAIN;
    }

    return 0;
}

/* RX thread: takes responses and triggers callbacks */
static void rpc_client_rx_thread(void)
{
    while (1) {
        struct rpc_msg reply;
        k_msgq_get(&rpc_res_q, &reply, K_FOREVER);

        struct pending_call *p = pending_take(reply.id);
        if (!p) {
            /* unknown/late reply -> ignore */
            continue;
        }

        rpc_cb_t cb = p->cb;
        void *user = p->user;
        uint16_t method = p->method;

        pending_free(p);

        /* call user callback */
        cb(reply.id, method, reply.status, reply.payload, reply.len, user);
    }
}

K_THREAD_DEFINE(rpc_client_rx_tid, 2048,
                rpc_client_rx_thread, NULL, NULL, NULL,
                6, 0, 0);