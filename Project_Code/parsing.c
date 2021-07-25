#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "parsing.h"


/**************************************************************************************************************************
Function for printing the current directory.
**************************************************************************************************************************/
void printCurDir()
{
	char *dir = NULL;
	fprintf(stdout, GREEN "%s" RESET_COLOR "$ ", (dir = get_current_dir_name()));
	free(dir);
}


/**************************************************************************************************************************
Function that takes the input and checks the ctrl+D at the beginning of the line.
Returns 0 if a ctrl + D was found or the input was not successful.
**************************************************************************************************************************/
unsigned int inputCommand(char *s)
{
	if (fgets(s, MAXCOMM, stdin) == NULL) {	// command input
		fprintf(stdout, "^D\n");	// ctrl+D to exit micro-bash
		return 0;
	}
	return 1;
}


/**************************************************************************************************************************
Function for environment variables.
It returns NULL if some error has occurred, otherwise it returns the correct environment variable.
**************************************************************************************************************************/
char *environmentVar(char *arg_token)
{
	unsigned int i;
	for (i = 0; i < strlen(arg_token); i++)
		arg_token[i] = toupper(arg_token[i]);	// capitalizes the environment variable (e.g.: $home = $HOME)
	if ((arg_token = getenv(arg_token + 1)) == NULL){	// I insert the corresponding environment variable in the arguments
		fprintf(stdout, RED "*** Variabile d'ambiente non esistente ***" RESET_COLOR "\n");
		return NULL;
	}
	return arg_token;
}


/**************************************************************************************************************************
Function for executing the "cd" command.
It returns 0 if some error occurred, otherwise it returns 1.
**************************************************************************************************************************/
unsigned int cd(char *dir, unsigned int num_arg)	// 0 if error; 1 if correct
{
	if (num_arg > 2) {	// error in the number of arguments for "cd"
		fprintf(stdout, RED "micro-bash: cd: troppi argomenti" RESET_COLOR "\n");
		return 0;
	} else if (num_arg == 1) {	// if you just write "cd" with no other arguments
		if (chdir(getenv("HOME")) == -1)
			fprintf(stdout, RED "micro-bash: cd: %s: File o directory non esistente" RESET_COLOR "\n", dir);
		return 1;
	}
	if (strcmp(dir, "-") == 0 || strcmp(dir, "~") == 0) {	// if you write "cd -" or "cd ~"
		if (chdir(getenv("HOME")) == -1)
			fprintf(stdout, RED "micro-bash: cd: %s: File o directory non esistente" RESET_COLOR "\n", dir);
		return 1;
	}
	if (chdir(dir) == -1) {
		fprintf(stdout, RED "micro-bash: cd: %s: File o directory non esistente" RESET_COLOR "\n", dir);
		return 0;
	}
	return 1;
}


/**************************************************************************************************************************
Function for the execution of a single command, therefore without pipes.
It returns 0 if some error occurred, otherwise it returns 1.
**************************************************************************************************************************/
unsigned int execSingleCommand(char **arg_token, int num_arg, int fd_in, int fd_out)
{
	pid_t child_pid;
	arg_token = (char **)realloc(arg_token, sizeof(char *) * (num_arg + 1));
	arg_token[num_arg] = NULL;
	if ((child_pid = fork()) == -1)
		return 0;
	if (child_pid == 0) {	// SON PROCESS
		if (fd_in >= 0)
			if (dup2(fd_in, STDIN_FILENO) == -1) {	// if I have an input redirect
				perror("Errore in dup2 per reindirizzamento input\n");
				return 0;
			}
		if (fd_out >= 0)
			if (dup2(fd_out, STDOUT_FILENO) == -1) {	// if I have an output redirect
				perror("Errore in dup2 per reindirizzamento output\n");
				return 0;
			}
		execvp(arg_token[0], arg_token);	// I execute the command
		// if i get here the execvp has failed
		fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
		exit(EXIT_FAILURE);
	} else {
		// FATHER PROCESS
		if (wait(NULL) == -1) {
			free(arg_token);
			return 0;
		}
		free(arg_token);
	}
	return 1;
}


