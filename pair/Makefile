LDLIBS = -lpthread

pair : pair.o
	gcc -o $@ $^ $(LDLIBS)
pair.o : pair.c
	gcc -o pair.o -c pair.c

