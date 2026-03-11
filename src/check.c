/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   check.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/08 18:47:54 by ktakamat          #+#    #+#             */
/*   Updated: 2025/12/12 14:24:38 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

//　ユーザーがターミナルで入力したコマンド(例: ./ft_ping -vq google.com)を解読し、
// 「実行権限があるか？」「オプションは正しいか？」「宛先（ホスト）はちゃんと一つ指定されているか？」をチェック

//　サポートするオプションの定義
//　このプログラムで許可されているオプション(-h:ヘルプ, -q:静音, -v:詳細)を文字列として定義する
//　staticを付けているため、この変数はこのファイル(check.c)の中だけで有効になります
static const char supported_opts[] = "hqv";

//　ルート権限の確認
//　getuid()プログラムを実行したユーザーのID(UID)を取得する
//　なぜ0なのか？：Unix系OSにおいて、UID０はroot(特権ユーザー)
//　ft_pingでは、OSの標準ネットワークを処理をバイパスしてICMPパケットを自作・送信するRaw Socket(生ソケット)を使用。
//　悪用を防ぐため、Raw Socketの作成には必ず管理者権限(sudo)が必要になる
int check_rigts(void) {
    if (getuid() != 0) {
        printf("ft_ping: usage error: need to be run as root\n");
        return -1;
    }
    return 0;
}

//　オプションの解析
//　引数の分解:例えば-qvという文字列が渡された場合を考える
//　i=1 から始まる理由:最初の文字arg[0]は-(ハイフン)だから。
//　strchr:検査中の文字(例:q)が、"hqv"の中に含まれているかを探す。
//　含まれていればswitch文で構造体(opts)の該当フラグを1(True)にする。
//  含まれていなければ、即座にエラーメッセージを出して、-1を返す。この仕組みにより
//　-vqのような連結されたオプション記法にも完璧に対応できる
static int parse_opt_arg(char *arg, struct options *opts) {
    char *match = NULL;
    size_t len = strlen(arg);

    for (size_t i = 1; i < len; ++i) {
       if ((match = strchr(supported_opts, arg[i])) != NULL) {
            switch (*match) {
                case 'h': opts->help = 1; break;
                case 'q': opts->quiet = 1; break;
                case 'v': opts->verb = 1; break;
                default: printf("ping: unkown option\n");
            }
       } else {
        return -1;
       }
    }
    return 0;
}

//　全体の引数チェック
//　ac(argc)とav(argv)をループで回す(i=1なのは、v[0]が./ft_pingというプログラム名だから)
//　分析：文字列が-で始まり、かつ長さが1より大きい場合は、オプションとして先ほどのparse_opt_argに渡す
//　それ以外の場合は宛先(ホスト名)として*hostに保存し、nb_hostをカウントアップする
//　ヘルプの優先:もしループの途中で-hを見つけたら、他のエラーを無視してすぐにヘルプを表示し、プログラムを正常に
//　終了させる。
int check_args(int ac, char **av, char **host, struct options *opts) {
    int nb_host = 0;
    for (int i = 1; i < ac; ++i) {
        if (av[i][0] == '-' && strlen(av[i] > 1)) {
            if (parse_opt_arg(av[i], opts) == -1)
                return -1;
        } else {
            *host = av[i];
            ++nb_host;
        }
        if (opts->help) {
            print_help();
            return 1;
        }
    }
    // 最後の検証:全ての引数が見終わった後、ホスト名が「無かった(0個)」場合や、「多すぎた(2個以上)」
    //　場合はエラーとする
    //　これにより、*hostには確実に1つの宛先(例:google.com)が入った状態で、次のネットワーク初期化処理(init_sock)へ進める。
    if (!nb_host) {
		printf("ft_ping: missing host operand\n");
		printf("Try 'ft_ping -h' for more information.\n");
		return -1;
	} else if (nb_host > 1) {
		printf("ft_ping: only one host is needed\n");
		printf("Try 'ft_ping -h' for more information.\n");
		return -1;
	}
	return 0;
}