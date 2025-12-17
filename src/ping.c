/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ping.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/03 13:06:38 by ktakamat          #+#    #+#             */
/*   Updated: 2025/12/12 16:16:05 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

// メモリ解放用関数 (これを追加しないとメモリリークします)
static void clean_pkt(t_pkt *req, t_pkt *resp) {
    // 受信パケットの解放 (recv_packetで確保されたrbuf)
    if (resp->ip_header) {
        free(resp->ip_header);
        resp->ip_header = NULL;
        resp->icmp_pkt = NULL; // 同じバッファ内なのでポインタを消すだけでOK
    }

    // 送信パケットの解放 (set_req_packetで確保された領域)
    if (req->ip_header) {
        free(req->ip_header);
        req->ip_header = NULL;
    }
    if (req->icmp_pkt) {
        free(req->icmp_pkt);
        req->icmp_pkt = NULL;
    }
}

// 統計情報の更新(最小、最大、平均、分散計算用)
static void set_rtt_stats(double rtt) {
    // 最初の1回目
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

// 一回分のパケットの受送信サイクル
static void run(const t_sock *socks, const t_args *args,
                const struct sockaddr_in *dst_ip, bool init) {
    t_pkt req;
    t_pkt resp;
    struct timeval ts_start, ts_end;
    double rtt;

    // 構造体の初期化
    memset(&req, 0, sizeof(req));
    memset(&resp, 0, sizeof(resp));

    // パケットの作成 (内部でmallocされる)
    set_req_packet(&req, dst_ip->sin_addr, init, args->options);

    // ログ表示
    if (init) {
        if (args->options.verbose)
            get_verbose_hdr(&dst_ip->sin_addr, &req);
        else
            log_ping_hdr(&dst_ip->sin_addr, &req); // typo修正: long -> log
    }

    // 送信
    gettimeofday(&ts_start, NULL);
    send_packet(socks->send_fd, dst_ip, &req);

    // 受信 (タイムアウトまで待機)
    recv_packet(socks->recv_fd, dst_ip, &resp);
    gettimeofday(&ts_end, NULL);

    // RTT計測(ミリ秒)
    rtt = ((ts_end.tv_sec - ts_start.tv_sec) * 1000.0) +
          ((ts_end.tv_usec - ts_start.tv_usec) / 1000.0);

    // 受信に成功していれば統計更新
    if (g.s.pkts_rx > 0 && resp.ip_header != NULL) {
        set_rtt_stats(rtt);
        log_ping_body(&req, &resp, dst_ip->sin_addr, rtt, args->options); // typo修正
    }

    // 【重要】メモリのお掃除
    clean_pkt(&req, &resp);

    // 1秒間隔の調整 (Floodモード以外)
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
        // エラー時は標準エラー出力へ
        fprintf(stderr, "ping: %s: %s\n", args->host, gai_strerror(status));
        exit(2);
    }

    // 最初に見つかったアドレスをコピー
    memcpy(dst_ip, res->ai_addr, sizeof(struct sockaddr_in));

    // メモ: g.s.hostの上書きは削除しました。
    // main.cでセットされたホスト名を維持するためです。

    freeaddrinfo(res);
}

// 全体の監督
void ping(const t_args *args) {
    t_sock *socks; // typo修正: t_stock -> t_sock
    struct sockaddr_in dst_ip;
    struct timeval start_all, now;
    bool init = true;

    // 名前解決
    resolve_host(&dst_ip, args);
    
    // ソケット作成
    socks = get_socket(args);
    g.p->socks = socks;

    gettimeofday(&start_all, NULL); // typo修正: . -> ,
    
    // 無限ループ
    while (true) {
        // -w タイムアウト判定
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