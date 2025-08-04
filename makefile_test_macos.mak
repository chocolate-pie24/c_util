TARGET = test

SRC_DIR = core/src containers/src tests
BUILD_DIR = bin
OBJ_DIR = obj

SRC_FILES = $(shell find core/src containers/src tests -name '*.c')
DIRECTORIES = $(shell find $(SRC_DIR) tests -type d)
OBJ_FILES = $(SRC_FILES:%=$(OBJ_DIR)/%.o)

INCLUDE_FLAGS = -Iinclude -Icore/include -Icontainers/include -Itests/include

LINKER_FLAGS += -fprofile-instr-generate -fcoverage-mapping
CC = /opt/homebrew/opt/llvm/bin/clang

COMPILER_FLAGS = -Wall -Wextra -std=c17
ifeq ($(BUILD_MODE), RELEASE_BUILD)
	COMPILER_FLAGS += -O3 -DRELEASE_BUILD -DPLATFORM_MACOS
else
	COMPILER_FLAGS += -g -O0 -DDEBUG_BUILD -DPLATFORM_MACOS
	# カバレッジ計測時のみ
	COMPILER_FLAGS += -fprofile-instr-generate -fcoverage-mapping
endif

.PHONY: all
all: scaffold link

.PHONY: scaffold
scaffold:
	@echo --- scaffolding folder structure... ---
	@echo Create directories into obj/
	@mkdir -p $(addprefix $(OBJ_DIR)/,$(DIRECTORIES))
	@echo Create bin directory.
	@mkdir -p $(BUILD_DIR)
	@echo Done.
	@echo --- compiling source files... ---
	@echo build mode - $(BUILD_MODE)

$(OBJ_DIR)/%.c.o: %.c
	@echo compiling $<...
	$(CC) $< $(COMPILER_FLAGS) -c -o $@ $(INCLUDE_FLAGS)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(COMPILER_FLAGS) $(INCLUDE_FLAGS) -c $< -o $@

.PHONY: link
link: scaffold $(OBJ_FILES)
	@echo --- linking $(TARGET)... ---
	@$(CC) $(OBJ_FILES) -o $(BUILD_DIR)/$(TARGET) $(LINKER_FLAGS)

.PHONY: clean
clean:
	@rm -f $(TARGET)
	@rm -rf $(BUILD_DIR)
	@rm -rf $(OBJ_DIR)
