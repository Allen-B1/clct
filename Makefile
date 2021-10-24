KLIB = ../../ext/klib
CCLIB = ../../ext/Collections-C

.PHONY: benchmark
benchmark:
	gcc -Wall benchmark/mem.c benchmark/benchmark.c -g -o bench -ldl \
		arrlist.c rhmap.c -I. \
		-I$(KLIB) \
		`pkg-config --cflags --libs glib-2.0`  \
		-I$(CCLIB)/src/include `find $(CCLIB)/src -name "*.c"`