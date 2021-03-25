main:
	bison -d nutshell.ypp
	flex -o lex.yy.cpp nutshell.lpp
	g++ -Wall -Wextra -Wno-unused-function nutshell.tab.cpp lex.yy.cpp -o nutshell