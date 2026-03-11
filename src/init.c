/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 19:29:09 by ktakamat          #+#    #+#             */
/*   Updated: 2025/12/19 14:55:34 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

// 住所録(DNS)の検索g
// hintsの設定IPv4(AF_INET)で、ICMPプロトコルを使いたいという条件を指定して、検索範囲を絞り込む
// getaddrinfo：OSに「google.comの住所を教えて！」という条件を指定して、検索範囲を絞り込む
// si->remote_addrへのコピー:見つかった住所(IPアドレス)を、自分たちの管理用構造体siに大切に保存する
// その後、使い終わったtmpをfreeaddrinfoで解放するの忘れない
// inet_ntop:(Network to Presentation):バイナリ形式の住所を、再び人間が読める"142.250..."のような文字列形式に変換して、si->str_sin_addrに
// 格納する。これは、起動時にPING google.com(142.250.196.110)と表示するために使われる
static int init_sock_addr(struct sockinfo *si)
{
	struct addrinfo hints = {
		.ai_family = AF_INET, //IPv4の住所だけ教えて
		.ai_socktype = SOCK_RAW, //Raw Socketを使いたい
		.ai_protocol = IPPROTO_ICMP //ICMPプロトコルを使いたい
	};
	struct addrinfo *tmp;

	if (getaddrinfo(si->host, NULL, &hints, &tmp) != 0) {
		printf("ft_ping: unknown host\n");
		return -1;
	}
	si->remote_addr = *(struct sockaddr_in *)tmp->ai_addr;
	freeaddrinfo(tmp);

    //項目内容 (例)役割si->host"google.com"ユーザーが入力した名前si->remote_addr(バイナリデータ)実際にパケットを送る時に使うデータsi->str_sin_addr"142.250.196.110"画面に表示するための文字列
	if (inet_ntop(AF_INET, &si->remote_addr.sin_addr, si->str_sin_addr,
	    INET_ADDRSTRLEN) == NULL) {
		printf("inet_ntop: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

// 通信機(ソケット)の作成
// 住所がわかったら、次は通信機本体を用意します
// socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)で、Raw Socket(生ソケット)を作成する
// これは、通常のwebブラウザなどが使う「OSにおまかせの発言」ではなく、パケットのヘッダーまで
// 自分で作りたいときに使う特別なソケットの種類
// setsockoptとTTL:ここでパケットの「寿命(TTL:Time To Live)」を設定する
static int create_socket(uint8_t ttl)
{
	int sock_fd;

	if ((sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
		printf("socket: %s\n", strerror(errno));
		return -1;
	}
	if (setsockopt(sock_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) == -1) {
		printf("setsockopt: %s\n", strerror(errno));
		close(sock_fd);
		return -1;
	}
	return sock_fd;
}

// init_sock:全体の統括
// 最後にこれらを順番に呼び出す
// 1.ホスト名をsiにセット 2.住所を調べる(init_sock_addr) 3.通信機を作る(create_socket)
// もし住所が見つからなかったり、ソケット作成（権限不足など）に失敗したりした場合は、すぐに-1を返してmain側に異常を伝える
int init_sock(int *sock_fd, struct sockinfo *si, char *host, int ttl)
{
	si->host = host;
	if (init_sock_addr(si) == -1)
		return -1;
	if ((*sock_fd = create_socket(ttl)) == -1)
		return -1;
	return 0;
}


