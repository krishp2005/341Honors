CC = clang -std=gnu99
WARNINGS = -Wall -Wextra -pedantic
FUSECMD = `pkg-config fuse3 --cflags --libs` 
CFLAGS = $(WARNINGS) $(FUSECMD) -g

EXE=hardcoded_fs
SRC=fs.c
LIBS=libs/sstring.c libs/vector.c
INCLUDES=includes

FILESYSTEM = course_explorer_fs
.PHONY: build mount unmount

$(EXE): $(SRC)
	$(CC) $(CFLAGS) -I$(INCLUDES) $(LIBS) $^ -o $(EXE)

mount: $(EXE)
	mkdir $(FILESYSTEM)
	./$(EXE) $(FILESYSTEM)

unmount: $(EXE)
	fusermount -u $(FILESYSTEM)
	rmdir $(FILESYSTEM)

clean:
	rm -rf $(EXE)
