/*
 * 文件 : daq_protocol.h
 * 描述 : DAQ 网络控制协议接口声明
 * 版本 : v1.0
 * 日期 : 2026/05/30
 *
 * 修改记录（最新版本在最前）:
 *  ver  who      date       modification
 * ----- -------  ---------- ---------------------------------
 * 1.0   ---      26/05/30   创建文件
 */

#ifndef DAQ_PROTOCOL_H_
#define DAQ_PROTOCOL_H_

#include "xil_types.h"

typedef struct {
    u32 data_type;
    u32 data_size;
    const u8 *payload;
} daq_protocol_frame_t;

typedef void (*daq_protocol_handler_t)(const daq_protocol_frame_t *frame, void *context);

void daq_protocol_init(void);
int daq_protocol_input(const u8 *data, u32 len, daq_protocol_handler_t handler, void *context);
int daq_protocol_get_u32_payload(const daq_protocol_frame_t *frame, u32 *value);
int daq_protocol_build_u32_response(u32 data_type, u32 value, u8 *out_buf, u32 out_size, u32 *out_len);
int daq_protocol_build_data_frame(u32 packet_index, const u8 *data, u32 data_len, u8 *out_buf, u32 out_size, u32 *out_len);

#endif
