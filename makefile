CC = gcc
EXAMPLES = user multi_user user_memmacro disk_write disk_read mem_clear
FLAGS = -Wall -Wextra

.PHONY: example cleanex run
example:
	mkdir examples/bin

	$(foreach file,$(EXAMPLES),$(CC) examples/$(file).c -o examples/bin/$(file).exe $(FLAGS);)

cleanex:
	rm -r examples/bin/

run:
	$(foreach file,$(EXAMPLES), echo Running $(file): ; ./examples/bin/$(file).exe;)