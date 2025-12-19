/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_ping.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 16:31:26 by ktakamat          #+#    #+#             */
/*   Updated: 2025/12/19 15:38:42 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* project/include/ft_ping.h */
#ifndef FT_PING_H
# define FT_PING_H

// --- Libraries ---
# include <stdio.h>      // printf, stderr, dprintf
# include <stdlib.h>     // malloc, free, exit, EXIT_SUCCESS, calloc
# include <unistd.h>     // getuid, close, sleep, usleep, getpid
# include <string.h>     // memset, memcpy, strncpy, strchr, strstr, strncmp
# include <stdbool.h>    // bool, true, false
# include <signal.h>     // signal
# include <sys/time.h>   // gettimeofday
# include <sys/types.h>
# include <sys/socket.h> // socket functions
# include <netdb.h>      // getaddrinfo, addrinfo, NI_MAXHOST
# include <arpa/inet.h>  // inet_ntop, inet_ntoa
# include <netinet/in.h> // sockaddr_in
# include <netinet/ip.h> // struct ip
# include <netinet/ip_icmp.h> // struct icmphdr
# include <errno.h>
# include <math.h>       // sqrt (for stddev)
# include <stdarg.h>     // va_list (for ft_strf)

// --- Constants & Macros ---
# define PACKET_SIZE 64
# define PING_PAYLOAD_SIZE 56 // ICMPヘッダ(8) + ペイロード(56) = 64
# define IP_DF 0x4000         // Don't Fragment flag

// Log Levels
# define DEBUG   1
# define INFO    2
# define WARNING 3
# define ERROR   4

// Log Messages & Formats
# define LOG_CALLOC_FAIL "ping: calloc: memory allocation failed\n"
# define LOG_MISS_HOST "ping: missing host operand\n"
# define LOG_REQ_ARG1 "ping: option requires an argument\n"
# define LOG_REQ_ARG2 "ping: option '%s' requires an argument\n"
# define LOG_REQ_ARG3 "ping: option requires an argument -- '%c'\n"
# define LOG_INVALID_OPT1 "ping: unrecognized option '%s'\n"
# define LOG_INVALID_OPT2 "ping: invalid option -- '%c'\n"
# define LOG_TRY "Try 'ping --help' or 'ping --usage' for more information.\n"
# define LOG_HELP "Usage: ping [OPTION...] HOST ...\nSend ICMP ECHO_REQUEST packets to network hosts.\n"
# define LOG_USAGE "Usage: ping [-v?] [-v verbose] HOST ...\n"
# define LOG_VERSION "ping (42 School) 1.0\n"

// Output Formats
# define LOG_PING_HDR "PING %s (%s): %ld data bytes\n"
# define LOG_BYTES1 "%d bytes from %s: %s\n"
# define LOG_BYTES2 "%d bytes from %s: Time to live exceeded\n"
# define LOG_BYTES3 "%d bytes from %s: icmp_seq=%u ttl=%d time=%.3f ms\n"
# define LOG_STAT "--- %s ping statistics ---\n%d packets transmitted, %d packets received, %d%% packet loss\n"
# define LOG_STAT2 "round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n"
# define LOG_IP_HDR "Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst     Data\n %1d  %1d  %02x %4d %04x  %1d  %04x %3d  %02x %04x %-15s %-15s\n"
# define LOG_ICMP_HDR "ICMP: type %d, code %d, size %d, id 0x%04x, seq 0x%04x\n"
# define LOG_PING_HDR_VERB "PING %s (%s): %ld data bytes, id 0x%04x = %d\n"
// --- Structures ---

typedef struct {
    bool set;
    int val;
} t_opt_int;

typedef struct {
    bool        verbose;
    bool        flood;
    bool        ignore_routing;
    t_opt_int   ttl;
    t_opt_int   tos;
    t_opt_int   timeout;
} t_opt;

typedef struct {
    char    *host;
    t_opt   options;
} t_args;

typedef struct {
    struct icmphdr  header;
    char            payload[PING_PAYLOAD_SIZE];
} t_icmp_pkt;

typedef struct {
    struct ip   *ip_header;
    t_icmp_pkt  *icmp_pkt;
} t_pkt;

typedef struct {
    int send_fd;
    int recv_fd;
} t_sock;

typedef struct {
    char    host[NI_MAXHOST];
    int     pkts_tx;
    int     pkts_rx;
    int     pkt_loss;
    double  rtt_min;
    double  rtt_avg;
    double  rtt_max;
    double  rtt_sum;
    double  rtt_sq_sum;
    double  rtt_stddev;
} t_stats;

typedef struct {
    t_sock  *socks;
    t_pkt   *req;
    t_pkt   *resp;
} t_ptr;

typedef struct {
    t_stats s;
    t_ptr   *p;
} t_g;

// --- Global Variable ---
extern t_g g;

// --- Function Prototypes ---

// main.c / parser.c
void    parser(char **argv, t_args *args);
void    log_mis_opt_arg(const char *raw);

// socket.c
t_sock  *get_socket(const t_args *args);

// ping.c
void    ping(const t_args *args);

// packets.c
void    set_req_packet(t_pkt *req, struct in_addr dst_ip, bool init, t_opt opt);
void    send_packet(int fd, const struct sockaddr_in *dst_ip, t_pkt *req);
void    recv_packet(int fd, const struct sockaddr_in *dst_ip, t_pkt *resp);

// logger.c
void    logger(char *msg, int level, bool to_exit, int exit_code);
void    log_stats(void);
void    log_ping_hdr(const struct in_addr *dst_ip, const t_pkt *req);
void    log_ping_body(const t_pkt *req, const t_pkt *resp, struct in_addr dst_ip, double rtt, t_opt opt);

// verbose.c
void    get_verbose_hdr(const struct in_addr *dst_ip, const t_pkt *req);
char    *get_verbose_body(const t_pkt *resp, const t_pkt *req);

// libft helpers
void    ft_free(void **ptr);
void    *ft_calloc(size_t count, size_t size);
char    *ft_strf(const char *fmt, ...);

#endif