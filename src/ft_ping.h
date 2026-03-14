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

#ifndef FT_PING_H
#define FT_PING_H

#include <netinet/ip_icmp.h>

#define IP_TTL_VALUE 64

#define IP_HDR_SIZE (sizeof(struct iphdr))
#define ICMP_HDR_SIZE (sizeof(struct icmphdr))
#define ICMP_BODY_SIZE 56

enum e_exitcode {
	E_EXIT_OK,
	E_EXIT_ERR_HOST,
	E_EXIT_ERR_ARGS = 64
};

struct options {
	_Bool help;
	_Bool quiet;
	_Bool verb;
};

struct rtt_node {
	struct timeval val;
	struct rtt_node *next;
};

struct packinfo {
	int nb_send;
	int nb_ok;
	struct timeval *min;
	struct timeval *max;
	struct timeval avg;
	struct timeval stddev;
	struct rtt_node *rtt_list;
	struct rtt_node *rtt_last;
};

struct sockinfo {
	char *host;
	struct sockaddr_in remote_addr;
	char str_sin_addr[INET_ADDRSTRLEN];
};

static inline void * skip_iphdr(void *buf)
{
	return (void *)((uint8_t *)buf + IP_HDR_SIZE);
}

static inline void * skip_icmphdr(void *buf)
{
	return (void *)((uint8_t *)buf + ICMP_HDR_SIZE);
}

/* check.c */
int check_rights();
int check_args(int ac, char **av, char **host, struct options *opts);

/* init.c */
int init_sock(int *sock_fd, struct sockinfo *si, char *host, int ttl);

/* icmp.c*/
int icmp_send_ping(int sock_fd, const struct sockinfo *si, struct packinfo *pi);
int icmp_recv_ping(int sock_fd, struct packinfo *pi,
		   const struct options *opts);

/* print.c */
void print_help();
void print_start_info(const struct sockinfo *si, const struct options *opts);
int print_recv_info(void *buf, ssize_t nb_bytes, const struct options *opts,
                    const struct packinfo *pi);
void print_end_info(const struct sockinfo *si, struct packinfo *pi);

/* rtts.c */
struct rtt_node * rtts_save_new(struct packinfo *pi, struct icmphdr *icmph);
void rtts_clean(struct packinfo *pi);
void rtts_calc_stats(struct packinfo *pi);

#endif
