# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/11/24 16:45:31 by ktakamat          #+#    #+#              #
#    Updated: 2025/12/19 15:35:41 by ktakamat         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME        = ft_ping
CC          = gcc
FLAGS       = -Wall -Werror -Wextra
LIBS        = -lm

PATH_SRCS   = src/
SRCS_FILES  = main.c check.c icmp.c init.c print.c rtts.c
HDRS        = ft_ping.h

SRCS        = $(addprefix $(PATH_SRCS), $(SRCS_FILES))

OBJS        = $(SRCS:.c=.o)

all : $(NAME)

$(NAME) : $(OBJS)
	$(CC) $(FLAGS) -o $(NAME) $(OBJS) $(LIBS)
	@echo "ft_ping is ready"

$(PATH_SRCS)%.o : $(PATH_SRCS)%.c $(PATH_SRCS)$(HDRS)
	$(CC) $(FLAGS) -c $< -o $@

clean :
	rm -rf $(OBJS)

fclean : clean
	rm -rf $(NAME)

re : fclean all

.PHONY : all clean fclean re
