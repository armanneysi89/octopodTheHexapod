cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hexapod)

target_sources(app PRIVATE 
src/main.c src/pca9685.c
 src/rpcserver.c src/rpc_async.c
 src/rpc_tcp_bridge.c)

