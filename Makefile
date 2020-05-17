all : test

test : test.c dco.c 
	gcc -g -Wall -o $@ $^
	./test

.PHONY : clean 
clean : 
	rm test
