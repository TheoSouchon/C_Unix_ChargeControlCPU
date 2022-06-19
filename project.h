#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<math.h>

//pointeur vers une fonction
typedef int (*func)(void*); 


//1 tache pour 1 fonction
typedef struct{
	func function;
	void* param;
	int etat;
	pid_t pid_number;
}Task;



typedef struct{
	Task* tasks; //tableau dynamique de task
	int nb_task;
}Monitor;

