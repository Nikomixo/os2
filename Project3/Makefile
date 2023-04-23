pcseml: pcseml.c
	gcc -Wall -Wextra -o $@ $^ -lpthread

pcseml.zip:
	rm -f $@
	zip $@ Makefile pcseml.c
