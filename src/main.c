/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 17:10:43 by ktakamat          #+#    #+#             */
/*   Updated: 2025/12/19 15:41:23 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Bool pingloop = 1;
_Bool send_packet = 1;

// グローバル変数とシグナルハンドラ(割り込み処理)
// SIGINT (Ctrl+C)でpingループを終了するためのフラグと、SIGALRMで定期的にパケットを送るためのフラグ
// SIGALRM(アラーム)alarm(1)によって1秒経過したときに、send_packetフラグを1にセットして、次のパケット送信をトリガーする
void handler(int signum) {
    if (signum == SIGINT) 
        pingloop = 0;
    else if (signum == SIGALRM)
        send_packet = 1;
}

int main(int ac, char **av) {
// 構造体を{}でゼロ初期化する
// 関数の実行結果を一時的に保管するための変数
int ret;
// 作成したRaw Socketを操作するための番号(ファイルディスクリプター)
int sock_fd;
// ユーザーが入力した宛先ホスト名を保存するための変数(例:google.com, "8.8.8.8")
char *host = NULL;
// ユーザーが指定したオプション設定をまとめて管理する箱
// opts.quiet (静音モードか？), opts.verb (詳細表示か？), opts.help (ヘルプが見たいか？) といったフラグが入っている
struct options opts = {};
// ネットワーク上の住所に関する詳細情報をまとめたバインダー
// si.host: ホスト名の文字列。si.remote_addr: コンピュータが理解できるバイナリ形式のIPアドレス。si.str_sin_addr: 人間が読める形式のIPアドレス（例："142.250.x.x"）
struct sockinfo si = {};
// 通信の統計を記録するための帳簿
// パケットを何発送ったか (nb_send)何発返ってきたか (nb_ok)返ってきた時間の履歴リスト (rtt_list)
struct packinfo pi = {};

// check_rights(ルート権限確認),　check_args(引数のオプション解析),　init_sock(ソケットの初期化)を順番に呼び出す
    if (check_rigts() == -1)
        return E_EXIT_ERR_ARGS;
    if ((ret = check_args(ac, av, &host, &opts)) != 0)
        return ret == -1 ? E_EXIT_ERR_ARGS : E_EXIT_OK;
    if (init_sock(&sock_fd, &si, host, IP_TTL_VALUE) == -1)
        return E_EXIT_ERR_HOST;
    
// シグナルの登録ctrl+cとアラームの監視を開始する
    signal(SIGINT, &handler);
    signal(SIGALRM, &handler);
//開始テキストPING google.com (142.250...): 56 data bytesのような最初の1行を出力する
    print_start_info(&si, &opts);
// while (pingloop) Ctrl + Cが押されるまでループ
    while (pingloop) {
        //　送信ブロック(if (send_packet)): アラームが鳴ってsend_packetが1になっていたら、パケットを送信
        //　送信後にalarm(1)をセットし、1秒後またSIGALRMが鳴るようにする
        if (send_packet) {
            send_packet = 0;
            if (icmp_send_ping(sock_fd, &si, &pi) == -1)
                goto fatal_close_sock;
            alarm(1);
        }
        // 受信ブロック(icmp_recv_ping):　相手からの返信をまつ(ここでプログラムは一時停止=ブロックする)
        // 1秒経つとSIGALRMが鳴り、この一時停止が強制キャンセルされ、ループの先頭に戻って次のパケットを送る
        if (icmp_recv_ping(sock_fd, &pi, &opts) == -1)
            goto fatal_close_sock;
    }
    //　正常終了:pingloopが0になってループを抜けたら、---google.com ping statistics---といった
    //　統計情報(print_end_info)を表示する
    print_end_info(&pi);
    // メモリ解放とソケットを閉じ:開きっぱなしのソケットをcloseし、rtts_cleanで計測したRTTの連結リストの
    // メモリ(mallocで確保したもの)をすべてfreeする
    close(sock_fd);
    rtts_clean(&pi);
    //　終了コード:pi.nb_ok > 0 (1つでも返信が返ってきたか)でE_EXIT_OK(0)かE_EXIT_ERR_HOST(1)を決定する
    return pi.nb_ok > 0 ? E_EXIT_OK : E_EXIT_ERR_HOST;
// goto fatal_close_sock:もし送受信中に致命的なシステムエラー（ソケットが壊れた等)が起きた場合は、強制的に
// ここへジャンプしてリソースを解放し終了する
fatal_close_sock:
    close(sock_fd);
    rtts_clean(&pi);
    return E_EXIT_ERR_HOST;
}


