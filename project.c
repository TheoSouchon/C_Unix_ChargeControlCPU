#include <string.h>
#include <stdio.h>
#include <math.h>
#include "project.h"
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>


char* stdinFile = "input_standart.txt";
char* stdoutFile = "output_standart.txt";
char* stderrFile = "output_error.txt";
int palier1 = 20; //en pourcentage
int palier2 = 70; //en pourcentage
int palier3 = 90; //en pourcentage
int delais1 = 2; //en secondes
int delais2 = 2; //en secondes
int nbProcessSimult = 10; //nombre de processus en simultanees

int fonction_test (void* param) {
	int *tab = (int*)param;
	int a = tab[0];
	int b = tab[1];
	
	//printf("a: %d , b: %d \n",a,b);
	//printf("Je suis %d venant de %d \n",getpid(),getppid());
	for (int i = 0; i < 1024*1024*150; ++i)
	{
		a=pow(sqrt(pow((a+1),2)),3);
		b=pow(sqrt(pow((b+1),2)),3);
	}
	printf("coucou depuis processus child\n");
	return 0;
}

//init task
Monitor* init_tasks(Monitor* monitor) {
	monitor->tasks = (Task*)malloc(sizeof(Task));
	return monitor;
}

//Init monitor
Monitor* init_monitor() {
	Monitor* monitor=(Monitor*)malloc(sizeof(Monitor));
	monitor->nb_task=0;
	monitor=init_tasks(monitor);
	return monitor;
}

//ajouter tâche à 1 monitor
int submit (Monitor* monitor, int (*func)(void*), void* data,const char* input, const char* output, const char *error) {

	monitor->tasks=realloc(monitor->tasks,(monitor->nb_task+1)*sizeof(Task));
	monitor->nb_task=monitor->nb_task+1;

	monitor->tasks[(monitor->nb_task)-1].function=*func;
	monitor->tasks[(monitor->nb_task)-1].param=data;
	monitor->tasks[(monitor->nb_task)-1].pid_number=rand()%10;


	return 0;
};

char checkStatusProcess(int pid) {
	/*chemin de lecture de fichier*/
		char ligne[100]= "";
		char result[50];
    	sprintf(result, "%d", pid);
    	sprintf(ligne,"%s%s%s","/proc/",result,"/status");
  


    	
    	FILE* fp = fopen(ligne,"r");
    	if(fp == NULL)
   		{
      		printf("Erreur ouverture fichier /proc/status");   
      		exit(1);             
   		}
   		
   		/*caractère de status de processus*/
		fgets(ligne,100,fp);
		fgets(ligne,100,fp);
		fgets(ligne,100,fp);
		char status = ligne[7];
		fclose(fp);
		return status;
}

