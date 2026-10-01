minishell.exe: main.o function.o
	gcc -o minishell.exe main.o function.o
main.o: main.c
	gcc -c main.c
function.o: function.c 
	gcc -c function.c 
clean:
	rm *.exe *.o