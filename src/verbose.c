/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   verbose.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/12 14:26:55 by ktakamat          #+#    #+#             */
/*   Updated: 2025/12/19 15:32:01 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

// IPヘッダのHexダンプ (0-19バイト目を表示)
static char *get_ip_hdr_hex(const t_pkt *resp) {
    char hex_dump[512] = {0}; // バッファサイズを余裕持って確保
    uint8_t *raw_hdr;
    size_t pos;
    int ip_hl;

    if (!resp->ip_header) return NULL;

    raw_hdr = (uint8_t *)resp->ip_header;
    ip_hl = resp->ip_header->ip_hl * 4; // ヘッダ長 (バイト)

    pos = 0;
    pos += snprintf(hex_dump + pos, sizeof(hex_dump) - pos, "IP Hdr Dump:\n ");
    
    // IPヘッダの先頭からヘッダ長分だけダンプする
    for (int i = 0; i < ip_hl; i += 2) {
        // バッファ溢れ防止
        if (pos >= sizeof(hex_dump) - 6) break;
        
        pos += snprintf(hex_dump + pos, sizeof(hex_dump) - pos, "%02x%02x ", 
            raw_hdr[i], raw_hdr[i + 1]);
    }
    pos += snprintf(hex_dump + pos, sizeof(hex_dump) - pos, "\n");
    
    return ft_strf("%s", hex_dump);
}

// IPヘッダの詳細表示 (構造体メンバを使用)
static char *get_ip_hdr(const t_pkt *resp) {
    struct ip *ip = resp->ip_header;
    char src_str[INET_ADDRSTRLEN];
    char dst_str[INET_ADDRSTRLEN];

    // IPアドレスを文字列化
    inet_ntop(AF_INET, &ip->ip_src, src_str, sizeof(src_str));
    inet_ntop(AF_INET, &ip->ip_dst, dst_str, sizeof(dst_str));

    // LOG_IP_HDR フォーマットに合わせて値をセット
    return ft_strf(
        LOG_IP_HDR,
        ip->ip_v,                       // Version
        ip->ip_hl,                      // Header Length
        ip->ip_tos,                     // TOS
        ntohs(ip->ip_len),              // Total Length
        ntohs(ip->ip_id),               // ID
        (ntohs(ip->ip_off) & 0xE000) >> 13, // Flags (3bit)
        ntohs(ip->ip_off) & 0x1FFF,     // Fragment Offset (13bit)
        ip->ip_ttl,                     // TTL
        ip->ip_p,                       // Protocol
        ntohs(ip->ip_sum),              // Checksum
        src_str,                        // Source IP
        dst_str                         // Dest IP
    );
}

// ICMPヘッダの詳細表示
static char *get_icmp_hdr(const t_pkt *resp) {
    int pl_sz;
    
    // ペイロードサイズ計算
    pl_sz = ntohs(resp->ip_header->ip_len) - (resp->ip_header->ip_hl * 4);
    
    return ft_strf(LOG_ICMP_HDR, 
        resp->icmp_pkt->header.type,
        resp->icmp_pkt->header.code, 
        pl_sz, 
        ntohs(resp->icmp_pkt->header.un.echo.id), 
        ntohs(resp->icmp_pkt->header.un.echo.sequence)
    );
}

// 詳細情報の本文作成 (メイン呼び出し)
char *get_verbose_body(const t_pkt *resp, const t_pkt *req) {
    char *ip_hdr_hex = NULL;
    char *ip_hdr_str = NULL;
    char *icmp_hdr_str = NULL;
    char *logs = NULL;

    (void)req; // 今回の実装ではresp（受信パケット）の内容を表示します

    if (!resp || !resp->ip_header || !resp->icmp_pkt)
        return NULL;

    ip_hdr_hex = get_ip_hdr_hex(resp);
    ip_hdr_str = get_ip_hdr(resp);
    icmp_hdr_str = get_icmp_hdr(resp);

    if (ip_hdr_hex && ip_hdr_str && icmp_hdr_str) {
        logs = ft_strf("%s%s%s", ip_hdr_hex, ip_hdr_str, icmp_hdr_str);
    }

    // 作成に使った一時文字列を解放
    if (ip_hdr_hex) free(ip_hdr_hex);
    if (ip_hdr_str) free(ip_hdr_str);
    if (icmp_hdr_str) free(icmp_hdr_str);

    return logs;
}

// 送信時の詳細ヘッダ表示
void get_verbose_hdr(const struct in_addr *dst_ip, const t_pkt *req) {
    char *logs;
    uint16_t pid;
    size_t pl_sz;
    char ip_buf[INET_ADDRSTRLEN];

    if (!req || !req->icmp_pkt) {
        logger("get_verbose_hdr(): invalid packet\n", ERROR, true, 1);
        return;
    }

    // 修正: &dst_ip ではなく dst_ip を渡す
    if (!inet_ntop(AF_INET, dst_ip, ip_buf, sizeof(ip_buf)))
        logger("get_verbose_hdr(): inet_ntop failed\n", ERROR, true, 1);

    pid = ntohs(req->icmp_pkt->header.un.echo.id);
    pl_sz = PING_PAYLOAD_SIZE; // 定数を使用

    // LOG_PING_HDR_VERB を使用
    if (!(logs = ft_strf(LOG_PING_HDR_VERB, g.s.host, ip_buf, pl_sz, pid, pid)))
        logger("get_verbose_hdr(): ft_strf allocation failed\n", ERROR, true, 1);
    
    logger(logs, INFO, false, 0);
    free(logs);
}