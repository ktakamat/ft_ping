/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   logger.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/08 18:47:54 by ktakamat          #+#    #+#             */
/*   Updated: 2025/12/12 14:24:38 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

// 統計情報の表示
void log_stats(void) {
    char *logs = NULL;
    int loss_pct = 0;

    // パケットロス率の計算 (ゼロ除算防止)
    if (g.s.pkts_tx > 0)
        loss_pct = ((g.s.pkts_tx - g.s.pkts_rx) * 100) / g.s.pkts_tx;

    // パケットロスが100%でない場合はRTT情報を付加
    if (g.s.pkts_rx > 0)
    {
        // 標準偏差の計算 (分散 = 二乗平均 - 平均の二乗)
        double avg = g.s.rtt_sum / g.s.pkts_rx;
        double vari = (g.s.rtt_sq_sum / g.s.pkts_rx) - (avg * avg);
        // 浮動小数点の誤差でマイナスになるのを防ぐ
        if (vari < 0) vari = 0;
        double stddev = sqrt(vari);

        // 平均値などもここで計算して更新
        g.s.rtt_avg = avg;
        g.s.rtt_stddev = stddev;

        logs = ft_strf(LOG_STAT LOG_STAT2, 
            g.s.host, g.s.pkts_tx, g.s.pkts_rx, loss_pct,
            g.s.rtt_min, g.s.rtt_avg, g.s.rtt_max, g.s.rtt_stddev);
    }
    else
    {
        logs = ft_strf(LOG_STAT, 
            g.s.host, g.s.pkts_tx, g.s.pkts_rx, loss_pct);
    }

    if (!logs)
        logger("log_stats(): internal_error\n", ERROR, true, EXIT_FAILURE);
    
    logger(logs, INFO, true, EXIT_SUCCESS);
    // 注意: exitするのでここでのfreeは不要だが、行儀良くするなら free(logs);
}

// 受信パケットの内容を文字列にする
static char *get_body(const t_pkt *resp, const struct in_addr dst_ip, 
                      const double rtt, const t_opt opt) 
{
    int pl_sz;
    const char *msg;
    char *ip_str;

    // ペイロードサイズの計算 (Total Len - IP Header Len)
    pl_sz = ntohs(resp->ip_header->ip_len) - (resp->ip_header->ip_hl * 4);
    if (pl_sz < 0) pl_sz = 0;

    // IPアドレスを文字列化
    ip_str = inet_ntoa(dst_ip);

    // ICMPタイプに応じたメッセージ作成
    if (resp->icmp_pkt->header.type == ICMP_DEST_UNREACH) {
        switch (resp->icmp_pkt->header.code) {
            case ICMP_NET_UNREACH: msg = "Destination Net Unreachable"; break;
            case ICMP_HOST_UNREACH: msg = "Destination Host Unreachable"; break;
            default: msg = "Destination Unreachable"; break;
        }
        return ft_strf(LOG_BYTES1, pl_sz, ip_str, msg);
    } 
    else if (resp->icmp_pkt->header.type == ICMP_TIME_EXCEEDED) {
        return ft_strf(LOG_BYTES2, pl_sz, ip_str);
    }
    else if (resp->icmp_pkt->header.type == ICMP_ECHOREPLY) {
        if (!opt.flood) {
            return ft_strf(LOG_BYTES3, pl_sz, ip_str, 
                ntohs(resp->icmp_pkt->header.un.echo.sequence), 
                resp->ip_header->ip_ttl, rtt);
        }
    }
    
    // Floodモードや、ログ不要な場合はNULLを返す
    return (NULL);
}

// Ping本体のログ出力
void log_ping_body(const t_pkt *req, const t_pkt *resp, 
                   const struct in_addr dst_ip, const double rtt, const t_opt opt) 
{
    char *basic = NULL;
    char *verbose = NULL;
    char *logs = NULL;

    if (!resp || !resp->icmp_pkt || !resp->ip_header)
        return;

    basic = get_body(resp, dst_ip, rtt, opt);

    // verboseオプションがあり、かつEchoReply以外の場合は詳細を表示
    if (opt.verbose && resp->icmp_pkt->header.type != ICMP_ECHOREPLY)
        verbose = get_verbose_body(resp, req); // verbose.c に実装が必要

    if (basic || verbose) {
        // 文字列結合
        logs = ft_strf("%s%s", basic ? basic : "", verbose ? verbose : "");
        if (logs) {
            logger(logs, INFO, false, 0);
            free(logs);
        }
    }
    
    if (basic) free(basic);
    if (verbose) free(verbose);
}

// Ping開始時のヘッダー表示
void log_ping_hdr(const struct in_addr *dst_ip, const t_pkt *req) {
    char ip_buf[INET_ADDRSTRLEN];
    char *logs;
    size_t pl_sz;

    if (!req) return; // 安全策

    if (!inet_ntop(AF_INET, dst_ip, ip_buf, sizeof(ip_buf)))
        logger("log_ping_hdr(): inet_ntop failed\n", ERROR, true, 1);
    
    // ICMPペイロードサイズ (パケット全体 - ヘッダ8バイト)
    pl_sz = PING_PAYLOAD_SIZE; 
    
    if (!(logs = ft_strf(LOG_PING_HDR, g.s.host, ip_buf, pl_sz)))
        logger("log_ping_hdr(): internal error\n", ERROR, true, 1);
    
    logger(logs, INFO, false, 0);
    free(logs);
}

// オプションエラー表示
void log_mis_opt_arg(const char *raw) {
    char *logs;

    if (!raw || raw[0] == '\0') {
        // 文字列リテラルは直接渡さず、フォーマットを通して渡すか、
        // loggerに直接渡すがfreeさせないようにする
        logger(LOG_REQ_ARG1, WARNING, true, 64);
        return;
    }

    if (raw[0] == '-' && raw[1] == '-' && raw[2]) {
        logs = ft_strf(LOG_REQ_ARG2 LOG_TRY, raw);
    } else {
        logs = ft_strf(LOG_REQ_ARG3 LOG_TRY, raw[1] ? raw[1] : '?');
    }

    if (!logs)
        logger("internal error\n", ERROR, true, 1);
    
    logger(logs, WARNING, true, 64);
    free(logs);
}

// 汎用ロガー関数 (修正: msgをfreeしないように変更)
void logger(char *msg, const int level, const bool to_exit, const int exit_code) {
    int fd;

    switch (level) {
        case DEBUG:   fd = STDOUT_FILENO; break;
        case INFO:    fd = STDOUT_FILENO; break;
        case WARNING: fd = STDERR_FILENO; break;
        case ERROR:   fd = STDERR_FILENO; break;
        default:      fd = STDERR_FILENO; break;
    }

    if (msg && *msg)
        dprintf(fd, "%s", msg);

    if (to_exit) {
        // ここでの ft_free((void **)&msg); は削除しました。
        // リテラルを渡した時にクラッシュするためです。
        exit(exit_code);
    }
}