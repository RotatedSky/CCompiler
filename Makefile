OBJECTS= \
	./build/compiler.o \
	./build/cprocess.o \
	./build/lex_process.o \
	./build/lexer.o \
	./build/token.o \
	./build/helpers/buffer.o \
	./build/helpers/vector.o

INCCLUDES= -I./

# -g for debugging simbols
all: ${OBJECTS}
	gcc main.c ${INCCLUDES} ${OBJECTS} -g -o ./main

./build/compiler.o: ./compiler.c
	gcc compiler.c ${INCCLUDES} -o ./build/compiler.o -g -c

./build/cprocess.o: ./cprocess.c
	gcc cprocess.c ${INCCLUDES} -o ./build/cprocess.o -g -c

./build/lex_process.o: ./lex_process.c
	gcc lex_process.c ${INCCLUDES} -o ./build/lex_process.o -g -c

./build/lexer.o: ./lexer.c
	gcc lexer.c ${INCCLUDES} -o ./build/lexer.o -g -c

./build/token.o: ./token.c
	gcc token.c ${INCCLUDES} -o ./build/token.o -g -c

./build/helpers/buffer.o: ./helpers/buffer.c
	gcc ./helpers/buffer.c ${INCCLUDES} -o ./build/helpers/buffer.o -g -c

./build/helpers/vector.o: ./helpers/vector.c
	gcc ./helpers/vector.c ${INCCLUDES} -o ./build/helpers/vector.o -g -c

clean:
	rm ./main
	rm -rf ${OBJECTS}
