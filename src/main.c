/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 17:10:43 by ktakamat          #+#    #+#             */
/*   Updated: 2025/11/25 19:48:50 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

t_g g;

static void	free_ptr(void) {
    if (!g.p) return;
	ft_free((void **)&g.p->socks);
    if (g.p->req)
	    ft_free((void **)&g.p->req->ip_header);
	ft_free((void **)&g.p->req);
    if (g.p->resp)
	f   t_free((void **)&g.p->resp->ip_header);
	ft_free((void **)&g.p->resp);
	ft_free((void **)&g.p);
}

static void sigint_handler(int sig) {
    if (sig == SIGINT) {
        free_ptr();
        if (g.s.pkt_loss || g.s.pkts_rx || g.s.pkts_tx)
            log_stats();
        exit(EXIT_SUCCESS);
    }
}

int main(const int argc, char **argv) {
    t_args  args;
    
    if (getuid() != 0)
        logger("ping: Lacking privilege for raw socket.\n", ERROR, true, 1);
    if (argc == 1)
        logger(LOG_MISS_HOST, WARNING, true, 64);

    if (!(g.p = calloc(1, sizeof(t_ptr))))
        logger("ping: calloc failed", ERROR, true, 64);
    memset(&args, 0, sizeof(args));
    parser(argv, &args);
    if (!args.host)
        logger(LOG_MISS_HOST, WARNING, true, 64);
    else {
        strncpy(g.s.host, args.host, sizeof(g.s.host) -1);
        g.s.host[sizeof(g.s.host) -1] = '\0';
    }
    signal(SIGINT, sigint_handler);
    ping(&args);
    return EXIT_SUCCESS;
}
