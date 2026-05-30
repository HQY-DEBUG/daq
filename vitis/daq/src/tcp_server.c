/*
 * Copyright (C) 2009 - 2019 Xilinx, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <string.h>

#include "daq_app.h"
#include "daq_config.h"
#include "lwip/err.h"
#include "lwip/tcp.h"
#include "tcp_server.h"
#include "xil_printf.h"

static struct tcp_pcb *g_client_pcb;

/**
 * @brief  打印 TCP 服务启动信息
 */
void tcp_server_print_header(void)
{
    xil_printf("\n\r\n\r-----DAQ TCP server ------\n\r");
    xil_printf("DAQ control and data port: %d\n\r", DAQ_TCP_PORT);
}

/**
 * @brief  TCP 接收回调函数
 * @param  arg lwIP 回调参数
 * @param  tpcb 当前 TCP 控制块
 * @param  p 接收数据缓冲链表，NULL 表示连接关闭
 * @param  err lwIP 错误码
 * @return lwIP 回调处理结果
 */
static err_t recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    struct pbuf *q;

    (void)arg;
    (void)err;

    if (!p) {
        if (g_client_pcb == tpcb) {
            g_client_pcb = NULL;
            daq_app_on_tcp_disconnected();
        }
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    tcp_recved(tpcb, p->tot_len);
    for (q = p; q != NULL; q = q->next) {
        daq_app_on_tcp_rx((const u8 *)q->payload, q->len);
    }

    pbuf_free(p);

    return ERR_OK;
}

/**
 * @brief  TCP 连接接入回调函数
 * @param  arg lwIP 回调参数
 * @param  newpcb 新连接 TCP 控制块
 * @param  err lwIP 错误码
 * @return lwIP 回调处理结果
 */
static err_t accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    static int connection = 1;

    (void)arg;
    (void)err;

    if (g_client_pcb != NULL) {
        xil_printf("Only one TCP client is supported\r\n");
        tcp_close(newpcb);
        return ERR_OK;
    }

    g_client_pcb = newpcb;
    tcp_recv(newpcb, recv_callback);
    tcp_arg(newpcb, (void *)(UINTPTR)connection);
    connection++;
    daq_app_on_tcp_connected();

    return ERR_OK;
}

/**
 * @brief  启动 TCP 服务监听
 * @return 0 表示成功，负数表示 TCP PCB 创建、绑定或监听失败
 */
int tcp_server_start(void)
{
    struct tcp_pcb *pcb;
    err_t err;
    unsigned port = DAQ_TCP_PORT;

    pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        xil_printf("Error creating PCB. Out of Memory\n\r");
        return -1;
    }

    err = tcp_bind(pcb, IP_ANY_TYPE, port);
    if (err != ERR_OK) {
        xil_printf("Unable to bind to port %d: err = %d\n\r", port, err);
        return -2;
    }

    tcp_arg(pcb, NULL);

    pcb = tcp_listen(pcb);
    if (!pcb) {
        xil_printf("Out of memory while tcp_listen\n\r");
        return -3;
    }

    tcp_accept(pcb, accept_callback);

    xil_printf("TCP server started @ port %d\n\r", port);

    return 0;
}

/**
 * @brief  通过当前 TCP 连接发送数据
 * @param  data 待发送数据缓冲区
 * @param  len 待发送数据长度
 * @return 0 表示成功，-2 表示发送缓存不足，其他负数表示发送失败
 */
int tcp_server_send(const u8 *data, u32 len)
{
    err_t err;

    if ((data == NULL) || (len == 0U)) {
        return -1;
    }

    if (g_client_pcb == NULL) {
        return -3;
    }

    if (tcp_sndbuf(g_client_pcb) < len) {
        return -2;
    }

    err = tcp_write(g_client_pcb, data, len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        xil_printf("tcp_write failed, err=%d\r\n", err);
        return -4;
    }

    err = tcp_output(g_client_pcb);
    if (err != ERR_OK) {
        xil_printf("tcp_output failed, err=%d\r\n", err);
        return -5;
    }

    return 0;
}

/**
 * @brief  查询当前是否已有 TCP 客户端连接
 * @return 非 0 表示已有连接，0 表示无连接
 */
int tcp_server_has_client(void)
{
    return g_client_pcb != NULL;
}
