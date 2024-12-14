CC = clang -std=gnu99
WARNINGS = -Wall -Wextra -pedantic
FUSECMD = `pkg-config fuse3 --cflags --libs` 
CFLAGS = $(WARNINGS) $(FUSECMD) -g

EXE=fs_mount
SRC=filesystem.c
FILESYSTEM=course_explorer_fs
BACKING_DIR=~/.local/share/cs341_fs

.PHONY: mount unmount clean

$(EXE): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

mount: $(EXE) unmount
	mkdir $(BACKING_DIR)
	mkdir $(FILESYSTEM)
	./$(EXE) $(FILESYSTEM)

unmount:
	if [ -d $(FILESYSTEM) ]; then fusermount -u $(FILESYSTEM); fi
	rm -rf $(BACKING_DIR)
	rm -rf $(FILESYSTEM)

clean: unmount unmount
	rm -f $(EXE)
	rm -f $(EXE)
