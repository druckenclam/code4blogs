CFLAGS=-g -Wall
COMPILER=gcc

dxf_transformer: dxf_transformer.o dxf_utility.o
	$(COMPILER) -o dxf_transformer dxf_transformer.o dxf_utility.o

dxf_transformer.o: dxf_transformer.c dxf_utility.h
	$(COMPILER) -c $(CFLAGS) dxf_transformer.c

dxf_utility.o: dxf_utility.c dxf_utility.h
	$(COMPILER) -c $(CFLAGS) dxf_utility.c

clean:
	rm -rf dxf_transformer dxf_transformer.o dxf_utility.o
