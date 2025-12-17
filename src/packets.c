/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   packets.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/08 18:48:03 by ktakamat          #+#    #+#             */
/*   Updated: 2025/12/12 16:04:46 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "ft_ping.h"

// 統計情報の更新
static void set_rtt_stats(const t_pkt *resp, double rtt) {
    if (resp->ip_header) {
        // 初回受信時の初期化
        if (g.s.pkts_rx == 1) {
            g.s.rtt_min = rtt;
            g.s.rtt_max = rtt;
        } else {
            g.s.rtt_min = (rtt < g.s.rtt_min) ? rtt : g.s.rtt_min;
            g.s.rtt_max = (rtt > g.s.rtt_max) ? rtt : g.s.rtt_max;
        }

        // 平均と標準偏差用の累積加算
        g.s.rtt_sum += rtt;
        g.s.rtt_sq_sum += (rtt * rtt);
        
        // 逐次計算する場合
        g.s.rtt_avg = g.s.rtt_sum / g.s.pkts_rx;
        // 標準偏差はルート計算が重いので、log_statsでの最後に計算するのが一般的ですが
        // ここでやるなら以下です（負の数チェックを追加）
        double variance = (g.s.rtt_sq_sum / g.s.pkts_rx) - (g.s.rtt_avg * g.s.rtt_avg);
        g.s.rtt_stddev = sqrt((variance < 0) ? 0 : variance);
    }
}

// メモリ解放（受信パケットのみ解放）
static void clean_resp(t_pkt *resp) {
    if (resp->ip_header) {
        free(resp->ip_header);
        resp->ip_header = NULL;
    }
    if (resp->icmp_pkt) {
        free(resp->icmp_pkt);
        resp->icmp_pkt = NULL;
    }
    // req (送信パケット) は packets.c で static バッファを使っているため
    // ここで free してはいけません。
}

static void run(
    const t_sock *socks,
    const t_args *args, 
    const struct sockaddr_in *dst_ip, // ポインタ渡しに変更
    const bool init
) {
    t_pkt req;   // スタック変数に変更（malloc/freeのオーバーヘッド削減）
    t_pkt resp;  // スタック変数に変更
    struct timeval ts_start, ts_end;
    double rtt;

    // 構造体クリア
    memset(&req, 0, sizeof(req));
    memset(&resp, 0, sizeof(resp));

    // パケット作成 (reqの中身は packets.c 内の static バッファを指すようになる)
    set_req_packet(&req, dst_ip->sin_addr, init, args->options);

    // ログ表示
    if (init) {
        if (args->options.verbose)
            get_verbose_hdr(&dst_ip->sin_addr, &req);
        else
            log_ping_hdr(&dst_ip->sin_addr, &req);
    }

    // 送信
    gettimeofday(&ts_start, NULL);
    send_packet(socks->send_fd, dst_ip, &req);

    // 受信待機
    recv_packet(socks->recv_fd, dst_ip, &resp);
    gettimeofday(&ts_end, NULL);

    // RTT計算
    rtt = ((ts_end.tv_sec - ts_start.tv_sec) * 1000.0) + \
          ((ts_end.tv_usec - ts_start.tv_usec) / 1000.0);

    // 受信成功していれば統計更新
    if (g.s.pkts_rx > 0 && resp.ip_header) {
        set_rtt_stats(&resp, rtt);
        log_ping_body(&req, &resp, dst_ip->sin_addr, rtt, args->options);
    }

    // 受信メモリのお掃除
    clean_resp(&resp);

    // 待機 (Floodモード以外)
    if (!args->options.flood) {
        double sleep_time = 1000000.0 - (rtt * 1000.0);
        if (sleep_time > 0)
            usleep((useconds_t)sleep_time);
    }
}

// ホスト名の解決 (getaddrinfo版)
static void set_dst_ip(struct sockaddr_in *dst_ip, const t_args *args) {
    struct addrinfo hints, *res;
    int status;

    memset(dst_ip, 0, sizeof(*dst_ip));
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;

    status = getaddrinfo(args->host, NULL, &hints, &res);
    if (status != 0) {
        // エラー表示して終了
        fprintf(stderr, "ping: %s: %s\n", args->host, gai_strerror(status));
        exit(2);
    }

    // アドレスをコピー
    memcpy(dst_ip, res->ai_addr, sizeof(struct sockaddr_in));
    
    // 統計表示用にIPアドレス文字列を保存
    inet_ntop(AF_INET, &dst_ip->sin_addr, g.s.host, sizeof(g.s.host));

    freeaddrinfo(res);
}

void ping(const t_args *args) {
    t_sock *socks;
    double elapsed;
    struct sockaddr_in dst_ip;
    struct timeval ts_now, ts_start;
    bool init;

    set_dst_ip(&dst_ip, args);
    socks = get_socket(args);
    g.p->socks = socks; // グローバルに保存 (シグナル時のclose用)

    gettimeofday(&ts_start, NULL);
    init = true;

    while (true) {
        // タイムアウト判定 (-w)
        if (args->options.timeout.set) {
            gettimeofday(&ts_now, NULL);
            elapsed = (ts_now.tv_sec - ts_start.tv_sec) +
                      (ts_now.tv_usec - ts_start.tv_usec) / 1e6;
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