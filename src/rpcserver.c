#include "rpc.h"
#include <string.h>

K_MSGQ_DEFINE(rpc_req_q, sizeof(struct rpc_msg), RPC_QUEUE_DEPTH, 4);
K_MSGQ_DEFINE(rpc_res_q, sizeof(struct rpc_msg), RPC_QUEUE_DEPTH, 4);

static void rpc_dispatch(const struct rpc_msg *req, struct rpc_msg *res)
{
    res->id     = req->id;
    res->method = req->method;
    res->status = RPC_OK;
    res->len    = 0;

    switch (req->method) {
    case RPC_PING: {
        /* no args */
        const uint32_t pong = 0xBEEFCAFE;
        res->len = sizeof(pong);
        memcpy(res->payload, &pong, sizeof(pong));
        break;
    }

    case RPC_ADD: {
        /* expects 2x int32: a,b -> returns int32 sum */
        if (req->len != 8) {
            res->status = RPC_ERR_BAD_ARGS;
            break;
        }

        int32_t a, b;
        memcpy(&a, &req->payload[0], 4);
        memcpy(&b, &req->payload[4], 4);

        int32_t sum = a + b;
        res->len = sizeof(sum);
        memcpy(res->payload, &sum, sizeof(sum));
        break;
    }

    default:
        res->status = RPC_ERR_UNKNOWN_METHOD;
        break;
    }
}

static void rpc_server_thread(void)
{
    struct rpc_msg req;
    struct rpc_msg res;

    while (1) {
        k_msgq_get(&rpc_req_q, &req, K_FOREVER);
        rpc_dispatch(&req, &res);
        k_msgq_put(&rpc_res_q, &res, K_FOREVER);
    }
}

int rpc_server_init(void)
{
    /* Server thread is started via K_THREAD_DEFINE already */
    return 0;
}

K_THREAD_DEFINE(rpc_srv_tid, 2048, rpc_server_thread, NULL, NULL, NULL, 5, 0, 0);