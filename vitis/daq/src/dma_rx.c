/*
 * 文件 : dma_rx.c
 * 描述 : AXI DMA 简单模式接收控制
 * 版本 : v1.0
 * 日期 : 2026/05/30
 *
 * 修改记录（最新版本在最前）:
 *  ver  who      date       modification
 * ----- -------  ---------- ---------------------------------
 * 1.0   ---      26/05/30   创建文件
 */

#include "daq_config.h"
#include "dma_rx.h"
#include "xaxidma.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xparameters.h"

#ifndef XPAR_AXIDMA_0_DEVICE_ID
#error "未找到 XPAR_AXIDMA_0_DEVICE_ID，请在 Vivado/Vitis BSP 中确认 AXI DMA IP 名称和 xparameters.h"
#endif

static XAxiDma g_axi_dma;
static u8 g_dma_rx_buffer[DAQ_DMA_RX_BUFFER_SIZE] __attribute__ ((aligned(64)));
static int g_dma_rx_busy;

int dma_rx_init(void)
{
    int status;
    XAxiDma_Config *config;

    config = XAxiDma_LookupConfig(XPAR_AXIDMA_0_DEVICE_ID);
    if (config == NULL) {
        xil_printf("XAxiDma_LookupConfig failed\r\n");
        return -1;
    }

    status = XAxiDma_CfgInitialize(&g_axi_dma, config);
    if (status != XST_SUCCESS) {
        xil_printf("XAxiDma_CfgInitialize failed, status=%d\r\n", status);
        return -2;
    }

    if (XAxiDma_HasSg(&g_axi_dma)) {
        xil_printf("AXI DMA SG mode is not supported in this driver\r\n");
        return -3;
    }

    XAxiDma_IntrDisable(&g_axi_dma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
    XAxiDma_IntrDisable(&g_axi_dma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
    g_dma_rx_busy = 0;

    return 0;
}

int dma_rx_start(void)
{
    int status;

    if (g_dma_rx_busy != 0) {
        return 0;
    }

    Xil_DCacheFlushRange((UINTPTR)g_dma_rx_buffer, DAQ_DMA_RX_BUFFER_SIZE);
    status = XAxiDma_SimpleTransfer(&g_axi_dma, (UINTPTR)g_dma_rx_buffer, DAQ_DMA_RX_BUFFER_SIZE, XAXIDMA_DEVICE_TO_DMA);
    if (status != XST_SUCCESS) {
        xil_printf("XAxiDma_SimpleTransfer RX failed, status=%d\r\n", status);
        return -1;
    }

    g_dma_rx_busy = 1;
    return 0;
}

int dma_rx_is_done(void)
{
    if (g_dma_rx_busy == 0) {
        return 0;
    }

    if (XAxiDma_Busy(&g_axi_dma, XAXIDMA_DEVICE_TO_DMA)) {
        return 0;
    }

    g_dma_rx_busy = 0;
    return 1;
}

void dma_rx_abort(void)
{
    int timeout;

    if (g_dma_rx_busy == 0) {
        return;
    }

    XAxiDma_Reset(&g_axi_dma);
    timeout = 1000000;
    while ((timeout > 0) && !XAxiDma_ResetIsDone(&g_axi_dma)) {
        timeout--;
    }

    if (timeout == 0) {
        xil_printf("XAxiDma_Reset timeout\r\n");
    }

    g_dma_rx_busy = 0;
}

u8 *dma_rx_get_buffer(void)
{
    return g_dma_rx_buffer;
}

u32 dma_rx_get_length(void)
{
    return DAQ_DMA_RX_BUFFER_SIZE;
}

void dma_rx_invalidate_cache(void)
{
    Xil_DCacheInvalidateRange((UINTPTR)g_dma_rx_buffer, DAQ_DMA_RX_BUFFER_SIZE);
}
