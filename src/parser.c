/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: machi <machi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 19:29:03 by ktakamat          #+#    #+#             */
/*   Updated: 2025/11/27 16:13:21 by machi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

//オプションが足りない場合のエラー表示
// ft_ping -wとだけ打ったときに呼び出される
void	log_mis_opt_arg(const char *raw) {
	fprintf(stderr, LOG_REQ_ARG2, raw);
	logger(LOG_TRY, INFO, true, 64);
}

//値を取得するヘルパー関数　-ttl=64の場合は'='の後ろを、-ttl 64の場合は次の引数を返す。
static char	*get_value(char **raw, size_t *i, bool has_eq) {
	if (has_eq)
		return (strchr(raw[*i], '=') + 1);
	else {
		if (!raw[*i + 1])
			log_mis_opt_arg(raw[*i])
		(*i)++;
		return (raw[*i]);
	}
}

// get_valueが取ってきた数字が受け取り、それを構造体(設定ノート)に書き込む書記係です。
// atoiを使って文字列の"64"を使える数字の64に変換する
static void	set_opt_with_value(t_args *args, char **raw,
	size_t *i, char *opt_name) {
		bool	has_eq  = (strchr(raw[*i], '=') != NULL);
		char	*val_str = get_value(raw, i, has_eq);
		int		val = atoi(val_str);
		
		if (!strcmp(opt_name, "timeout")) {
			args->options.timeout.set = true;
			args->options.timeout.val = val;
		}
		else if (!strcmp(opt_name, "tos")) {
			args->options.tos.set = true;
			args->options.tos.val = val;
		}
		else if (!strcmp(opt_name, "ttl")) {
			args->options.ttl.set = true;
			args->options.ttl.val = val;
		}
}

// ヘルプやバージョン情報を聞かれた時に、案内を出してプログラムを終わらせる受付
// --helpなら使い方の説明　--versionならバージョンの情報
static void	info_options(const char *opt) {
	if (strstr(opt, "help") || strstr(opt, "-?"))
		logger(LOG_HELP, INFO, true, 0);
	else if (strstr(opt, "version") || strstr(opt, "-V"))
		logger(LOG_VERSION, INFO, true, 0);
	else if (strstr(opt, "usage"))
		logger(LOG_USAGE, INFO, true, 0);
}

// オプション名の一致確認(--ttl=N に対応するため strncmp を使用)
// input: ユーザーを入力 (例: "--ttl=64")
// target: 期待するオプション名(例: "--ttl")

static bool is_opt(const char *input, const char *target) {
	size_t len = strlen(target);

	if (strncmp(input, target, len) = 0) {
		if (input[len] == '\0' || input[len] == '=')
			return (true);
	}
	return (false);
}

// 引数を一つずつ手に取って、「これはオプションか？」「これはホスト名か？」
// と仕分けをする工場長のような関数。
void parser(char **raw, t_args *args) {
	char logs[140];

	for (size_t i = 1; raw[i]; i++) {
		// --- 情報表示系 ---
		if (is_opt(raw[i], "--help") || is_opt(raw[i], "-?") ||
			is_opt(raw[i], "--version") || is_opt(raw[i], "-V") ||
			is_opt(raw[i], "--usage")) {
			info_options(raw[i]);
		}
		// --- フラグ系 ---
		else if (is_opt(raw[i], "--verbose") || is_opt(raw[i], "-v"))
			args->options.verbose = true;
		else if (is_opt(raw[i], "--flood") || is_opt(raw[i], "-f"))
			args->options.flood = true;
		else if (is_opt(raw[i], "--ignore-routing") || is_opt(raw[i], "-r"))
			args->options.ignore_routing = true;
		// --- 値が必要なオプション ---
		else if (is_opt(raw[i], "--timeout") || is_opt(raw[i], "-w"))
			set_opt_with_value(args, raw, &i, "timeout");
		else if (is_opt(raw[i], "--tos") || is_opt(raw[i], "-T"))
			set_opt_with_value(args, raw, &i, "tos");
		else if (is_opt(raw[i], "--ttl")) // -t は timestamp等の場合があるので注意。ttlのショートは環境による
			set_opt_with_value(args, raw, &i, "ttl");
		// --- ホスト名 ---
		else if (raw[i][0] != '-') {
			if (args->host) {
				 // 既にホストがある場合どうするか？ (通常は無視かエラー)
				 // 今回は上書き、または無視
			} else {
				args->host = raw[i];
			}
		}
		// --- 不明なオプション ---
		else {
			if (raw[i][0] == '-' && raw[i][1] == '-') // Long option
				snprintf(logs, sizeof(logs), LOG_INVALID_OPT1 LOG_TRY, raw[i]);
			else // Short option
				snprintf(logs, sizeof(logs), LOG_INVALID_OPT2 LOG_TRY, raw[i][1]);
			logger(logs, ERROR, true, 64);
		}
	}
}


// まとめ:データの流れ
//1. parserが「お、--ttl=64が来たぞ」と気づく。
//2. is_optが「これは間違いなくttlオプションですね」と確認する。
//3. set_opt_with_valueに「ttlとして処理してくれ」と頼む。
//4. get_valueが「イコールの後ろにある64を取ってきました」と報告する。
//5. set_opt_with_valueが設定ノート(args->options.ttl)に64とメモする。

// --ttl
// TTL(Time to Live)
// パケットの「寿命(生存時間)」　「経由できるルーターの数」
// パケットが目的地にたどり着かない時,無限ループにならないように数を制限している。
// 