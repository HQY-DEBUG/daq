/*
 * 文件 : daq_protocol.c
 * 描述 : DAQ 网络控制协议解析与组包
 * 版本 : v1.0
 * 日期 : 2026/05/30
 *
 * 修改记录（最新版本在最前）:
 *  ver  who      date       modification
 * ----- -------  ---------- ---------------------------------
 * 1.0   ---      26/05/30   创建文件
 */

#include <string.h>

#include "daq_config.h"
#include "daq_protocol.h"

static u8 g_rx_cache[DAQ_PROTOCOL_RX_CACHE_SIZE];
static u32 g_rx_cache_len;

/**
 * @brief  按小端格式读取 32 位无符号整数
 * @param  buf 输入字节缓冲区
 * @return 读取到的 32 位无符号整数
 */
static u32 daq_protocol_read_u32_le(const u8 *buf)
{
    return ((u32)buf[0]) | ((u32)buf[1] << 8) | ((u32)buf[2] << 16) | ((u32)buf[3] << 24);
}

/**
 * @brief  按小端格式写入 32 位无符号整数
 * @param  buf 输出字节缓冲区
 * @param  value 待写入的 32 位无符号整数
 */
static void daq_protocol_write_u32_le(u8 *buf, u32 value)
{
    buf[0] = (u8)(value & 0xFFU);
    buf[1] = (u8)((value >> 8) & 0xFFU);
    buf[2] = (u8)((value >> 16) & 0xFFU);
    buf[3] = (u8)((value >> 24) & 0xFFU);
}

/**
 * @brief  从协议接收缓存头部丢弃指定字节数
 * @param  drop_len 需要丢弃的字节数
 */
static void daq_protocol_drop_bytes(u32 drop_len)
{
    if (drop_len >= g_rx_cache_len) {
        g_rx_cache_len = 0U;
        return;
    }

    memmove(g_rx_cache, &g_rx_cache[drop_len], g_rx_cache_len - drop_len);
    g_rx_cache_len -= drop_len;
}

/**
 * @brief  初始化协议解析缓存
 */
void daq_protocol_init(void)
{
    g_rx_cache_len = 0U;
}

/**
 * @brief  输入 TCP 字节流并解析完整协议帧
 * @param  data 输入字节流
 * @param  len 输入字节数
 * @param  handler 完整协议帧回调函数
 * @param  context 回调上下文指针
 * @return 0 表示成功，负数表示输入参数或缓存状态错误
 * @note   支持 TCP 半包和粘包，会在缓存中等待完整协议帧
 */
int daq_protocol_input(const u8 *data, u32 len, daq_protocol_handler_t handler, void *context)
{
    u32 offset;
    u32 data_size;
    u32 frame_size;
    u32 frame_tail;
    daq_protocol_frame_t frame;

    if ((data == NULL) || (handler == NULL)) {
        return -1;
    }

    if (len > (DAQ_PROTOCOL_RX_CACHE_SIZE - g_rx_cache_len)) {
        g_rx_cache_len = 0U;
        return -2;
    }

    memcpy(&g_rx_cache[g_rx_cache_len], data, len);
    g_rx_cache_len += len;

    while (g_rx_cache_len >= 4U) {
        offset = 0U;
        while ((offset + 4U) <= g_rx_cache_len) {
            if (daq_protocol_read_u32_le(&g_rx_cache[offset]) == DAQ_PROTOCOL_HEAD) {
                break;
            }
            offset++;
        }

        if (offset > 0U) {
            daq_protocol_drop_bytes(offset);
        }

        if (g_rx_cache_len < DAQ_PROTOCOL_HEADER_SIZE) {
            break;
        }

        data_size = daq_protocol_read_u32_le(&g_rx_cache[8]);
        if (data_size > DAQ_PROTOCOL_MAX_PAYLOAD_SIZE) {
            daq_protocol_drop_bytes(1U);
            continue;
        }

        frame_size = DAQ_PROTOCOL_HEADER_SIZE + data_size + DAQ_PROTOCOL_TAIL_SIZE;
        if (g_rx_cache_len < frame_size) {
            break;
        }

        frame_tail = daq_protocol_read_u32_le(&g_rx_cache[DAQ_PROTOCOL_HEADER_SIZE + data_size]);
        if (frame_tail != DAQ_PROTOCOL_TAIL) {
            daq_protocol_drop_bytes(1U);
            continue;
        }

        frame.data_type = daq_protocol_read_u32_le(&g_rx_cache[4]);
        frame.data_size = data_size;
        frame.payload = &g_rx_cache[DAQ_PROTOCOL_HEADER_SIZE];
        handler(&frame, context);
        daq_protocol_drop_bytes(frame_size);
    }

    return 0;
}

