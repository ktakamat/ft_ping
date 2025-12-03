#include "ft_ping.h"

// 統計情報の更新(最小、　最大、　平均、標準偏差用)
static void	set_rtt_stats(double rtt) {
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
static void	run(const t_sock *socks, const t_args *args,
				const struct sockaddr_in *dst_ip, bool init) {
	// ポインタではなく実体で確保し、スタックで管理した方が楽
	t_pkt	req;
	t_pkt	resp;
	struct	timeval ts_start, ts_end;
	double	rtt;
	// 構造体の初期化
	memset(&req, 0, sizeof(req));
	memset(&resp, 0, sizeof(resp));
	// パケットの作成
	set_req_packet(&req, dst_ip->sin_addr, init, args->options);

	// ログ表示
	if (init) {
		if (args->options.verbose)
			get_verbose_hdr(&dst_ip->sin_addr, &req);
		else
			long_ping_hdr(&dst_ip->sin_addr, &req);
	}
	// 送信
	gettimeofday(&ts_start, NULL);
	send_packet(socks->send_fd, dst_ip, &req);
	// 受信
	recv_packet(socks->recv_fd, dst_ip, &resp);
	gettimeofday(&ts_end, NULL);

	// RTT計測(ミリ秒)
	rtt = ((ts_end.tv_sec - ts_start.tv_sec) * 1000.0) +
		  ((ts_end.tv_usec - ts_start.tv_usec) / 1000.0);

	// 受信に成功していれば統計更新
	if (g.s.pkts_rx > 0 && resp.ip_header != NULL) {
		set_rtt_stats(rtt);
		long_ping_body(&req, &resp, dst_ip->sin_addr, rtt, args->options);
	}

	// 1秒間隔の調整
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
		fprintf(stderr, "ping: %s: %s\n", args->host,
		gai_strerror(status));
		exit(2);
	}

	// 最初に見つかったアドレスをコピー
	memcpy(dst_ip, res->ai_addr, sizeof(struct sockaddr_in));

	// IPアドレスを文字列として統計情報に保存しておく
	inet_ntop(AF_INET, &dst_ip->sin_addr, g.s)
}