/**************************************************************************************************************************
Function for input redirection.
It returns -1 if any errors occurred, otherwise returns the file descriptor of the file.
**************************************************************************************************************************/
int openRedirInput(char *arg_token)
{
	int fd_in = -2;
	if ((fd_in = open(arg_token + 1, O_RDONLY)) < 0) {
		fprintf(stdout, RED "micro-bash: %s: File o directory non esistente" RESET_COLOR "\n", arg_token + 1);
		return -1;
	}
	return fd_in;
}


/**************************************************************************************************************************
Function for output redirection.
It returns -1 if any errors occurred, otherwise returns the file descriptor of the file.
**************************************************************************************************************************/
int openRedirOutput(char *arg_token)
{
	int fd_out = -2;
	if ((fd_out = open(arg_token + 1, O_TRUNC | O_CREAT | O_RDWR, 0666)) < 0) {
		fprintf(stdout, RED "micro-bash: Errore in apertura del file per reindirizzamento in output" RESET_COLOR "\n");
		return -1;
	}
	return fd_out;
}


/**************************************************************************************************************************
Function that does wait for each child of the parent process and checks if a child process has been stopped with status
different from 0.
It returns 0 if some error occurred, otherwise it returns 1.
**************************************************************************************************************************/
unsigned int wait_children_inPipe(int numPipes, int *status, int *pid)
{
	for (int i = 0; i < numPipes + 1; i++) {
		if (wait(status) == -1) {
			return 0;
		}
		if (WIFEXITED(*status) && WEXITSTATUS(*status) != 0)
			fprintf(stdout, LIGHT_BLUE "Il processo con pid %d termina con status %d" RESET_COLOR "\n", *pid, WEXITSTATUS(*status));
	}
	return 1;
}


/**************************************************************************************************************************
Function for closing the file descriptor of the pipe and resetting the input and output of the program.
It returns 0 if some error occurred, otherwise it returns 1.
**************************************************************************************************************************/
unsigned int close_pipe(unsigned int *stdin_safe, unsigned int *stdout_safe, int numPipes, int *pipefds)
{
	for (int i = 0; i < 2 * numPipes; i++)
		if (close(pipefds[i]) == -1)
			break;
	free(pipefds);
	if (dup2(*stdin_safe, 0) == -1) {	// reset the input
		perror("Errore in dup2\n");
		return 0;
	}
	if (dup2(*stdout_safe, 1) == -1) {	// reset the output
		perror("Errore in dup2\n");
		return 0;
	}
	if (close(*stdout_safe) == -1)
		return 0;
	if (close(*stdin_safe) == -1)
		return 0;
	return 1;
}


/**************************************************************************************************************************
Function that checks if:
  - I have the ">" and then a space;
  - I have other after the ">file.extension";
  - the "<" is in the correct position.
It returns 0 if some error occurred, otherwise it returns 1.
**************************************************************************************************************************/
unsigned int checkErrorPipedCommand(queue * q)
{
	queue q2;
	// Support queue creation and copy
	create(&q2, size(q));
	copyQueue(q, &q2);
	
	char * s1 = NULL;
	do{						// I take the first command (command up to pipe or empty queue)
		if (isEmpty(&q2))
			break;
		s1 = dequeue(&q2);	// I take first element from the support queue
	} while (strcmp(s1, "|") != 0); // as long as I have pipes
	while (!isEmpty(&q2)){
		s1 = dequeue(&q2);		// I take the second command
		if (s1[0] == '>'){	
			if (strlen(s1) == 1) {	// I have the ">" and then a space: that's not good
				fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
				reset(&q2);
				return 0;
			}
			if (!isEmpty(&q2)) {	// I have something else after the ">file.extension": that's not good
				fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
				reset(&q2);
				return 0;
			}
		}
		if (s1[0] == '<') {	// error because the "<" I can only have it on the first command that is checked outside the while
			fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
			reset(&q2);
			return 0;
		}
	}
	reset(&q2);
	return 1;
}


