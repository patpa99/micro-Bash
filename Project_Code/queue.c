#include "queue.h"


/**************************************************************************************************************************
Function that creates the queue with dimension dim.
**************************************************************************************************************************/
void create(queue * q, unsigned int dim)
{
	q->array = malloc(dim * sizeof(char *));
	q->first = 0;
	q->last = 0;
}


/**************************************************************************************************************************
Function that empties the queue.
**************************************************************************************************************************/
void reset(queue * q)
{
	free(q->array);
	q->first = 0;
	q->last = 0;
}


/**************************************************************************************************************************
Function that returns 1 if the queue is empty, otherwise it returns 0.
**************************************************************************************************************************/
unsigned int isEmpty(queue * q)
{
	if (q->first == q->last)
		return 1;
	return 0;
}


/**************************************************************************************************************************
Function that inserts items at the end of the queue.
**************************************************************************************************************************/
void enqueue(queue * q, char *str)
{
	q->array[q->last] = str;
	q->last++;
}


/**************************************************************************************************************************
Function that removes from the beginning of the queue and returns the removed element.
**************************************************************************************************************************/
char *dequeue(queue * q)
{
	q->first++;
	return q->array[q->first - 1];
}


/**************************************************************************************************************************
Function that returns 1 if there are two consecutive pipes in the queue, otherwise it returns 0.
**************************************************************************************************************************/
unsigned int checkPipeError(const queue * q)
{
	for (int i = q->first; i < q->last - 1; i++) {
		if (q->array[i][0] == '|' && q->array[i + 1][0] == '|')
			return 1;
		if (q->array[i][0] == '|' && (q->array[i + 1][0] == '<' || q->array[i + 1][0] == '>'))
			return 1;
	}
	if(q->array[q->last-1][0] == '|')
		return 1;
	return 0;
}


/**************************************************************************************************************************
Function that returns the number of items in the queue.
**************************************************************************************************************************/
unsigned int size(const queue * q)
{
	return (q->last - q->first);
}


/**************************************************************************************************************************
Function that copies the queue passed as the first parameter to the queue passed as the second parameter.
**************************************************************************************************************************/
void copyQueue(const queue * q, queue * q2)
{
	unsigned int length;
	length = size(q);
	for (unsigned int i = 0; i < length; i++)
		enqueue(q2, q->array[i]);
}


/**************************************************************************************************************************
Function that prints all items in the queue.
**************************************************************************************************************************/
void printQueue(const queue * q)
{
	unsigned int i;
	for (i = q->first; i < q->last; i++)
		fprintf(stdout, "%s\n", q->array[i]);
}