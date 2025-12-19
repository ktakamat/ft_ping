/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ping.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/03 13:06:38 by ktakamat          #+#    #+#             */
/*   Updated: 2025/12/19 15:05:30 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

// reqとrespの両方を解放する関数
static void clean_pkt(t_pkt *req, t_pkt *resp) {
    // 受信パケットの解放
    if (resp->ip_header) {
        free(resp->ip_header);
        resp->ip_header = NULL;
        // icmp_pktはip_headerと同じブロック内にあるため個別のfreeは不要
        resp->icmp_pkt = NULL; 
    }

    // 【重要】送信パケットの解放 (packets.cでmallocしているので必須)
    if (req->ip_header) {
        free(req->ip_header);
        req->ip_header = NULL;
    }
    if (req->icmp_pkt) {
        free(req->icmp_pkt);
        req->icmp_pkt = NULL;
    }
}

// 統計情報の更新
static void set_rtt_stats(double rtt) {
    if (g.s.pkts_rx == 1) {
        g.s.rtt_min = rtt;
        g.s.rtt_max = rtt;
    } else {
        if (rtt < g.s.rtt_min) g.s.rtt_min = rtt;
        if (rtt > g.s.rtt_max) g.s.rtt_max = rtt;
    }
    g.s.rtt_sum += rtt;
    g.s.rtt_sq_sum += (rtt * rtt);
}

// 1回分のパケット送受信
static void run(const t_sock *socks, const t_args *args,
                const struct sockaddr_in *dst_ip, bool init) {
    t_pkt req;
    t_pkt resp;
    struct timeval ts_start, ts_end;
    double rtt;

    memset(&req, 0, sizeof(req));
    memset(&resp, 0, sizeof(resp));

    // パケット作成 (内部でmallocされる)
    set_req_packet(&req, dst_ip->sin_addr, init, args->options);

    if (init) {
        if (args->options.verbose)
            get_verbose_hdr(&dst_ip->sin_addr, &req);
        else
            log_ping_hdr(&dst_ip->sin_addr, &req);
    }

    gettimeofday(&ts_start, NULL);
    send_packet(socks->send_fd, dst_ip, &req);

    recv_packet(socks->recv_fd, dst_ip, &resp);
    gettimeofday(&ts_end, NULL);

    rtt = ((ts_end.tv_sec - ts_start.tv_sec) * 1000.0) +
          ((ts_end.tv_usec - ts_start.tv_usec) / 1000.0);

    if (g.s.pkts_rx > 0 && resp.ip_header != NULL) {
        set_rtt_stats(rtt);
        log_ping_body(&req, &resp, dst_ip->sin_addr, rtt, args->options);
    }

    // 【重要】メモリのお掃除
    clean_pkt(&req, &resp);

    if (!args->options.flood) {
        double sleep_usec = 1000000.0 - (rtt * 1000.0);
        if (sleep_usec > 0)
            usleep((useconds_t)sleep_usec);
    }
}

// ホスト名解決
static void resolve_host(struct sockaddr_in *dst_ip, const t_args *args) {
    struct addrinfo hints, *res;
    int status;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;

    if ((status = getaddrinfo(args->host, NULL, &hints, &res)) != 0) {
        fprintf(stderr, "ping: %s: %s\n", args->host, gai_strerror(status));
        exit(2);
    }

    memcpy(dst_ip, res->ai_addr, sizeof(struct sockaddr_in));
    // main.cでホスト名はセット済みなので上書きしない
    freeaddrinfo(res);
}

void ping(const t_args *args) {
    t_sock *socks;
    struct sockaddr_in dst_ip;
    struct timeval start_all, now;
    bool init = true;

    resolve_host(&dst_ip, args);
    socks = get_socket(args);
    g.p->socks = socks;

    gettimeofday(&start_all, NULL);
    
    while (true) {
        if (args->options.timeout.set) {
            gettimeofday(&now, NULL);
            double elapsed = (now.tv_sec - start_all.tv_sec) +
                    (now.tv_usec - start_all.tv_usec) / 1e6;
            if (elapsed >= args->options.timeout.val) {
                log_stats();
                break;
            }
        }
        run(socks, args, &dst_ip, init);
        init = false;
    }

    close(socks->send_fd);
    close(socks->recv_fd);
}
//run(現場作業員)
//役割:実際に一回分のpingを打つ作業員です
//処理:1.作成：送るパケット（データ）を作る（set_req_packet)
//2.送信:パケットを投げる(send_packet)同時にストップウォッチを押す。
//3.受信:返事を来るのを待つ(recv_packet)
//4.計算:返事が来たら、かかった時間（RTT)を計算する。
//5.記録:「今の記録は〇〇ｍｓでした」とノートに書く（set_rtt_stats)
//6.休憩:次の１秒が来るまで休む(usleep)