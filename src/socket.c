/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socket.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: machi <machi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 19:29:09 by ktakamat          #+#    #+#             */
/*   Updated: 2025/12/02 16:01:25 by machi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* src/socket.c */
#include "ft_ping.h"

// socket()関数で使ったもの　socket(ドメイン, タイプ, プロトコル)
// AF_INET(Address Family - Internet)
// 意味「IPv4(普通のインターネット)を使います」
// 例え: 「日本の住所システム（郵便番号＋住所）を使います」と言っているようなもの。
// 対比: これ以外に AF_INET6（IPv6用）や AF_UNIX（PC内部通信用）などがありますが、今回は普通のIPアドレスを使うのでこれ一択です。

// SOCK_RAW(Socket Raw)
// 意味「生の(Raw)ソケットを使います」
// ここが重要: 通常の通信では SOCK_STREAM (TCP) や SOCK_DGRAM (UDP) を使います。これらはOSが面倒な処理（再送制御やヘッダ付与）をやってくれます。
// 例え:
// TCP/UDP: 「封筒も切手も用意済みのレターパック」。中身を入れるだけ。
// RAW: 「自分で紙を折って封筒を作り、糊付けし、切手の金額も計算して貼る」完全自作コース。 Pingを作るには、この細かい制御が必要なのでこれを選びます。

// IPPROTO_ICMP(IP Protocol - ICMP)
// 意味「ICMPプロトコル(Pingの信号)だけを扱います」
// 役割: 受信用のソケットで指定しました。これにより、Web閲覧や動画視聴などの他の通信パケットは無視され、Pingに関するパケットだけがこのソケットに届くようになります。

// IPPROTO_RAW(IP Protocol - Raw)
// 意味「プロトコルの中身を自分で決める(あるいは指定しない)」
// 役割: 送信用のソケットで指定しました。「中身は俺が全部書くから、OSは余計な口出しをしないでくれ」という設定です。

// setsockopt()関数で使ったもの setsocket(ソケット, レベル, オプション名, ...)ソケットの詳細設定
// SOL_SOCKET(Socket Level)
// 意味: 「ソケット全般に関わる設定を変更します」 という指定。
// 役割: どのプロトコル（TCP/IPなど）を使っているかに関係なく、ソケットそのものの設定（例：タイムアウト時間など）を変える時に使います。
// 例え: テレビのリモコンで「画面の明るさ」や「音量」を変えるような、本体設定のレベル。

// SO_RCVTIMEO(Socket Option - Recieve Timeout)
// 意味: 「受信のタイムアウト時間」。
// 役割: 「返事が来るまで永遠に待ち続ける」のを防ぐため、「0.1秒待って来なかったら諦める」という時間を設定しました。

// IPPROTO_IP(IP Protocol Level)
// 意味: 「IPプロトコルに関わる設定を変更します」 という指定。
// 役割: この課題の最重要オプションです。
// OFFの場合（通常）: データだけ渡せば、OSが「送信元IP」「宛先IP」「TTL」などを自動でくっつけて送ってくれます。
// ONの場合（今回）: OSは何もしません。データの先頭に自分で作ったIPヘッダがないと、相手に届きません。その代わり、TTLを自由に変えたり、偽の送信元IPを書いたり（！）できるようになります。

// SO_DONTROUTE(Don't Route)
// 意味: 「ルーティングテーブル（道案内）を無視する」。
// 役割: 通常、パケットはルーターという案内役を経由して遠くへ運ばれますが、これをONにすると「ルーターを無視して、直接ケーブルで繋がっている相手に送りつける」という挙動になります。-r オプション用です。

t_sock *get_socket(const t_args *args)
{
	t_sock *socks;
	struct timeval to;
	int on;

	on = 1;

	// タイムアウト設定
	// floodモード(-f)のときは待機時間をゼロにして高速連射する
	// 通常時は 0.1秒 (100000マイクロ秒) 待つ設定になっています
	to.tv_sec = 0;
	to.tv_usec = args->options.flood ? 0 : 100000;

	// 構造体のメモリ確保
	if (!(socks = calloc(1, sizeof(t_sock))))
		logger(LOG_CALLOC_FAIL, ERROR, true, 1);

	// 【送信ソケット】: IPPROTO_RAW (生のIPパケットを送る)
	if ((socks->send_fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1)
		logger("ping: socket failed\n", ERROR, true, 1);

	// 【受信ソケット】: IPPROTO_ICMP (ICMPパケットを受け取る)
	if ((socks->recv_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
		logger("ping: socket failed\n", ERROR, true, 1);

	// 受信タイムアウトの設定
	if (setsockopt(socks->recv_fd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to)) == -1)
		logger("ping: setsockopt SO_RCVTIMEO failed\n", ERROR, true, 1);

	// 【重要】IP_HDRINCL: IPヘッダを自分で作成するオプションを有効化
	if (setsockopt(socks->send_fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) == -1)
		logger("ping: setsockopt IP_HDRINCL failed\n", ERROR, true, 1);

	// -r オプション: ルーティングテーブルを無視して直接ホストに送る
	if (args->options.ignore_routing) {
		if (setsockopt(socks->send_fd, SOL_SOCKET, SO_DONTROUTE, &on, sizeof(on)) == -1)
			logger("ping: setsockopt SO_DONTROUTE failed\n", ERROR, true, 1);
	}

	return socks;
}

// 送信と受信でソケット分ける
// send_fd(送信):IPPROTO_RAWを使用
// recv_fd(受信):IPPROTO_ICMPを使用
// IP_HDRINCL(IPヘッダ手動作成モード)は送信時に有効ですが、受信時はICMPパケット
// だけを効率よくフィルタリングして拾いたいため、受信にはICMP専用プロトコルを指定します。

// IP_HDRINCLオプション
// setsockopt(..., IPPROTO_IP, IP_HDRINCL, ...)
// 意味: 「カーネルよ、IPヘッダを勝手に作るな。俺（ユーザープログラム）が自分で1バイトずつバイナリを書くから」という宣言
// これがないと、OSが自動的にIPヘッダを付与してしまい、ping の詳細な挙動（TTL操作など）を完全制御できません。

// SO_RCVTIMEO(受信タイムアウト)
// recvfrom関数はずっと待機(ブロック)してしまいますが、パケットが帰ってこない場合に無限に
// フリーズするのを防ぐため、タイムアウトを設定する
