all:
	lex cterm.l
	yacc -d cterm.y
	g++ -c refenviron.cpp
	g++ -c littab.cpp
	g++ -c codegen.cpp
	g++ -c optimizer.cpp
	g++ -c funcdesc.cpp
	g++ -c funcbody.cpp
	g++ -c list.cpp
	gcc -c lex.yy.c
	gcc -c y.tab.c
	g++ -o cterm lex.yy.o y.tab.o codegen.o optimizer.o funcbody.o refenviron.o list.o littab.o funcdesc.o -ll -ly
clean:
	rm -f *.o cterm lex.yy.c y.tab.c result y.tab.h
res:
	nasm -felf -oresult.o result.asm
	ld -oresult /usr/lib/crt1.o /usr/lib/crti.o result.o /usr/lib/crtn.o -lc --dynamic-linker /lib/ld-linux.so.2
