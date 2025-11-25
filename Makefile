# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: ktakamat <ktakamat@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/11/24 16:45:31 by ktakamat          #+#    #+#              #
#    Updated: 2025/11/25 16:56:33 by ktakamat         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #


NAME        = ft_ping
CC          = cc
CFLAGS      = -Wall -Wextra -Werror -I./include -g

# ソースディレクトリ
SRC_DIR     = src
OBJ_DIR     = obj
LIBFT_DIR   = src/libft

# ソースファイル一覧
SRCS        = $(SRC_DIR)/main.c \
              $(SRC_DIR)/parser.c \
              $(SRC_DIR)/socket.c \
              $(SRC_DIR)/ping.c \
              $(SRC_DIR)/packets.c \
              $(SRC_DIR)/logger.c \
              $(SRC_DIR)/verbose.c \
              $(LIBFT_DIR)/ft_free.c \
              $(LIBFT_DIR)/ft_int_len.c \
              $(LIBFT_DIR)/ft_str_realloc.c \
              $(LIBFT_DIR)/ft_strf.c

OBJS        = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -lm -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re