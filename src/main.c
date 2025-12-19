/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 17:10:43 by ktakamat          #+#    #+#             */
/*   Updated: 2025/12/19 15:41:23 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

t_g g;

static void free_ptr(void) {
    if (!g.p)
        return;

    // ソケット構造体の解放
    ft_free((void **)&g.p->socks);

    // 送信パケット構造体の解放
    if (g.p->req) {
        ft_free((void **)&g.p->req->ip_header);
        ft_free((void **)&g.p->req->icmp_pkt);
    }
    ft_free((void **)&g.p->req);

    // 受信パケット構造体の解放
    if (g.p->resp) {
        ft_free((void **)&g.p->resp->ip_header);
        if (g.p->resp->icmp_pkt)
            ft_free((void **)&g.p->resp->icmp_pkt);
    }
    ft_free((void **)&g.p->resp);

    // 管理構造体の解放
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


// 全体の流れ
// 起動(main):プログラムが始まる
// 解読(parser):ユーザーの注文(宛先やオプション)を開く
// 開通(socket):通信回線を開く
// 実行(ping):実際にパケットを投げて、返事を待つ(無限ループ)
// 終了:結果を表示して終わる