.PHONY: null install uninstall

null:

install: uninstall
	# python3 -m pip install -r bin/requirements.txt
	cp bin/course_explorer_api.py /usr/bin/cs341_course_explorer_api.py

uninstall:
	rm -f /usr/bin/cs341_course_explorer_api.py

