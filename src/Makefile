CC = clang -std=gnu99
WARNINGS = -Wall -Wextra -pedantic
FUSECMD = `pkg-config fuse3 --cflags --libs` 
CFLAGS = $(WARNINGS) $(FUSECMD) -g

EXE=fs_mount
SRC=filesystem.c

.PHONY: clean

$(EXE): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(EXE)
