COMPILER = gcc
BIN_NAME = fuse_mount

CFLAGS = -std=c99 -Wall -Wextra -Wno-unused-parameter -Wformat-security -Wno-unused-result -pedantic -fPIC `pkg-config fuse3 --cflags`
LIBS = `pkg-config fuse3 --libs`

SRC_DIR = src
SRC_HEADERS := $(wildcard $(SRC_DIR)/*.h)
SRC_SOURCES := $(wildcard $(SRC_DIR)/*.c)
SRC_OBJECTS := $(addprefix $(SRC_DIR)/, $(notdir $(SRC_SOURCES:.c=.o)))

TEST_DIR = tests
TEST_HEADERS := $(wildcard $(TEST_DIR)/*.h)
TEST_SOURCES := $(wildcard $(TEST_DIR)/*.c)
TEST_FILES := $(addprefix $(TEST_DIR)/, $(notdir $(TEST_SOURCES:.c=.test)))

ifeq ($(DEBUG), true)
	CFLAGS += -g3
else
# TODO: use -s flag?
	CFLAGS += -O2 -flto -Werror
endif

# Always save debug info for perf
ifeq ($(PERF), true)
	CFLAGS += -g3
endif

all: executable

executable: $(BIN_NAME)
	@ echo 'To mount: ./'$(BIN_NAME)' -f [mount point]'

test: $(BIN_NAME) $(TEST_FILES)
	@ bash ./tests/run_tests.sh

$(BIN_NAME): $(SRC_OBJECTS)
	@ printf "%8s %-40s %s\n" $(COMPILER) $(BIN_NAME)
	@ $(COMPILER) $(CFLAGS) $^ -o $(BIN_NAME) $(LIBS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_HEADERS)
	@ printf "%8s %-40s %s\n" $(COMPILER) $<
	@ $(COMPILER) -c $(CFLAGS) -o $@ $< $(LIBS)

$(TEST_DIR)/%.test: $(TEST_DIR)/%.c $(TEST_HEADERS)
	@ printf "%8s %-40s %s\n" $(COMPILER) $<
	@ $(COMPILER) $(CFLAGS) -o $@ $< $(LIBS) -lcheck -lsubunit -lrt -lm -pthread

# build: $(FILESYSTEM_FILES)
# 	$(COMPILER) -std=c99 -O2 $(FILESYSTEM_FILES) -o lsysfs `pkg-config fuse --cflags --libs`
# 	@ echo 'To Mount: ./lsysfs -f [mount point]'
#	gcc -std=c99 -O2 lsysfs.c -o lsysfs `pkg-config fuse --cflags --libs`
#
# clean:
# 	rm ssfs

