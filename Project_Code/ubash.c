#include "parsing.h"


/**************************************************************************************************************************
Main.
It returns 0 if some error occurred, otherwise it returns 1.
**************************************************************************************************************************/
int main(int argc, char **argv)
{
	char comm[MAXCHARCOMM];
	queue q;
	printf("\n##### uBASH - Laboratorio 2 di SET(i) 2019/2020 #####\n\n");
	while (1) {
		printCurDir();
		if (!inputCommand(comm))	// I take the input and check if there is ctrl+D
			return 0;
		if (comm[0] == '\n')	// if the user enters a '\n' in the first position of the input
			continue;
		create(&q, MAXQUEUEELEM);
		if (!parser(comm, &q)) {	// I execute the function for the parser
			reset(&q);
			continue;
		}
		reset(&q);
	}
	return 1;
}