/*
 * 文件 : daq_app.h
 * 描述 : 数采应用状态机接口声明
 * 版本 : v1.0
 * 日期 : 2026/05/30
 *
 * 修改记录（最新版本在最前）:
 *  ver  who      date       modification
 * ----- -------  ---------- ---------------------------------
 * 1.0   ---      26/05/30   创建文件
 */

#ifndef DAQ_APP_H_
#define DAQ_APP_H_

#include "xil_types.h"

int daq_app_init(void);
int daq_app_process(void);
void daq_app_on_tcp_connected(void);
void daq_app_on_tcp_disconnected(void);
void daq_app_on_tcp_rx(const u8 *data, u32 len);

#endif
