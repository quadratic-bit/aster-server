CC ?= gcc
AR ?= ar
RM ?= rm -f
MKDIR_P ?= mkdir -p

BIN_DIR := bin
BUILD_DIR := build
INC_PUBLIC := headers
INC_SRC := src/aster

LIB_SRC := $(wildcard src/aster/*.c)
MAIN_SRC := src/main.c
TEST_SRC := $(wildcard tests/*.c)

LIB_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(LIB_SRC))
MAIN_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(MAIN_SRC))
TEST_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SRC))

LIB_STATIC := $(BUILD_DIR)/libaster.a
DEPFILES := $(LIB_OBJS:.o=.d) $(MAIN_OBJS:.o=.d) $(TEST_OBJS:.o=.d)

MODE ?= debug   # debug | release

CFLAGS_COMMON := -std=c89 -pedantic-errors -Wall -Wextra -Werror \
				 -Wshadow -Wformat=2 -Wstrict-prototypes -Wmissing-prototypes \
				 -Wmissing-declarations -Wmissing-field-initializers \
				 -Wcast-align -Wwrite-strings -Wold-style-definition \
				 -Wpointer-arith -Wstrict-aliasing=2 \
				 -I$(INC_PUBLIC) -I$(INC_SRC)

ifeq ($(SAN),1)
	CFLAGS_SAN := -fsanitize=address,undefined
	LDFLAGS_SAN := -fsanitize=address,undefined
endif

CFLAGS_debug := -O1 -g -fstack-protector-all $(CFLAGS_SAN)
LDFLAGS_debug := $(LDFLAGS_SAN)

CFLAGS_release := -O2 -flto -fstack-protector-strong
LDFLAGS_release:= -flto

CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_$(MODE))
LDFLAGS := $(LDFLAGS_$(MODE))
#LDLIBS := -lz
LDLIBS :=

DEPFLAGS := -MMD -MP

.PHONY: all clean distclean test run help

all: $(BIN_DIR)/server $(BIN_DIR)/test

$(BIN_DIR)/server: $(LIB_STATIC) $(MAIN_OBJS) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $(MAIN_OBJS) $(LIB_STATIC) $(LDLIBS)

$(BIN_DIR)/test: $(LIB_STATIC) $(TEST_OBJS) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $(TEST_OBJS) $(LIB_STATIC) $(LDLIBS)

$(LIB_STATIC): $(LIB_OBJS) | $(BUILD_DIR)
	$(AR) rcs $@ $(LIB_OBJS)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

test: $(BIN_DIR)/test
	./$(BIN_DIR)/test

run: $(BIN_DIR)/server
	./$(BIN_DIR)/server

help:
	@echo "Targets: all (default), run, test, clean, distclean"
	@echo "Modes:   MODE=debug (default) | MODE=release"
	@echo "SAN=1 to enable ASan/UBSan in debug"

$(BIN_DIR) $(BUILD_DIR):
	$(MKDIR_P) $@

clean:
	$(RM) -r $(BUILD_DIR) $(BIN_DIR)/test $(BIN_DIR)/server

distclean: clean

-include $(DEPFILES)
