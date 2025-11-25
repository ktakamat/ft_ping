/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_ping.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 16:31:26 by ktakamat          #+#    #+#             */
/*   Updated: 2025/11/25 18:50:45 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_PING_H
# define FT_PING_H

// --- Libraries ---
# include <stdio.h>      // printf, stderr
# include <stdlib.h>     // malloc, free, exit, EXIT_SUCCESS
# include <unistd.h>     // getuid, close, sleep
# include <string.h>     // memset, memcpy, strncpy
# include <stdbool.h>    // bool, true, false
# include <signal.h>     // signal
# include <sys/time.h>   // gettimeofday
# include <sys/types.h>
# include <sys/socket.h> // socket functions
# include <netdb.h>      // getaddrinfo, addrinfo, NI_MAXHOST
# include <arpa/inet.h>  // inet_ntop
# include <netinet/in.h> // sockaddr_in
# include <netinet/ip.h> // struct ip
# include <netinet/ip_icmp.h> // struct icmphdr
# include <errno.h>
# include <math.h>       // sqrt (for stddev)

// --- Constants & Macros ---
# define PACKET_SIZE 64
# define PING_PAYLOAD_SIZE 56 // ICMPヘッダ(8) + ペイロード(56) = 64

// Log Levels
# define DEBUG   1
# define INFO    2
# define WARNING 3
# define ERROR   4

// Log Messages
# define LOG_CALLOC_FAIL "ping: calloc: memory allocation failed\n"
# define LOG_MISS_HOST "ping: missing host operand\n"
# define LOG_REQ_ARG1 "ping: option requires an argument\n"
# define LOG_INVALID_OPT1 "ping: unrecognized option '%s'\n"
# define LOG_TRY "Try 'ping --help' or 'ping --usage' for more information.\n"
# define LOG_HELP "Usage: ping [OPTION...] HOST ...\nSend ICMP ECHO_REQUEST packets to network hosts.\n"
# define LOG_USAGE "Usage: ping [-v?] [-v verbose] HOST ...\n" // 簡易版
# define LOG_VERSION "ping (42 School) 1.0\n"

// --- Structures ---

// オプション管理用 (TTLなど値を持つオプション)
typedef struct {
    bool set;
    int val;
} t_opt_int;

// コマンドラインオプション全体
typedef struct {
    bool        verbose;
    bool        flood;
    bool        ignore_routing; // -r
    t_opt_int   ttl;            // --ttl
    t_opt_int   tos;            // --tos
    t_opt_int   timeout;        // -w
} t_opt;

// 解析済み引数
typedef struct {
    char    *host;      // コマンドラインで指定されたホスト名文字列
    t_opt   options;    // 解析されたオプション
} t_args;

// パケット送受信用構造体
typedef struct {
    struct icmphdr  header;
    char            payload[PING_PAYLOAD_SIZE];
} t_icmp_pkt;

// 受信パケット解析用 (IPヘッダ + ICMP)
typedef struct {
    struct ip   *ip_header; // Rawソケットで受け取るとIPヘッダが付いている
    t_icmp_pkt  *icmp_pkt;  // その後ろにICMPパケットがある
} t_pkt;

// ソケット管理
typedef struct {
    int send_fd; // 送信用 (SOCK_RAW, IPPROTO_RAW)
    int recv_fd; // 受信用 (SOCK_RAW, IPPROTO_ICMP)
} t_sock;

// 統計情報 (Ctrl+Cを押したときに表示するもの)
typedef struct {
    char    host[NI_MAXHOST]; // ホスト名 (修正: 十分なサイズを確保)
    int     pkts_tx;          // 送信パケット数
    int     pkts_rx;          // 受信パケット数
    int     pkt_loss;         // 損失率計算用
    double  rtt_min;
    double  rtt_avg;
    double  rtt_max;
    double  rtt_sum;          // 平均計算用
    double  rtt_sq_sum;       // 標準偏差計算用 (二乗和)
} t_stats;

// メモリ解放用ポインタまとめ
typedef struct {
    t_sock  *socks;
    t_pkt   *req;  // 送信パケット構築用エリア
    t_pkt   *resp; // 受信パケット格納用エリア
} t_ptr;

// グローバル構造体 (シグナルハンドラからアクセスするため)
typedef struct {
    t_stats s; // 統計情報
    t_ptr   *p; // ポインタ管理
} t_g;

// --- Global Variable Declaration ---
extern t_g g;

// --- Function Prototypes ---

// main.c / parsing
void    parser(char **argv, t_args *args);

// socket.c
t_sock  *get_socket(const t_args *args);

// ping.c
void    ping(const t_args *args);

// logger.c
void    logger(char *msg, int level, bool to_exit, int exit_code);
void    log_stats(void);

// libft helpers (簡易定義)
void    ft_free(void **ptr);

#endif