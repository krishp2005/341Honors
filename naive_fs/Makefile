CC = clang -std=gnu99
WARNINGS = -Wall -Wextra -pedantic
FUSECMD = `pkg-config fuse3 --cflags --libs` 
CFLAGS = $(WARNINGS) $(FUSECMD) -g
LIBS=libs/sstring.c libs/vector.c
INCLUDES=includes

EXE=naive
SRC=naive_filesystem.c
FILESYSTEM=naive_filesystem

EXE2=mirror
SRC2=mirror.c
FILESYSTEM2=mirror_fs
BACKING_DIR=~/.local/share/cs341_fs

.PHONY: build mount unmount mount2 unmount2

$(EXE): $(SRC)
	$(CC) $(CFLAGS) -I$(INCLUDES) $(LIBS) $^ -o $(EXE)

mount: $(EXE) unmount
	mkdir $(FILESYSTEM)
	./$(EXE) $(FILESYSTEM)

unmount:
	if [ -d $(FILESYSTEM) ]; then fusermount -u $(FILESYSTEM); fi
	rm -rf $(FILESYSTEM)

$(EXE2): $(SRC2)
	$(CC) $(CFLAGS) -I$(INCLUDES) $(LIBS) $^ -o $(EXE2)

mount2: $(EXE2) unmount2
	mkdir $(BACKING_DIR)
	mkdir $(FILESYSTEM2)
	./$(EXE2) $(FILESYSTEM2)

unmount2:
	if [ -d $(FILESYSTEM2) ]; then fusermount -u $(FILESYSTEM2); fi
	rm -rf $(BACKING_DIR)
	rm -rf $(FILESYSTEM2)

clean: unmount unmount2
	rm -f $(EXE)
	rm -f $(EXE2)
