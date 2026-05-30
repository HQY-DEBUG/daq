/*
 * 文件 : daq_app.c
 * 描述 : DMA 到 TCP 的连续采集调度状态机
 * 版本 : v1.0
 * 日期 : 2026/05/30
 *
 * 修改记录（最新版本在最前）:
 *  ver  who      date       modification
 * ----- -------  ---------- ---------------------------------
 * 1.0   ---      26/05/30   创建文件
 */

#include "daq_app.h"
#include "daq_config.h"
#include "daq_protocol.h"
#include "dma_rx.h"
#include "tcp_server.h"
#include "xil_printf.h"

typedef enum {
    DAQ_APP_STATE_IDLE = 0,
    DAQ_APP_STATE_DMA_WAIT,
    DAQ_APP_STATE_TCP_SEND,
    DAQ_APP_STATE_WAIT_PC_ACK
} daq_app_state_t;

static daq_app_state_t g_daq_state;
static int g_is_collecting;
static u32 g_packet_index;
static u8 g_tx_frame[DAQ_PROTOCOL_MAX_FRAME_SIZE];
static u32 g_tx_frame_len;

/**
 * @brief  发送带 32 位数据区的协议应答
 * @param  data_type 协议数据类型
 * @param  value 应答数据区数值
 * @return 0 表示成功，负数表示组包或发送失败
 */
static int daq_app_send_u32_response(u32 data_type, u32 value)
{
    int ret;
    u32 response_len;
    u8 response_buf[DAQ_PROTOCOL_HEADER_SIZE + 4U + DAQ_PROTOCOL_TAIL_SIZE];

    ret = daq_protocol_build_u32_response(data_type, value, response_buf, sizeof(response_buf), &response_len);
    if (ret != 0) {
        xil_printf("build response failed, ret=%d\r\n", ret);
        return ret;
    }

    ret = tcp_server_send(response_buf, response_len);
    if (ret != 0) {
        xil_printf("send response failed, ret=%d\r\n", ret);
        return ret;
    }

    return 0;
}

/**
 * @brief  处理系统复位命令
 * @note   当前复位命令清除采集状态并复位 DMA 接收状态
 */
static void daq_app_handle_reset_command(void)
{
    g_is_collecting = 0;
    g_daq_state = DAQ_APP_STATE_IDLE;
    g_packet_index = 0U;
    dma_rx_abort();
    (void)daq_app_send_u32_response(DAQ_PROTOCOL_TYPE_RESET, DAQ_STATUS_OK);
}

/**
 * @brief  处理开始采集和停止采集命令
 * @param  frame 控制命令协议帧
 */
static void daq_app_handle_control_command(const daq_protocol_frame_t *frame)
{
    int ret;
    u32 control_value;

    ret = daq_protocol_get_u32_payload(frame, &control_value);
    if (ret != 0) {
        xil_printf("invalid control command, ret=%d\r\n", ret);
        (void)daq_app_send_u32_response(DAQ_PROTOCOL_TYPE_CONTROL, DAQ_STATUS_ERROR);
        return;
    }

    if (control_value == DAQ_CONTROL_START) {
        g_is_collecting = 1;
        g_daq_state = DAQ_APP_STATE_IDLE;
        g_packet_index = 0U;
        (void)daq_app_send_u32_response(DAQ_PROTOCOL_TYPE_CONTROL, DAQ_CONTROL_START);
        return;
    }

    if (control_value == DAQ_CONTROL_STOP) {
        g_is_collecting = 0;
        g_daq_state = DAQ_APP_STATE_IDLE;
        dma_rx_abort();
        (void)daq_app_send_u32_response(DAQ_PROTOCOL_TYPE_CONTROL, DAQ_CONTROL_STOP);
        return;
    }

    xil_printf("unknown control value=%d\r\n", (int)control_value);
    (void)daq_app_send_u32_response(DAQ_PROTOCOL_TYPE_CONTROL, DAQ_STATUS_ERROR);
}

/**
 * @brief  处理 PC 返回的数据上传应答
 * @param  frame 数据上传应答协议帧
 */