/**************************************************************************************************************************
Function for executing commands with the pipe.
It returns 0 if some error occurred, otherwise it returns 1.
**************************************************************************************************************************/
unsigned int runPipedCommands(queue * q, char **command, int n_arg, int numPipes)
{
	int status, *pipefds;
	unsigned int i, first = 0, j = 0;
	pid_t pid;
	unsigned int redirect_Out = 0;	// 0 false; 1 true
	int std_save;		// for the ">" and "<"
	unsigned int stdin_safe = dup(STDIN_FILENO);	// variable to save the stdin
	unsigned int stdout_safe = dup(STDOUT_FILENO);	// variable to save the stdout
	pipefds = (int *)malloc(sizeof(int) * (2 * numPipes));
	for (i = 0; i < numPipes; i++)
		if (pipe(pipefds + i * 2) == -1) {
			perror("Errore in pipe\n");
			close_pipe(&stdin_safe, &stdout_safe, numPipes, pipefds);
			free(command);
			return 0;
		}
	// I check the "<"
	for (int k = 0; k < n_arg - 1; k++)	// I check if it has been inserted in the first command in the wrong position (i.e. before the last argument)
		if (command[k][0] == '<') {
			fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
			close_pipe(&stdin_safe, &stdout_safe, numPipes, pipefds);
			free(command);
			return 0;
		}
	// I check if the "<" is in the last position and proceed to change the standard input
	if (command[n_arg - 1][0] == '<') {
		if (strlen(command[n_arg - 1]) == 1) {
			fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
			close_pipe(&stdin_safe, &stdout_safe, numPipes, pipefds);
			return 0;
		}
		if ((std_save = openRedirInput(command[n_arg - 1])) == -1) {
			free(pipefds);
			free(command);
			return 0;
		}
		n_arg--;
		// if there is to change the standard input with the file or path (so using "<")
		if (dup2(std_save, 0) == -1) {
			perror("Errore dup2 stdin in file\n");
			close(std_save);
			free(pipefds);
			free(command);
			return 0;
		}
		if (close(std_save) == -1) {
			free(pipefds);
			free(command);
			return 0;
		}
	}


	while (!isEmpty(q)) {
		
		while (!isEmpty(q) && j > 0) {
			command = (char **)realloc(command, sizeof(char *) * (n_arg + 1));
			char *singleArg;
			singleArg = dequeue(q);
			if (singleArg[0] == '|'){	// after pipe I have nothing left
				break;
			}

			// I check the ">"
			if (singleArg[0] == '>') {
				redirect_Out = 1;
				// I take the new stdout
				if ((std_save = openRedirOutput(singleArg)) == -1) {
					wait_children_inPipe(numPipes, &status, &pid);
					close_pipe(&stdin_safe, &stdout_safe, numPipes, pipefds);
					free(command);
					return 0;
				}
				break;
			}
			command[n_arg] = singleArg;
			n_arg++;
		}
		command = (char **)realloc(command, sizeof(char *) * (n_arg + 1));
		command[n_arg] = NULL;	// null value at the end for execvp
		pid = fork();
		if (pid == 0) {	// SON PROCESS
			// OUTPUT
			if (redirect_Out == 1) {
				// if there is to change standard output with the file (so using ">")
				if (dup2(std_save, 1) == -1) {
					perror("Errore dup2 stdout in file\n");
					close_pipe(&stdin_safe, &stdout_safe, numPipes, pipefds);
					free(command);
					return 0;
				}
				if (close(std_save) == -1) {
					free(command);
					free(pipefds);
					return 0;
				}
			} else {
				// if it is not the last command
				if (j < 2 * numPipes) {
					if (dup2(pipefds[j + 1], 1) == -1) {
						perror("Errore dup2 output - PIPE\n");
						close_pipe(&stdin_safe, &stdout_safe, numPipes, pipefds);
						free(command);
						return 0;
					}
				}
			}

			// INPUT
			// if I am not in the first command
			if (j != 0) {
				if (dup2(pipefds[j - 2], 0) == -1) {
					perror("Errore dup2 input - PIPE\n");
					close_pipe(&stdin_safe, &stdout_safe, numPipes, pipefds);
					free(command);
					return 0;
				}
			}

			// I close all open file descriptor for pipes
			for (i = 0; i < 2 * numPipes; i++)
				if (close(pipefds[i]) == -1)
					break;

			// I execute the instruction
			if (execvp(command[first], command + first) == -1) {
				fprintf(stdout, RED "*** COMANDO ERRATO!!! *** - Errore di: %s" RESET_COLOR "\n", command[first]);
				close_pipe(&stdin_safe, &stdout_safe, numPipes, pipefds);
				free(command);
				exit(EXIT_FAILURE);
			}

		} else if (pid < 0) {	// if the fork gave an error
			perror("Errore fork in pipe\n");
			free(pipefds);
			free(command);
			return 0;
		}
		// FATHER PROCESS
		j += 2;
		first = n_arg;
	}

	// I close all open file descriptor for pipes
	for (i = 0; i < 2 * numPipes; i++)
		if (close(pipefds[i]) == -1)
			break;
	// I do wait for each child and check if any of them have failed to execute and close the pipes by resetting inputs and outputs
	if (!wait_children_inPipe(numPipes, &status, &pid)) {
		free(pipefds);
		free(command);
		return 0;
	}
	if (!close_pipe(&stdin_safe, &stdout_safe, numPipes, pipefds)) {
		free(command);
		return 0;
	}
	free(command);
	return 1;
}


