/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   packets.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/08 18:48:03 by ktakamat          #+#    #+#             */
/*   Updated: 2025/12/19 15:15:19 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

// シーケンス番号管理用の静的変数
static uint16_t g_seq = 0;
/**
チェックサム計算 (RFC 1071)
パケットの整合性を保証するための計算
*/
static unsigned short get_checksum(const void *data, int data_sz)
{
	const unsigned short *buf = data;
	unsigned int sum = 0;
	while (data_sz > 1)
	{
		sum += *buf++;
		data_sz -= 2;
	}
	if (data_sz == 1)
		sum += *((uint8_t *)buf);
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	return (unsigned short)~sum;
}
/**
受信処理
Rawソケットからデータを読み込み、自分宛てのICMPパケットか確認する
*/
void recv_packet(const int fd, const struct sockaddr_in *dst_ip, t_pkt *resp)
{
	uint8_t *rbuf;
	ssize_t bytes;
	socklen_t addr_sz = sizeof(*dst_ip);
	struct ip *ip_hdr;
	struct icmphdr *icmp_hdr;
	int hdr_sz;
	// 受信バッファの確保
	if (!(rbuf = calloc(1, 1024)))
		logger(LOG_CALLOC_FAIL, ERROR, true, 1);
	// ソケットからデータを受信
	bytes = recvfrom(fd, rbuf, 1024, 0, (struct sockaddr *)dst_ip, &addr_sz);
	// 受信失敗またはタイムアウト
	if (bytes <= 0)
	{
		free(rbuf);
		return;
	}
	// IPヘッダの解析
	ip_hdr = (struct ip *)rbuf;
	hdr_sz = ip_hdr->ip_hl * 4;
	// サイズチェック: 最低限 IPヘッダ + ICMPヘッダ の長さがあるか
	if (bytes < hdr_sz + (ssize_t)sizeof(struct icmphdr))
	{
		free(rbuf);
		return;
	}
	// ICMPヘッダの位置を特定
	icmp_hdr = (struct icmphdr *)(rbuf + hdr_sz);
	// 【重要】フィルタリング処理
	// Rawソケットはシステム上の全ICMPを受け取るため、IDで識別する
	if (icmp_hdr->type == ICMP_ECHOREPLY)
	{
		// IDが自分のプロセスIDと一致しない場合は、他人のパケットなので無視
		if (icmp_hdr->un.echo.id != htons(getpid() & 0xFFFF))
		{
			free(rbuf);
			return;
		}
	}
	// エラーパケット(Time Exceeded等)の場合は、元のパケット情報を見る必要があるが
	// 必須要件の範囲ではこの簡易チェックで動作する
	// 構造体にポインタをセット
	resp->ip_header = ip_hdr;
	// キャストしてICMP部分をセット
	resp->icmp_pkt = (t_icmp_pkt *)(rbuf + hdr_sz);
	// 受信成功数をカウントアップ
	g.s.pkts_rx++;
}
/**
送信処理
作成したパケットをシリアライズ(連結)して送信する
*/
void send_packet(const int fd, const struct sockaddr_in *dst_ip, t_pkt *req)
{
	size_t packet_size = sizeof(struct ip) + sizeof(t_icmp_pkt);
	char buffer[sizeof(struct ip) + sizeof(t_icmp_pkt)]; // 送信用の一時バッファ
	// IPヘッダとICMPパケットを一つのバッファに詰める
	memcpy(buffer, req->ip_header, sizeof(struct ip));
	memcpy(buffer + sizeof(struct ip), req->icmp_pkt, sizeof(t_icmp_pkt));
	// 送信実行
	ssize_t sent = sendto(
		fd,
		buffer,
		packet_size,
		0,
		(struct sockaddr *)dst_ip,
		sizeof(*dst_ip));
	if (sent <= 0)
	{
		// 送信失敗時はログを出すが、プログラムは止めない
		// logger("ping: sendto: Network is unreachable\n", WARNING, false, 0);
	}
	else
	{
		g.s.pkts_tx++;
	}
}
/**
ICMPヘッダとペイロードの作成
*/
static void set_icmp(const t_pkt *req, const bool init)
{
	static char data[PING_PAYLOAD_SIZE - sizeof(struct timeval)];
	struct timeval ts;
	// ダミーデータの初期化 (初回のみ)
	if (init)
	{
		for (size_t i = 0; i < sizeof(data); i++)
			data[i] = (unsigned char)i;
	}
	// ヘッダ設定
	req->icmp_pkt->header.type = ICMP_ECHO; // Type 8 (Request)
	req->icmp_pkt->header.code = 0;
	req->icmp_pkt->header.un.echo.id = htons(getpid() & 0xFFFF);
	req->icmp_pkt->header.un.echo.sequence = htons(++g_seq);
	// ペイロード設定
	gettimeofday(&ts, NULL);
	memset(req->icmp_pkt->payload, 0, sizeof(req->icmp_pkt->payload));
	// 1. 先頭に現在時刻を入れる (RTT計測用)
	memcpy(req->icmp_pkt->payload, &ts, sizeof(ts));
	// 2. 残りをダミーデータで埋める
	if (sizeof(req->icmp_pkt->payload) > sizeof(ts))
	{
		memcpy(req->icmp_pkt->payload + sizeof(ts), data,
			   sizeof(req->icmp_pkt->payload) - sizeof(ts));
	}
	// チェックサム計算 (ヘッダ+ペイロード)
	req->icmp_pkt->header.checksum = 0;
	req->icmp_pkt->header.checksum = get_checksum(
		req->icmp_pkt,
		sizeof(t_icmp_pkt));
}
/**
IPヘッダの作成
*/
static void set_ip_hdr(t_pkt *req, const struct in_addr dst_ip, const t_opt opt)
{
	req->ip_header->ip_hl = 5; // ヘッダ長 (5 * 4 = 20 bytes)
	req->ip_header->ip_v = 4;  // IPv4
	req->ip_header->ip_tos = opt.tos.set ? opt.tos.val : 0;
	req->ip_header->ip_len = htons(sizeof(struct ip) + sizeof(t_icmp_pkt));
	req->ip_header->ip_id = htons(getpid() & 0xFFFF);
	req->ip_header->ip_off = htons(IP_DF);					 // フラグメント禁止
	req->ip_header->ip_ttl = opt.ttl.set ? opt.ttl.val : 64; // TTL
	req->ip_header->ip_p = IPPROTO_ICMP;					 // 中身はICMP
	req->ip_header->ip_sum = 0;
	req->ip_header->ip_src.s_addr = INADDR_ANY; // 送信元IPはOS任せ
	req->ip_header->ip_dst = dst_ip;			// 宛先IP
}
/**
送信パケット構築のエントリーポイント
*/
void set_req_packet(t_pkt *req, const struct in_addr dst_ip,
					const bool init, const t_opt opt)
{
	// メモリ確保 (ping.c の clean_pkt で解放される)
	if (!(req->ip_header = calloc(1, sizeof(struct ip))))
		logger(LOG_CALLOC_FAIL, ERROR, true, 1);
	if (!(req->icmp_pkt = calloc(1, sizeof(t_icmp_pkt))))
		logger(LOG_CALLOC_FAIL, ERROR, true, 1);
	set_ip_hdr(req, dst_ip, opt);
	set_icmp(req, init);
}