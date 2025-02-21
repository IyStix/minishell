CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99 -Wvla

SRC = src/main.c src/lexer/lexer.c src/parser/parser.c src/exec/exec.c src/exec/builtins.c

minishell: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o minishell

check: minishell
	@echo "Running test suite..."
	@chmod +x tests/testsuite.sh
	@./tests/testsuite.sh

.PHONY: minishell check
