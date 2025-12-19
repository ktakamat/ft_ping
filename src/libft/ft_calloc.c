/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_calloc.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 15:01:11 by ktakamat          #+#    #+#             */
/*   Updated: 2025/12/19 15:03:01 by ktakamat         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"
#include <stdlib.h>
#include <string.h>

void *ft_calloc(size_t count, size_t size)
{
	void *ptr;
	size_t total_size;

	total_size = count * size;
	// オーバーフロー検知 (必要であれば)
	if (count != 0 && total_size / count != size)
		return (NULL);

	ptr = malloc(total_size);
	if (!ptr)
		return (NULL);

	memset(ptr, 0, total_size);
	return (ptr);
}