/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   icmp.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 19:29:03 by ktakamat          #+#    #+#             */
/*   Updated: 2025/12/19 15:43:58 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/ip_icmp.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

// 生データからパケットを組み立ててネットワークに放ち、返ってきたパケットが「本当に自分宛てのものか」を検証する

// 受信バッファのサイズ計算
// なぜ「IPヘッダとICMPヘッダのサイズを2倍」にしているのか？
// それは、ICMPエラーバケットを受信したときの構造に備えるためで、エラーパケットのデータ部(ペイロード)には、
// 「エラーの原因となった元のパケット(あなたが送ったIPヘッダー+ICMPヘッダー)」が丸ごと格納されて返ってくる。
// なので、２倍以上のバッファサイズが必要となる
#define RECV_PACK_SIZE ((IP_HDR_SIZE + ICMP_HD_SIZE) * 2 + ICMP_BODY_SIZE + 1)

// パケットの破損チェック
// インターネット通信における標準的なエラー検出アルゴリズム
// パケット全体を16ビット(2バイト)ずつ足し合わせ、桁あふれが発生したらそれ下位ビットに足し戻し、最後にビット反転(-sum)
// これらにより、通信途中でデータが１ビットでも反転してしまったら、受信側でチェックサムが合わなくなりパケットが破棄される
static unsigned short checksum(unsigned short *ptr, int nbytes) {
	unsigned long sum; 
	unsigned short oddbyte;

	sum = 0;
	while (nbytes > 1) {
		sum += *ptr++;
		nbytes -= 2;
	}
	if (nbytes == 1) {
		oddbyte = 0;
		*((unsigned char *)&oddbyte) = *(unsigned char *)ptr;
		sum += oddbyte;
	}
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return (unsigned short) ~sum;
}

// fill_icmp_echo_packet と icmp_send_ping送信処理
// 賢いポイント:gettimeofdayで取得した「パケットを作った現在時刻」を、なんとパケット全体のデーター部(body)の中に直接埋め込む
// こうすることで、パケットが返ってきたときに、データ部からこの時刻を取り出して「今の時刻」と引き算するだけで、簡単にRTT(往復時間)
// を計算することができる。これがpingのRTT計測の肝です。いちいち送信履歴をメモリに保存しておく必要がない。
// 最後にsendtoを使って、組み立てたパケットを宛先IPに向けて発射する
static int fill_icmp_echo_packet(unit8_t *buf, int packet_len) {
	static int seq = 0;
	struct icmphdr *hdr = (struct icmphdr *)buf;
	struct timeval *timestamp = skip_icmphdr(buf);

	if (gettimeofday(timestamp, NULL) == -1) {
		printf("gettimeofday error: %s\n", strerror(errno));
		return -1;
	}
	hdr->type = ICMP_ECHO;
	hdr->un.echo.id = getpid();
	hdr->un.echo.sequence = seq++;
	hdr->checksum = checksum((unsigned short *)buf, packet_len);
	return 0;
}

int icmp_send_ping(int sock_fd, const struct sockinfo *si, struct packinfo *pi) {
	ssize_t nb_bytes;
	uint8_t buf[sizeof(struct icmphdr) + ICMP_BODY_SIZE] = {};

	if (fill_icmp_echo_packet(buf, sizeof(buf)) == -1)
		return -1;
	nb_bytes = sendto(sock_fd, buf, sizeof(buf), 0, (const struct sockaddr *)&si->remote_addr,
			 sizeof(si->remote_addr));
	if (nb_bytes == -1)
		goto err;
	pi->nb_send++;
	return 0;
err:
	if (errno == EACCES) {
		printf("ft_ping: socket access error. Are you trying "
		       "to ping broadcast ?\n");
	} else {
		printf("sendto err: %s\n", strerror(errno));
	}
	return -1;
}

// 自分宛のパケットか検証
// Raw Socketの特性上、あなたのプログラムはOSに届いた全てのICMPパケットを受信している。
// 他のターミナルでばてに人が実行しているpingの返信も混ざってくる。
// 送信時にセットしたgetpid()と照らし合わせることで、確実に自分が送ったパケットの返信だと断定する
static _Bool is_addressed_to_us(unit8_t *buf) {
	struct icmphdr *hdr_sent;
	struct icmphdr *hdr_rep = (struct icmphdr *)buf;
	// 1.ローカルホスト(127.0.0.1)にpingしたとき、自分が送ったRequestを無視する
	if (hdr_rep->type == ICMP_ECHO)
		return 0;
	// 2.エラーパケットの場合、データ部の奥底にある自分が送った元のヘッダまでポインタを進める
	if (hdr_rep->type != ICMP_ECHOREPLY)
		buf += IP_HDR_SIZE + IP_HDR_SIZE;
	hdr_sent = (struct icmphdr *)buf;
	return hdr_sent->un.echo.id == getpid();
}

// 受信処理
// ノンブロッキング受信:MSG_DONTWAITフラグを使っているのがポイント
// パケットが届いていなくてもプログラムを停止させず、すぐにエアー(EAGINまたはEWOULDBLOCK)として処理を返す
// これにより、main.cのループの中でパケットを持つ間も、次に1秒のアラーム(SIGALRM)にすぐ反応できるようになる
// 正常な返信(ICMP_ECHOREPLY)であれば、pi->nb_okを増やし、rtts_save_newでRTTリストに時間を記録してから画面に出力する
int icmp_recv_ping(int sock_fd, struct packinfo *pi, const struct options *opts) {
	unit8_t buf[RECV_PACK_SIZE] = {};
	ssize_t nb_bytes;
	struct icmphdr *icmph;
	struct iovec iov[1] = {
		[0] = {
			.iov_base = buf,
			.iov_len = sizeof(buf)
		}
	};
	struct msghdr msg = {
		.msg_iov = iov,
		.msg_iovlen = 1
	};
	nb_bytes = recvmsg(sock_fd, &msg, MSG_DONTWAIT);
	if (errno != EAGAIN && errno != EWOULDBLOCK && nb_bytes == -1) {
		printf("recvmsg error: %s\n", strerror(errno));
		return -1;
	} else if (nb_bytes == -1) {
		return 0;
	}
	icmph = skip_iphdr(buf);
	if (!is_addressed_to_us((unit8_t *)icmph))
		return 0;
	if (icmph->type == ICMP_ECHOREPLY) {
		pi->nb_ok++;
		if (rtt_save_new(pi, icmph) == NULL)
			return -1;
	}
	if (print_recv_info(buf, nb_bytes, opts, pi) == -1)
		return -1;
	return 1;
}

