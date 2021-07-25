all:
	rm -rf ./Project_Code/ubash
	gcc -std=c11 -Wall -pedantic -Werror -ggdb ./Project_Code/*.c -o ./Project_Code/ubash

clean:
	rm -rf ./Project_Code/ubash