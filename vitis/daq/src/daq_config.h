/*
 * 文件 : daq_config.h
 * 描述 : 数采系统公共配置
 * 版本 : v1.0
 * 日期 : 2026/05/30
 *
 * 修改记录（最新版本在最前）:
 *  ver  who      date       modification
 * ----- -------  ---------- ---------------------------------
 * 1.0   ---      26/05/30   创建文件
 */

#ifndef DAQ_CONFIG_H_
#define DAQ_CONFIG_H_

#define DAQ_TCP_PORT                     7U
#define DAQ_DMA_RX_BUFFER_SIZE           1024U
#define DAQ_PROTOCOL_RX_CACHE_SIZE       2048U
#define DAQ_PROTOCOL_HEADER_SIZE         12U
#define DAQ_PROTOCOL_TAIL_SIZE           4U
#define DAQ_PROTOCOL_PACKET_INDEX_SIZE   4U
#define DAQ_PROTOCOL_MAX_PAYLOAD_SIZE    (DAQ_DMA_RX_BUFFER_SIZE + DAQ_PROTOCOL_PACKET_INDEX_SIZE)
#define DAQ_PROTOCOL_MAX_FRAME_SIZE      (DAQ_PROTOCOL_HEADER_SIZE + DAQ_PROTOCOL_MAX_PAYLOAD_SIZE + DAQ_PROTOCOL_TAIL_SIZE)

#define DAQ_PROTOCOL_HEAD                0xA5A5A5A5U
#define DAQ_PROTOCOL_TAIL                0xB5B5B5B5U
#define DAQ_PROTOCOL_TYPE_RESET          0xC00000FFU
#define DAQ_PROTOCOL_TYPE_CONTROL        0xC00100FFU
#define DAQ_PROTOCOL_TYPE_DATA_UPLOAD    0xF00000FFU

#define DAQ_CONTROL_STOP                 0U
#define DAQ_CONTROL_START                1U
#define DAQ_STATUS_OK                    0U
#define DAQ_STATUS_ERROR                 1U

#endif
