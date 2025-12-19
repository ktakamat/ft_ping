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
CC          = cc
CFLAGS      = -Wall -Wextra -Werror -I./include -g -D_GNU_SOURCE

# ソースディレクトリ
SRC_DIR     = src
OBJ_DIR     = obj
LIBFT_DIR   = src/libft

# ソースファイル一覧
# 修正: ft_calloc.c を追加し、使っていないファイルを削除しました
SRCS        = $(SRC_DIR)/main.c \
              $(SRC_DIR)/parser.c \
              $(SRC_DIR)/socket.c \
              $(SRC_DIR)/ping.c \
              $(SRC_DIR)/packets.c \
              $(SRC_DIR)/logger.c \
              $(SRC_DIR)/verbose.c \
              $(LIBFT_DIR)/ft_free.c \
              $(LIBFT_DIR)/ft_strf.c \
              $(LIBFT_DIR)/ft_calloc.c

# オブジェクトファイル生成 (src/libft/xxx.c -> obj/libft/xxx.o に対応)
OBJS        = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -lm -o $(NAME)

# ディレクトリ構造を維持してコンパイル
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re