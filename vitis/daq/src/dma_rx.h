/*
 * 文件 : dma_rx.h
 * 描述 : AXI DMA 接收接口声明
 * 版本 : v1.0
 * 日期 : 2026/05/30
 *
 * 修改记录（最新版本在最前）:
 *  ver  who      date       modification
 * ----- -------  ---------- ---------------------------------
 * 1.0   ---      26/05/30   创建文件
 */

#ifndef DMA_RX_H_
#define DMA_RX_H_

#include "xil_types.h"

int dma_rx_init(void);
int dma_rx_start(void);
int dma_rx_is_done(void);
void dma_rx_abort(void);
u8 *dma_rx_get_buffer(void);
u32 dma_rx_get_length(void);
void dma_rx_invalidate_cache(void);

#endif
