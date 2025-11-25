/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 19:29:03 by ktakamat          #+#    #+#             */
/*   Updated: 2025/11/25 19:53:31 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

//オプションが足りない場合のエラー表示
void	log_mis_opt_arg(const char *raw) {
	fprintf(stderr, LOG_REQ_ARG2, raw);
	logger(LOG_TRY, INFO, true, 64);
}

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


