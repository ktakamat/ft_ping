/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rtts.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/12 14:26:55 by ktakamat          #+#    #+#             */
/*   Updated: 2025/12/19 15:32:01 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

// 時間の引き算
// icmp.cでパケットのデータ部分に埋め込んだタイムスタン部を覚えている？
// 送信時:パケットの中に「送った時刻」を取得する
// 受信時:この関数が呼ばれ、gettimeofdayで「今届いた時刻」を取得する
// timersu:便利なマクロを使って、届いた時刻ー送った時刻を計算し、その結果(RTT)をnew_rtt->valに保存する
static int calc_packet_rtt(struct icmphdr *icmph, struct rtt_node *new_rtt)
{
	struct timeval *t_send;
	struct timeval t_recv;

	t_send = ((struct timeval *)skip_icmphdr(icmph));
	if (gettimeofday(&t_recv, NULL) == -1) {
		printf("gettimeofday err: %s\n", strerror(errno));
		return -1;
	}
	timersub(&t_recv, t_send, &new_rtt->val);
	return 0;
}

// 連結リストへの保存
// 計算したRTTをメモリに保存する
// ・malloc:新しいノード(rtt_node)を作成する
// ・末尾に追加:リストの最後までポンタを進めて、新しいノードを繋げる
// ・これにより、100回pingを打てば100個のRTTデータが数珠繋ぎに保存される
struct rtt_node * rtts_save_new(struct packinfo *pi, struct icmphdr *icmph)
{
	struct rtt_node *elem = pi->rtt_list;
	struct rtt_node *new_rtt = NULL;

	if ((new_rtt = malloc(sizeof(*new_rtt))) == NULL)
		return NULL;
	if (calc_packet_rtt(icmph, new_rtt) == -1)
		return NULL;
	new_rtt->next = NULL;
	if (elem != NULL) {
		while (elem->next)
			elem = elem->next;
		elem->next = new_rtt;
	} else {
		pi->rtt_list = new_rtt;
	}
	pi->rtt_last = new_rtt;
	return new_rtt;
}

void rtts_clean(struct packinfo *pi)
{
	struct rtt_node *elem = pi->rtt_list;
	struct rtt_node *tmp;

	while (elem) {
		tmp = elem;
		elem = elem->next;
		free(tmp);
	}
}

// 標準偏差/mdev
// pingの統計で最後にでてくるstddev（またはmdev)は、「パケットごとの時間のバラツキ」を表す。
// ・各パケットのRTTと平均値の差を二乗し、その平均の平方根(sqrt)を取る
// ・これにより、ネットワークが安定している(バラツキが少ない)か、不安定(ラグが激しい)かを数値化できる
void calc_stddev(struct packinfo *pi, long nb_elem)
{
	struct rtt_node *elem = pi->rtt_list;
	struct timeval *avg = &pi->avg;
	long sec_dev = 0;
	long usec_dev = 0;
	long total_sec_dev = 0;
	long total_usec_dev = 0;

	while (elem) {
		sec_dev = elem->val.tv_sec - avg->tv_sec;
		sec_dev *= sec_dev;
		total_sec_dev += sec_dev;
		usec_dev = elem->val.tv_usec - avg->tv_usec;
		usec_dev *= usec_dev;
		total_usec_dev += usec_dev;
		elem = elem->next;
	}
	if (nb_elem - 1 > 0) {
		total_sec_dev /= nb_elem - 1;
		total_usec_dev /= nb_elem - 1;
		pi->stddev.tv_sec = (long)sqrt(total_sec_dev);
		pi->stddev.tv_usec = (long)sqrt(total_usec_dev);
	} else {
		pi->stddev.tv_sec = 0;
		pi->stddev.tv_usec = 0;
	}
}

// 統計の計算
// プログラム終了直前に呼ばれ、蓄積した全データから統計を割り出す
// min/max:リストを走査して、最も小さい時間と大きい時間を特定する
// avg:全て秒(tv_sec)とマイクロ秒(tv_usec)を足し合わせ、最後に要素数で割る。
void rtts_calc_stats(struct packinfo *pi)
{
	struct rtt_node *elem = pi->rtt_list;
	long nb_elem = 0;
	long total_sec = 0;
	long total_usec = 0;

	pi->min = &elem->val;
	pi->max = &elem->val;
	while (elem) {
		if (timercmp(pi->min, &elem->val, >))
			pi->min = &elem->val;
		else if (timercmp(pi->max, &elem->val, <))
			pi->max = &elem->val;

		total_sec += elem->val.tv_sec;
		total_usec += elem->val.tv_usec;
		if (total_usec > 100000) {
			total_usec -= 100000;
			++total_sec;
		}
		++nb_elem;
		elem = elem->next;
	}
	pi->avg.tv_sec = total_sec / nb_elem;
	pi->avg.tv_usec = total_usec / nb_elem;
	calc_stddev(pi, nb_elem);
}

