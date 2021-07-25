#include "queue.h"

#define MAXCOMM 1000	// maximum number of commands (for example: "comm1 | comm2 | comm3 | ...")
#define MAXCHARCOMM 1000	// maximum number of characters per command

/**************************************************************************************************************************
Constants to change the color of the micro-bash writings.
**************************************************************************************************************************/
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define LIGHT_BLUE "\x1B[36m"
#define RESET_COLOR "\x1b[0m"


/**************************************************************************************************************************
Function for printing the current directory.
**************************************************************************************************************************/
void printCurDir();


/**************************************************************************************************************************
Function that takes the input and checks the ctrl+D at the beginning of the line.
It returns 0 if a ctrl + D was found or the input was not successful.
**************************************************************************************************************************/
unsigned int inputCommand(char *);


/**************************************************************************************************************************
Useful function to decompose the string inserted in input by the user.
It returns 0 if some error occurred, otherwise it returns 1.
**************************************************************************************************************************/
unsigned int parser(char *, queue *);