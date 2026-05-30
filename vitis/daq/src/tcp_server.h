/*
 * 文件 : tcp_server.h
 * 描述 : TCP 服务接口声明
 * 版本 : v1.0
 * 日期 : 2026/05/30
 *
 * 修改记录（最新版本在最前）:
 *  ver  who      date       modification
 * ----- -------  ---------- ---------------------------------
 * 1.0   ---      26/05/30   创建文件
 */

#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_

void tcp_server_print_header(void);
int tcp_server_start(void);
int tcp_server_transfer_data(void);

#endif
