/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_ping.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 16:31:26 by ktakamat          #+#    #+#             */
/*   Updated: 2025/12/19 15:38:42 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_PING_H
#define FT_PING_H

#include <netinet/ip_icmp.h>

// パケットの寿命64個のルーター
#define IP_TTL_VALUE 64
// プロトコルはコンピュータでデータをやりとりするために定められた手順や規約、
// 信号の電気的規則、通信における送受信の手順などを定めた規格
// ICMPはネットワーク上のデバイス間のデータ通信の問題を送信するためのプロトコル
// ICMPヘッダーのバイトサイズを計算する
// struct iphdrはIPヘッダーの構造体、送信元IPや宛先IPなどの情報が詰まってる通常は20バイト
// struct icmphdrはICMPヘッダーの構造体、PingのType(要求か返信か)やIDが詰まっている通常は8バイト
#define IP_HDR_SIZE (sizeof(struct iphdr)) //20バイト
#define ICMP_HDR_SIZE (sizeof(struct icmphdr)) //8バイト
//　ICMPのパケットに載せるデータ（ペイロード）のサイズ
#define ICMP_BODY_SIZE 56

// プログラムが終了した時にOSに返すステータスコード
enum e_exitcode {
    E_EXIT_OK, //1回でも返信があったら0を返す
    E_EXIT_ERR_HOST, //エラーや応答なしの場合は1を返す
    E_EXIT_ERR_ARGS = 64 //引数エラーの場合は64を返すEX_USAGEと同じ値
};

//　コマンドライン引数の状態を記録するフラグ
struct rtt_node {
    struct timeval val; //１つのパケットの往復時間
    struct rtt_node *next; //次のデータへのポインタ
};

struct options {
    _Bool help; // -h オプションが指定されたか
    _Bool quiet; // -q オプションが指定されたか
    _Bool verb; // -v オプションが指定されたか
};

// pingが成功するたびに、その往復時間(RTT)を記録していく連結リストのノード
struct rtt_node {
    struct timeval val; //1つのパケットの往復時間(RTT)
    struct rtt_node *next; // 次のデータへのポインタ
};

//pingを終了するときに表示される統計情報を管理するための構造体
struct packinfo {
    int nb_send; //送信したパケットの合計
    int nb_ok; //受信に成功したパケットの合計
    struct timeval *min; //全RTTのうち、最小の時間へのポインタ
    struct timeval *mac; //全RTTのうち、最大の時間へのポインタ
    struct timeval avg; //平均のRTT
    struct rtt_node *rtt_list; //記録したRTTリストの先頭
    struct rtt_node *rtt_last; //記録したRTTリストの最後尾
};

//宛先(ターゲット)に関するネットワーク情報をまとめたもの
struct sockinfo {
    char *host; //ユーザーが入力した宛先(例:"google.com")
    struct sockaddr_in remote_addr; //IPアドレスやポート番号を持つバイナリデータ構造体
    char str_sin_addr[INET_ADDRSTRLEN]; //人間が読めるIPアドレス文字列
};

//パケットの解剖に使う便利な関数
// 受信したパケットの生データ(buf)は[IPヘッダー][ICMPヘッダー][ICMPのペイロード]の順で構成されている
//skip_iphdr関数は、受信したパケットの生データからIPヘッダーをスキップして、ICMPヘッダーの先頭の位置を返す
static inline void *skip_iphdr(void *buf) {
    return (void *)((uint8_t *)buf + IP_HDR_SIZE);
}
//skip_icmphdr関数は、受信したパケットの生データからICMPヘッダーをスキップして、データ部分の先頭の位置を返す
static inline void *skip_icmphdr(void *buf) {
    return (void *)((uint8_t *)buf + ICMP_HDR_SIZE);
}
//inlineをつけることで、関数呼び出しのオーバーヘッドがなくなり、高速に処理される

//権限の確認と、引数(オプション)の解析を行う関数群
int check_rights(void);
int check_args(int argc, char **argv, struct options *opts);

//名前解決(DNS)を行い、Rawソケットを作成する関数
//ハードウェア物理的な接続部品や、プログラムとネットワークをつなげる
// 論理的な接続口のインターフェース
int init_sock(int *sock_fd, struct sockinfo *si, char *host, int ttl);

//プロジェクトの心臓部
//パケットを組み立てて送信する
int icmp_send_ping(int sock_fd, const struct sockinfo *si, struct packinfo *pi);
//返ってきたパケットを受信し、エラーがないか確認する
int icmp_recv_ping(int sock_fd, struct packinfo *pi, const struct options *opts);

//画面に文字を出力するための関数群。開始時、パケット受信時、終了時の出力を担当する
void print_help();
void print_start_info(const struct sockinfo *si, const struct options *opts);
void print_recv_info(void *buf, ssize_t nb_bytes, const struct options *opts, const struct packinfo *pi);
void print_end_info(const struct sockinfo *si, const struct packinfo *pi);

//パケットのRTTをリストに保存し、最後に平均や標準偏差を計算(calc_status)、メモリを
//解放(clean)する関数群
struct rtt_node *rtt_save_new(struct packinfo *pi, struct icmphdr *icmphdr);
void rtts_clean(struct packinfo *pi);
void rtts_calc_stats(struct packinfo *pi);





#endif