/**
 * @brief  从协议帧数据区读取 32 位参数
 * @param  frame 协议帧
 * @param  value 输出参数值
 * @return 0 表示成功，负数表示协议帧无效
 */
int daq_protocol_get_u32_payload(const daq_protocol_frame_t *frame, u32 *value)
{
    if ((frame == NULL) || (value == NULL) || (frame->payload == NULL)) {
        return -1;
    }

    if (frame->data_size < 4U) {
        return -2;
    }

    *value = daq_protocol_read_u32_le(frame->payload);
    return 0;
}

/**
 * @brief  构建带 32 位数据区的应答帧
 * @param  data_type 协议数据类型
 * @param  value 应答数据区数值
 * @param  out_buf 输出缓冲区
 * @param  out_size 输出缓冲区大小
 * @param  out_len 输出帧长度
 * @return 0 表示成功，负数表示参数或缓冲区大小错误
 */
int daq_protocol_build_u32_response(u32 data_type, u32 value, u8 *out_buf, u32 out_size, u32 *out_len)
{
    if ((out_buf == NULL) || (out_len == NULL)) {
        return -1;
    }

    if (out_size < (DAQ_PROTOCOL_HEADER_SIZE + 4U + DAQ_PROTOCOL_TAIL_SIZE)) {
        return -2;
    }

    daq_protocol_write_u32_le(&out_buf[0], DAQ_PROTOCOL_HEAD);
    daq_protocol_write_u32_le(&out_buf[4], data_type);
    daq_protocol_write_u32_le(&out_buf[8], 4U);
    daq_protocol_write_u32_le(&out_buf[12], value);
    daq_protocol_write_u32_le(&out_buf[16], DAQ_PROTOCOL_TAIL);
    *out_len = 20U;

    return 0;
}

/**
 * @brief  构建数据上传协议帧
 * @param  packet_index 数据包序号
 * @param  data 采集数据缓冲区
 * @param  data_len 采集数据长度
 * @param  out_buf 输出协议帧缓冲区
 * @param  out_size 输出缓冲区大小
 * @param  out_len 输出协议帧长度
 * @return 0 表示成功，负数表示参数或缓冲区大小错误
 */
int daq_protocol_build_data_frame(u32 packet_index, const u8 *data, u32 data_len, u8 *out_buf, u32 out_size, u32 *out_len)
{
    u32 payload_len;

    if ((data == NULL) || (out_buf == NULL) || (out_len == NULL)) {
        return -1;
    }

    payload_len = DAQ_PROTOCOL_PACKET_INDEX_SIZE + data_len;
    if ((payload_len > DAQ_PROTOCOL_MAX_PAYLOAD_SIZE) || (out_size < (DAQ_PROTOCOL_HEADER_SIZE + payload_len + DAQ_PROTOCOL_TAIL_SIZE))) {
        return -2;
    }

    daq_protocol_write_u32_le(&out_buf[0], DAQ_PROTOCOL_HEAD);
    daq_protocol_write_u32_le(&out_buf[4], DAQ_PROTOCOL_TYPE_DATA_UPLOAD);
    daq_protocol_write_u32_le(&out_buf[8], payload_len);
    daq_protocol_write_u32_le(&out_buf[12], packet_index);
    memcpy(&out_buf[16], data, data_len);
    daq_protocol_write_u32_le(&out_buf[16 + data_len], DAQ_PROTOCOL_TAIL);
    *out_len = DAQ_PROTOCOL_HEADER_SIZE + payload_len + DAQ_PROTOCOL_TAIL_SIZE;

    return 0;
}
