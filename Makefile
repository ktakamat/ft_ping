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

NAME = ft_ping
SRCS = main.c\
        check.c\
        icmp.c\
        init.c\
        print.c\
        rtts.c\

HDRS = ft_ping.h

PATH_SRCS = src/

OBJS =	$(SRCS:.c=.o)

CC = gcc

FLAGS =	-Wall -Werror -Wextra

all : $(NAME)

$(NAME) : $(addprefix $(PATH_SRCS), $(OBJS))
			$(CC) -o $(NAME) $(addprefix $(PATH_SRCS), $(OBJS)) $(FLAGS) -lm
			@echo "ft_ping is ready";

clean : rm -rf $(addprefix $(PATH_SRCS), $(OBJS))

fclean : clean
			rm -rf $(NAME)

re : fclean all

.PHONY : all clean fclean re

%.o : %.c $(addprefix $(PATH_SRCS), $(HDRS))
				$(CC) $(FLAGS) -o $@ -c $<