/**************************************************************************************************************************
Function that analyzes the commands entered and calls the correct functions in the case of a single command, redirection
input/output and pipe.
It returns 0 if some error occurred, otherwise it returns 1.
**************************************************************************************************************************/
unsigned int execCommand(queue * q, unsigned int num_pipe)
{
	char *singleArg, *singleArg2;
	int n_arg = 0, n_comm = 0;
	char **commArray = NULL;
	if (isEmpty(q))	// if there are no commands
		return 0;
	while (!isEmpty(q)) {
		commArray = (char **)realloc(commArray, sizeof(char *) * (n_arg + 1));
		singleArg = dequeue(q);	// I take the argument
		if (strcmp(singleArg, "|") == 0) {	// I check the pipe
			if (n_arg == 0) {	// if I have a pipe at the beginning of the line
				fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
				free(commArray);
				return 0;
			}
			n_comm++;
			if (strcmp(commArray[0], "cd") == 0) {	// if I have the "cd" command along with a pipe it must fail
				fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
				free(commArray);
				return 0;
			}
			if (!runPipedCommands(q, commArray, n_arg, num_pipe))	// I execute the function for the pipe
				return 0;
			return 1;
		} else if (singleArg[0] == '<' && !isEmpty(q) && num_pipe == 0) {	// if I have a "<" (and then a ">")
			if (n_arg == 0) {
				fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
				free(commArray);
				return 0;
			}
			n_comm++;
			singleArg2 = dequeue(q);
			if (!isEmpty(q)) {	// if I have anything else after "<file.extension >file.extension" I have an error
				fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
				free(commArray);
				return 0;
			}
			if (singleArg2[0] == '>') {	// if I found the ">"
				n_comm++;
				if (n_comm > 2) {
					fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
					free(commArray);
					return 0;
				}
				if (strcmp(commArray[0], "cd") == 0) {	// if I have "cd" I have error
					fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
					free(commArray);
					return 0;
				}
				int fd_out;	// I change output
				if ((fd_out = openRedirOutput(singleArg2)) == -1) {
					free(commArray);
					return 0;
				}
				int fd_in;	// I change input
				if ((fd_in = openRedirInput(singleArg)) == -1) {
					free(commArray);
					return 0;
				}
				pid_t child_pid;
				commArray[n_arg] = NULL;	// null value at the end for execvp
				if ((child_pid = fork()) == -1) {
					free(commArray);
					return 0;
				}
				if (child_pid == 0) {	// SON PROCESS
					if (dup2(fd_in, STDIN_FILENO) == -1) {
						perror("Errore dup2 per ridirezione in input\n");
						free(commArray);
						return 0;
					}
					if (dup2(fd_out, STDOUT_FILENO) == -1) {
						perror("Errore dup2 per ridirezione in input\n");
						free(commArray);
						return 0;
					}
					execvp(commArray[0], commArray);	// I execute the command
					fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
					free(commArray);
					exit(EXIT_FAILURE);
				} else {	// FATHER PROCESS
					if (wait(NULL) == -1) {
						free(commArray);
						return 0;
					}
					if (close(fd_in) == -1) {
						free(commArray);
						return 0;
					}
					if (close(fd_out) == -1) {
						free(commArray);
						return 0;
					}
					free(commArray);
				}
			} else {
				fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
				free(commArray);
				return 0;
			}
			return 1;
		} else if (singleArg[0] == '>' && !isEmpty(q) && num_pipe == 0) {	// if I have ">" (and then a "<")
			if (n_arg == 0) {
				fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
				free(commArray);
				return 0;
			}
			n_comm++;
			singleArg2 = dequeue(q);
			if (!isEmpty(q)) {	// if I have anything else after ">file.extension <file.extension" I have an error
				fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
				free(commArray);
				return 0;
			}
			if (singleArg2[0] == '<') {	// if after I have "<" 
				n_comm++;
				if (n_comm > 2) {
					fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
					free(commArray);
					return 0;
				}
				if (strcmp(commArray[0], "cd") == 0) {	// if I have "cd" I have error
					fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
					free(commArray);
					return 0;
				}
				int fd_out;	// I change output
				if ((fd_out = openRedirOutput(singleArg)) == -1) {
					free(commArray);
					return 0;
				}
				int fd_in;	// I change input
				if ((fd_in = openRedirInput(singleArg2)) == -1) {
					free(commArray);
					return 0;
				}
				pid_t child_pid;
				commArray[n_arg] = NULL;	// null value at the end for execvp
				if ((child_pid = fork()) == -1) {
					free(commArray);
					return 0;
				}
				if (child_pid == 0) {	// SON PROCESS
					if (dup2(fd_in, STDIN_FILENO) == -1) {
						perror("Errore dup2 per ridirezione in input\n");
						free(commArray);
						return 0;
					}
					if (dup2(fd_out, STDOUT_FILENO) == -1) {
						perror("Errore dup2 per ridirezione in output\n");
						free(commArray);
						return 0;
					}
					execvp(commArray[0], commArray);	// I execute the command
					fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
					free(commArray);
					exit(EXIT_FAILURE);
				} else {	// FATHER PROCESS
					if (wait(NULL) == -1) {
						free(commArray);
						return 0;
					}
					if (close(fd_in) == -1) {
						free(commArray);
						return 0;
					}
					if (close(fd_out) == -1) {
						free(commArray);
						return 0;
					}
					free(commArray);
				}
			} else {
				fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
				free(commArray);
				return 0;
			}
			return 1;
		} else if (singleArg[0] == '<' && isEmpty(q)) {	// simple input redirection control
			if (n_arg == 0) {
				fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
				free(commArray);
				return 0;
			}
			n_comm++;
			if (n_comm > 2) {
				fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
				free(commArray);
				return 0;
			}
			if (strcmp(commArray[0], "cd") == 0) {	// if I have "cd" it's not good
				fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
				free(commArray);
				return 0;
			}
			if (n_comm != 1) {	// I check that the "<" has been inserted in the first command
				fprintf(stdout, RED "*** Errore di ridirezione in input ***" RESET_COLOR "\n");
				free(commArray);
				return 0;
			}
			int fd_in;	// I change input
			if ((fd_in = openRedirInput(singleArg)) == -1) {
				free(commArray);
				return 0;
			}
			if (!execSingleCommand(commArray, n_arg, fd_in, -2)) {	// I use the function for executing the command with input redirection
				close(fd_in);
				free(commArray);
				return 0;
			}
			if (close(fd_in) == -1) {
				free(commArray);
				return 0;
			}
			return 1;
		} else if (singleArg[0] == '>') {	// I check output redirection
			if (n_arg == 0) {
				fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
				free(commArray);
				return 0;
			}
			n_comm++;
			if (strcmp(commArray[0], "cd") == 0) {	// if I have "cd" it's not good
				fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
				free(commArray);
				return 0;
			}
			if (!isEmpty(q)) {	// I check that the ">" is the last command
				fprintf(stdout, RED "*** Errore di ridirezione in output ***" RESET_COLOR "\n");
				free(commArray);
				return 0;
			}
			int fd_out;	// I change output
			if ((fd_out = openRedirOutput(singleArg)) == -1) {
				free(commArray);
				return 0;
			}
			if (!execSingleCommand(commArray, n_arg, -2, fd_out)) {	// I use the function for executing the command with output redirection
				close(fd_out);
				free(commArray);
				return 0;
			}
			if (close(fd_out) == -1) {
				free(commArray);
				return 0;
			}
			return 1;
		}
		commArray[n_arg] = singleArg;
		n_arg++;
	}
	// if I didn't have "|" or "<" or ">" I do the operation on the single command
	if (strcmp(commArray[0], "cd") == 0) {	// if the first argument is "cd"
		if (n_arg == 1){	// if there is only one argument, that is "cd"
			if (!cd(NULL, n_arg)) {
				free(commArray);
				return 0;	// if it fails
			}
		} else {
			if (!cd(commArray[1], n_arg)) {
				free(commArray);
				return 0;	// if it fails
			}
		}
		free(commArray);
	} else {
		if (!execSingleCommand(commArray, n_arg, -2, -2)) {	// I use the function for single command execution
			free(commArray);
			return 0;
		}
	}
	return 1;
}


