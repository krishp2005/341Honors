CC = clang -std=gnu99
WARNINGS = -Wall -Wextra -pedantic
FUSECMD = `pkg-config fuse3 --cflags --libs` 
CFLAGS = $(WARNINGS) $(FUSECMD) -g

EXE=hardcoded_fs
SRC=fs.c
LIBS=libs/sstring.c libs/vector.c
INCLUDES=includes

.PHONY: all

all: $(SRC)
	$(CC) $(CFLAGS) -I$(INCLUDES) $(LIBS) $^ -o $(EXE)
	
