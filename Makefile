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
CFLAGS      = -Wall -Werror -Wextra
LIBS        = -lm
RM          = rm -f

PATH_SRCS   = src/
HDRS        = $(PATH_SRCS)ft_ping.h
SRCS_FILES  = main.c check.c icmp.c init.c print.c rtts.c
SRCS        = $(addprefix $(PATH_SRCS), $(SRCS_FILES))

OBJS        = $(SRCS:.c=.o)

all : $(NAME)

$(NAME) : $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS) $(LIBS)

%.o : %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	$(RM) $(OBJS)

fclean : clean
	$(RM) $(NAME)

re : fclean all

.PHONY : all clean fclean re