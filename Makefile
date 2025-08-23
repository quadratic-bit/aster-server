CC := gcc
CFLAGS_DEBUG := -std=c89 -pedantic-errors -Wall -Wextra -Werror \
		-Wshadow -Wformat=2 -Wfloat-equal -Wstrict-prototypes \
		-Wmissing-prototypes -Wmissing-declarations -fno-omit-frame-pointer \
		-Wmissing-field-initializers -Wconversion -Wsign-conversion \
		-Wlogical-op -Wcast-align -Wwrite-strings -Wredundant-decls \
		-Wold-style-definition -Winline -Wpointer-arith -Wstrict-overflow=5 \
		-Wstrict-aliasing=2 -Wcast-qual -Wunreachable-code \
		-D_FORTIFY_SOURCE=2 \
		-Wnull-dereference \
		-fanalyzer \
		-fsanitize=address,undefined \
		-O1 \
		-g \
		-fstack-protector-all \
		-fno-common \
		-fno-builtin \
		-Werror=implicit-function-declaration

CFLAGS_RELEASE := -std=c89 -pedantic-errors -O2 \
				  -flto -fstack-protector-strong -fno-common -fno-builtin \
				  -D_FORTIFY_SOURCE=2 -march=native

.PHONY: clean test release

bin/server: bin src/main.c src/parser.* src/request.* src/str.* src/response.* src/datetime.*
	$(CC) $(CFLAGS_DEBUG) src/main.c src/parser.c src/request.c src/str.c src/response.c src/datetime.c -o bin/server

bin/release: bin src/main.c src/parser.* src/request.* src/str.* src/response.* src/datetime.*
	$(CC) $(CFLAGS_RELEASE) src/main.c src/parser.c src/request.c src/str.c src/response.c src/datetime.c -o bin/server

bin:
	mkdir -p bin

test: bin/test

bin/test: src/test_parser.c src/test.* src/parser.* src/request.* src/str.* src/response.* src/datetime.*
	$(CC) -std=c89 -pedantic-errors $^ -o $@
	./$@
	rm -f ./$@

clean:
	rm -rf ./bin