/**************************************************************************************************************************
Function that splits the string entered in input by the user.
It returns 0 if some error occurred, otherwise it returns 1.
**************************************************************************************************************************/
unsigned int parser(char *complete_comm, queue * q)
{
	unsigned int num_pipe = 0, i;
	char *comm_token, *arg_token;
	complete_comm[strlen(complete_comm) - 1] = 0;	// to avoid including the final '\n' in the string
	for (i = 0; i < strlen(complete_comm); i++)	// to remove tabs
		if (complete_comm[i] == '\t')
			complete_comm[i] = ' ';
	if(complete_comm[0] == '|' || complete_comm[strlen(complete_comm)-1] == '|'){
		fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
		return 0;
	}
	while ((comm_token = strtok_r(complete_comm, "|", &complete_comm))) {	// decomposition by pipe "|"
		while ((arg_token = strtok_r(comm_token, " ", &comm_token))) {	// decomposition by spaces
			if (arg_token[0] == '$') {	// if at the beginning of an argument there is a '$'
				if ((arg_token = environmentVar(arg_token)) == NULL)
					return 0;
			}
			
			enqueue(q, arg_token);
		}
		if (strlen(complete_comm) > 0) {	// I insert the pipe
			enqueue(q, "|");
			num_pipe++;
		}
		if (!checkErrorPipedCommand(q))	// I check for redirection errors
			return 0;
	}
		
	if (checkPipeError(q)) {	// I check if I have more than one consecutive pipe
		fprintf(stdout, RED "*** COMANDO ERRATO!!! ***" RESET_COLOR "\n");
		return 0;
	}
	if (!execCommand(q, num_pipe))	// command execution
		return 0;
	return 1;
}