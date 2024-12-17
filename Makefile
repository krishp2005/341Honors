.PHONY: install uninstall mount unmount clean

EXE=fs_mount
FILESYSTEM=course_explorer_fs
BACKING_DIR=~/.local/share/cs341_fs

mount: unmount
	cd src && make
	mkdir $(BACKING_DIR)
	mkdir $(FILESYSTEM)
	src/$(EXE) $(FILESYSTEM)

unmount:
	if [ -d $(FILESYSTEM) ]; then fusermount -u $(FILESYSTEM); fi
	rm -rf $(BACKING_DIR)
	rm -rf $(FILESYSTEM)

install: uninstall
	# python3 -m pip install -r bin/requirements.txt
	cp bin/course_explorer_api.py /usr/bin/cs341_course_explorer_api.py

clean: unmount
	cd src && make clean

uninstall: clean
	rm -f /usr/bin/cs341_course_explorer_api.py