//exec la fonction dans un processus fils
int fork_exec(Task* task) {
	pid_t pidChild;
	int resFork;

	resFork = fork();
	if(resFork == 0) {

		/*code processus fils*/

		//redirection sortie standart
		int fout,ferr;
		fout = open (stdoutFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
		ferr = open (stderrFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
		close(1);
		close(2);
		dup(fout);
		dup(ferr);
		close(fout);
		close(ferr);

		//start fonction
		func fonction = task->function;
		void* param = task->param;
		int code_retour = fonction(param);

		exit(0);
	} else if (resFork == -1){
		return 1;
	} else {
		task->pid_number = resFork;
	}
	
}

/*
//le premier test de l'idleFraction / idleTime n'est pas réprenstatif du système actuel (sert de reférence ?)
void cpuCheck2() {
	char ligne[100];
	const char d[2] = " ";
	char* token;
	int i = 0,lag;
	int sum = 0, idle, lastSum = 0,lastIdle = 0;
	double idleFraction;
 
	lag = 1; //1s de décalage à chaque fois
 
	while(1){
		FILE* fp = fopen("/proc/stat","r");
	    i = 0;
		fgets(ligne,100,fp);
		fclose(fp);
		//lire tous les éléments d'une ligne 1 à 1 délimité par ' ' (un esapce)
		token = strtok(ligne,d);
	    sum = 0;
		while(token!=NULL){
			//on avance de 1 élément dans la même ligne
			token = strtok(NULL,d);
			if(token!=NULL){
				//temps total
				sum += atoi(token);
 
				if(i==3)
					//temps d'inactivité (le 4e élément de la ligne)
					idle = atoi(token);
 
			i++;
			       }
		    }
		        
	        idleFraction = 100 - (idle-lastIdle)*100.0/(sum-lastSum);
		    printf("Busy for : %f %% of the time.\n", idleFraction);
 
		    lastIdle = idle;
		    lastSum = sum;

		    sleep(lag);
		}	
}
*/

double * checkCPU(int lastIdle, int lastSum) {
	char ligne[100];
	const char d[2] = " ";
	char* token;
	int i = 0;
	int sum = 0, idle;
	double idleFraction;
 
	FILE* fp = fopen("/proc/stat","r");
	i = 0;
	fgets(ligne,100,fp);
	fclose(fp);
	//lire tous les éléments d'une ligne 1 à 1 délimité par ' ' (un esapce)
	token = strtok(ligne,d);
	sum = 0;
	while(token!=NULL){
		//on avance de 1 élément dans la même ligne
		token = strtok(NULL,d);
		if(token!=NULL){
			//temps total
			sum += atoi(token);
 
			if(i==3)
				//temps d'inactivité (le 4e élément de la ligne)
				idle = atoi(token);
 
		i++;
		       }
	    }
		        
	    idleFraction = 100 - (idle-lastIdle)*100.0/(sum-lastSum);
	    //printf("Busy for : %f %% of the time.\n", idleFraction);

	    static double resultat[3];
	    resultat[0] = idleFraction;
	    resultat[1] = idle;
	    resultat[2] = sum;
	    return resultat;		
}

void paramMonitor() {
	FILE* fp = fopen("monitorOption.INI","r");
	char* token;
	char ligne[100];

	fgets(ligne,100,fp);
	fgets(ligne,100,fp);
	fgets(ligne,100,fp);

	fgets(ligne,100,fp);
	token = strtok(ligne,":");
	token = strtok(NULL,":");
	palier1=atoi(token);
	//printf("palier1 :: %d\n",palier1);
	
	fgets(ligne,100,fp);
	token = strtok(ligne,":");
	token = strtok(NULL,":");
	palier2=atoi(token);
	//printf("palier2 :: %d\n",palier2);

	fgets(ligne,100,fp);
	token = strtok(ligne,":");
	token = strtok(NULL,":");
	palier3=atoi(token);
	//printf("palier3 :: %d\n",palier3);

	fgets(ligne,100,fp);
	token = strtok(ligne,":");
	token = strtok(NULL,":");
	delais1=atoi(token);
	//printf("delais1 :: %d\n",delais1);

	fgets(ligne,100,fp);
	token = strtok(ligne,":");
	token = strtok(NULL,":");
	delais2=atoi(token);
	//printf("delais2 :: %d\n",delais2);

	fgets(ligne,100,fp);
	token = strtok(ligne,":");
	token = strtok(NULL,":");
	nbProcessSimult=atoi(token);
	//printf("nbProcessSimult :: %d\n",nbProcessSimult);


	fclose(fp);

	
}

/* return thie index of the task */
int indexOfTask(Monitor *monitor,pid_t pidTask) {
	for(int i=0; i<(monitor->nb_task); i++) {
		if(monitor->tasks[i].pid_number==pidTask) {
			return i;
		}
	}
	return -1;
}


/* remove a specif task */
void supprTask(Monitor* monitor,pid_t pidTask) {

	int index = indexOfTask(monitor,pidTask);
	
    if (index <  (monitor->nb_task) && index >= 0 ) {
   		Task* temp = (Task*)malloc((monitor->nb_task-1)*sizeof(Task));
        //on recopie le tableau avant l'indice
		for(int i = 0;i<index;i++) {
			temp[i]=monitor->tasks[i];

		}

		//on recopie le tableau après l'indice
		for (int i = (index+1) ; i< (monitor->nb_task) ; i++) {
			//printf("hop");
			temp[i-1]=monitor->tasks[i];

		}
		//destroy all param of task at the index
    	free(monitor->tasks[index].param);
    	free(monitor->tasks);

    	monitor->tasks = temp;

    	//refresh the size of the list   
    	monitor->nb_task--;

    }

   
    return;
    
}

//lancer le monitor et toutes ses taches
//pour les processus stop et relancé : FIFO
//J'ai fait le choix de
//se ferme lorsque toutes les taches ont ete execute
int run(Monitor* monitor) {

	int nbProcessusLances=0;
	int nbProcessusFinis=0;
	int tempsPalier1=0;
	int tempsPalier2=0;
	int statusProcess=0;
	int nbProcessus=monitor->nb_task;
	int pidPaused[nbProcessus];
	int pidStarted[nbProcessus];
	int nbProcessusPaused=0;

	int idle = 0;
	int sum = 0;
	double *tab;

	//on enlève le premier car il sert de référence et n'est pas représentatif de la charge cpu
	tab = checkCPU(idle,sum);
	double cpuCharge = tab[0];
	idle = tab[1];
	sum = tab[2];
	sleep(1);
	printf("palier1: %d\n",palier1);
	while(1)
	{
		tab = checkCPU(idle,sum);
		double cpuCharge = tab[0];
		idle = tab[1];
		sum = tab[2];
		printf("cpuCharge : %.2f\n",cpuCharge);
		sleep(1);

		//Si cpuCharge dans le palier1 (entre 0 et palier1)
		if(cpuCharge>=0 && cpuCharge<palier1) {
			tempsPalier1+=1;
			if(tempsPalier1>=delais1) {
				//printf("palier1 atteind depuis au moins %ds : 0 - %d\n",delais1,palier1);
				if(nbProcessusPaused>0 && nbProcessusLances<nbProcessSimult) {
					//relancé le premier processus
					kill(pidPaused[0],SIGCONT);
					for (int i = 0; i < (nbProcessusPaused-1); ++i)
					{
						pidPaused[i]=pidPaused[i+1];
					}
					pidPaused[nbProcessusPaused]=0;
					printf("processus %d relancé\n",pidPaused[0]);
					nbProcessusPaused-=1;
					sleep(1); //pour laisser de calculer la véritable charge cpu
				}else {
					//si il y a encore des processus a lancé
					if(nbProcessusLances<monitor->nb_task && nbProcessusLances<nbProcessSimult) {
						int code_retour = fork_exec(&(monitor->tasks[nbProcessusLances]));
						pidStarted[nbProcessusLances] = monitor->tasks[nbProcessusLances].pid_number;
						printf("processus %d lancé\n",monitor->tasks[nbProcessusLances].pid_number);
						sleep(1); //pour laisser de calculer la véritable charge cpu
						nbProcessusLances++;
						
					}
				}
					
			}
			tempsPalier2=0;
		}

		//Si cpuCharge dans le palier2 (entre palier2 et palier3)
		if(cpuCharge>=palier2 && cpuCharge<palier3) {
			tempsPalier2+=1;
			if(tempsPalier2>=delais2) {
				//on met en pause le dernier processus lancé
				if(nbProcessusLances>0) {
					printf("(1)processus stop : %d\n",pidStarted[nbProcessusLances-1]);
					kill(pidStarted[nbProcessusLances-1],SIGSTOP);
					pidStarted[nbProcessusLances-1]=0;
					nbProcessusLances--;
				}
			}
			tempsPalier1=0;
		}


		//Si cpuCharge au-dessus du palier3
		if(cpuCharge>=palier3) {
			tempsPalier1=0;
			tempsPalier2+=0;
			//printf("palier3 atteind : au-dessus de %d\n",palier3);
			//on met en pause le dernier processus lancé
			if(nbProcessusLances>0) {
				printf("(2)processus stop : %d\n",pidStarted[nbProcessusLances-1]);
				kill(pidStarted[nbProcessusLances-1],SIGSTOP);
				pidStarted[nbProcessusLances-1]=0;
				nbProcessusLances--;
			}
				
		}


		for (int i = 0; i < monitor->nb_task; ++i)
		{
			int w = waitpid(monitor->tasks[i].pid_number,&statusProcess,WNOHANG|WUNTRACED);
			if(w > 0 && WIFEXITED(statusProcess)) {
				printf("+1 process finis :: %d\n",w);
				nbProcessusFinis+=1;
				nbProcessusLances-=1;
			}
			if(w > 0 && WIFSTOPPED(statusProcess)) {
				printf("+1 process stopped :: %d\n",w);
				pidPaused[nbProcessusPaused]=monitor->tasks[i].pid_number;
				nbProcessusPaused+=1;
				//printf("processus paused ajouté : %d\n",pidPaused[nbProcessusPaused-1]);
			}
			if(nbProcessusFinis==monitor->nb_task) {
				exit(0);
			}
		}
		
		

	}

	
	//char status = checkStatusProcess(monitor->tasks[1].pid_number);
	//printf("pid : %d et état : %c\n",monitor->tasks[1].pid_number,status);
	return 1;


}

/*Free task memory*/
void destroy_tasks(Monitor* monitor) {
	for (int i = 0; i < monitor->nb_task-1; i++)
	{	
		if(monitor->tasks[i].param!=NULL) {
			free(monitor->tasks[i].param);
		}
	}
}

/*Free of all allocated memory*/
void destroy_monitor(Monitor* monitor) {
	destroy_tasks(monitor);
	free(monitor->tasks);
	free(monitor);
}



int main(void) {


	Monitor* monitor = init_monitor();
	int* param= (int*)malloc(2*sizeof(int));
	param[0]=1;
	param[1]=2;

	int* param2= (int*)malloc(2*sizeof(int));
	param2[0]=3;
	param2[1]=4;

	int* param3= (int*)malloc(2*sizeof(int));
	param3[0]=5;
	param3[1]=6;

	int* param4= (int*)malloc(2*sizeof(int));
	param3[0]=5;
	param3[1]=6;

	int* param5= (int*)malloc(2*sizeof(int));
	param3[0]=5;
	param3[1]=6;
	//void* param = &tab;

	int* param6= (int*)malloc(2*sizeof(int));
	param3[0]=5;
	param3[1]=6;
	//void* param

	int* param7= (int*)malloc(2*sizeof(int));
	param3[0]=5;
	param3[1]=6;
	//void* param

	int* param8= (int*)malloc(2*sizeof(int));
	param3[0]=5;
	param3[1]=6;
	//void* param

	paramMonitor();

	int code_retour = submit(monitor,*fonction_test,param,stdinFile,stdoutFile,stderrFile);
	int code_retour3 = submit(monitor,*fonction_test,param2,stdinFile,stdoutFile,stderrFile);
	int code_retour4 = submit(monitor,*fonction_test,param3,stdinFile,stdoutFile,stderrFile);
	int code_retour5 = submit(monitor,*fonction_test,param4,stdinFile,stdoutFile,stderrFile);
	int code_retour6 = submit(monitor,*fonction_test,param5,stdinFile,stdoutFile,stderrFile);
	int code_retour7 = submit(monitor,*fonction_test,param5,stdinFile,stdoutFile,stderrFile);
	int code_retour8 = submit(monitor,*fonction_test,param5,stdinFile,stdoutFile,stderrFile);
	int code_retour9 = submit(monitor,*fonction_test,param5,stdinFile,stdoutFile,stderrFile);


	int code_retour2 = run(monitor);


	//supprTask(monitor,monitor->tasks[1].pid_number); //attention avec le destroy_monitor ca peut faire un double free

	//int* tab = monitor->tasks[1].param;
	//int a = (int)tab[0];
	//printf("val : %d\n",a);


	destroy_monitor(monitor);
	//cat /proc/22/status | grep "State" | cut -c8- | cut -d " " -f1


	return 0;
}