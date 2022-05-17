COMPILER = gcc
BIN_NAME = fuse_mount

CFLAGS = -std=c99 -Wall -Wextra -Wno-unused-parameter -Wformat-security -Wno-unused-result -pedantic -fPIC `pkg-config fuse --cflags`
LIBS = `pkg-config fuse --libs`

SRC_DIR = src
SRC_HEADERS := $(wildcard $(SRC_DIR)/*.h)
SRC_SOURCES := $(wildcard $(SRC_DIR)/*.c)
SRC_OBJECTS := $(addprefix $(SRC_DIR)/, $(notdir $(SRC_SOURCES:.c=.o)))

ifeq ($(DEBUG), true)
	CFLAGS += -g
else
# TODO: use -s flag?
	CFLAGS += -O2 -flto -Werror
endif

all: executable

executable: $(SRC_OBJECTS)
	@ printf "%8s %-40s %s\n" $(COMPILER) $(BIN_NAME)
	@ $(COMPILER) $(CFLAGS) $^ -o $(BIN_NAME) $(LIBS)
	@ echo 'To mount: ./'$(BIN_NAME)' -f [mount point]'

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_HEADERS)
	@ printf "%8s %-40s %s\n" $(COMPILER) $<
	@ $(COMPILER) -c $(CFLAGS) -o $@ $< $(LIBS)

# build: $(FILESYSTEM_FILES)
# 	$(COMPILER) -std=c99 -O2 $(FILESYSTEM_FILES) -o lsysfs `pkg-config fuse --cflags --libs`
# 	@ echo 'To Mount: ./lsysfs -f [mount point]'
#	gcc -std=c99 -O2 lsysfs.c -o lsysfs `pkg-config fuse --cflags --libs`
#
# clean:
# 	rm ssfs