static void daq_app_handle_data_ack(const daq_protocol_frame_t *frame)
{
    int ret;
    u32 ack_index;

    ret = daq_protocol_get_u32_payload(frame, &ack_index);
    if (ret != 0) {
        xil_printf("invalid data ack, ret=%d\r\n", ret);
        return;
    }

    if ((g_daq_state == DAQ_APP_STATE_WAIT_PC_ACK) && (ack_index == g_packet_index)) {
        g_packet_index++;
        g_daq_state = DAQ_APP_STATE_IDLE;
        return;
    }

    xil_printf("unexpected data ack=%d, current=%d, state=%d\r\n", (int)ack_index, (int)g_packet_index, g_daq_state);
}

/**
 * @brief  协议解析完成后的分发回调
 * @param  frame 已解析的协议帧
 * @param  context 回调上下文指针
 */
static void daq_app_protocol_handler(const daq_protocol_frame_t *frame, void *context)
{
    (void)context;

    if (frame->data_type == DAQ_PROTOCOL_TYPE_RESET) {
        daq_app_handle_reset_command();
        return;
    }

    if (frame->data_type == DAQ_PROTOCOL_TYPE_CONTROL) {
        daq_app_handle_control_command(frame);
        return;
    }

    if (frame->data_type == DAQ_PROTOCOL_TYPE_DATA_UPLOAD) {
        daq_app_handle_data_ack(frame);
        return;
    }

    xil_printf("unknown protocol type=0x%08lx\r\n", frame->data_type);
}

/**
 * @brief  初始化数采应用状态机和 DMA 接收模块
 * @return 0 表示成功，负数表示初始化失败
 */
int daq_app_init(void)
{
    int ret;

    daq_protocol_init();
    g_daq_state = DAQ_APP_STATE_IDLE;
    g_is_collecting = 0;
    g_packet_index = 0U;
    g_tx_frame_len = 0U;

    ret = dma_rx_init();
    if (ret != 0) {
        xil_printf("dma_rx_init failed, ret=%d\r\n", ret);
        return ret;
    }

    return 0;
}

/**
 * @brief  执行一次数采应用状态机调度
 * @return 0 表示成功，负数表示 DMA 接收或 TCP 发送失败
 * @note   主循环周期调用，负责 DMA 接收完成后组包并等待 PC 应答
 */
int daq_app_process(void)
{
    int ret;

    if ((g_is_collecting == 0) || !tcp_server_has_client()) {
        return 0;
    }

    if (g_daq_state == DAQ_APP_STATE_IDLE) {
        ret = dma_rx_start();
        if (ret != 0) {
            return ret;
        }
        g_daq_state = DAQ_APP_STATE_DMA_WAIT;
        return 0;
    }

    if (g_daq_state == DAQ_APP_STATE_DMA_WAIT) {
        if (!dma_rx_is_done()) {
            return 0;
        }

        dma_rx_invalidate_cache();
        ret = daq_protocol_build_data_frame(g_packet_index, dma_rx_get_buffer(), dma_rx_get_length(), g_tx_frame, sizeof(g_tx_frame), &g_tx_frame_len);
        if (ret != 0) {
            xil_printf("build data frame failed, ret=%d\r\n", ret);
            return ret;
        }

        g_daq_state = DAQ_APP_STATE_TCP_SEND;
    }

    if (g_daq_state == DAQ_APP_STATE_TCP_SEND) {
        ret = tcp_server_send(g_tx_frame, g_tx_frame_len);
        if (ret == -2) {
            return 0;
        }

        if (ret != 0) {
            xil_printf("send data frame failed, ret=%d\r\n", ret);
            return ret;
        }

        g_daq_state = DAQ_APP_STATE_WAIT_PC_ACK;
        return 0;
    }

    return 0;
}

/**
 * @brief  处理 TCP 客户端连接事件
 */
void daq_app_on_tcp_connected(void)
{
    g_daq_state = DAQ_APP_STATE_IDLE;
}

/**
 * @brief  处理 TCP 客户端断开事件
 * @note   断开后停止采集并中止 DMA 接收
 */
void daq_app_on_tcp_disconnected(void)
{
    g_is_collecting = 0;
    g_daq_state = DAQ_APP_STATE_IDLE;
    dma_rx_abort();
}

/**
 * @brief  处理 TCP 接收到的原始字节流
 * @param  data TCP 接收数据
 * @param  len TCP 接收数据长度
 */
void daq_app_on_tcp_rx(const u8 *data, u32 len)
{
    int ret;

    ret = daq_protocol_input(data, len, daq_app_protocol_handler, NULL);
    if (ret != 0) {
        xil_printf("daq_protocol_input failed, ret=%d\r\n", ret);
    }
}
