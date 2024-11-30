CC = clang -std=gnu99
WARNINGS = -Wall -Wextra -pedantic
FUSECMD = `pkg-config fuse3 --cflags --libs` 
CFLAGS = $(WARNINGS) $(FUSECMD) -g
LIBS=libs/sstring.c libs/vector.c
INCLUDES=includes

EXE=naive
SRC=naive_filesystem.c client.c
FILESYSTEM=naive_filesystem
BACKING_DIR=~/.local/share/cs341_fs

.PHONY: build mount unmount

$(EXE): $(SRC)
	$(CC) $(CFLAGS) -I$(INCLUDES) $(LIBS) $^ -o $(EXE)

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
