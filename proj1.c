#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/times.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sys/sem.h>
#include <string.h>
#include  <errno.h>

#define SIZE 80+1
/*Exponential time: http://stackoverflow.com/questions/2106503/pseudorandom-number-generator-exponential-distribution */

/*Out buffer context*/
struct in{
	pid_t pid;
	int fileNo;
};

/*In buffer context*/
struct out{
	pid_t pid;
	char buf[SIZE];
};

#if 1
union semun{
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};
#endif

/*Initializing semaphores function*/
int seminit(int semid,int val){
	union semun arg;
	arg.val = val;
	return semctl(semid,0,SETVAL,arg);
}
/*Up semaphore function*/
int up(int semid){
	struct sembuf operation;
	operation.sem_flg = 0;
	operation.sem_num = 0;
	operation.sem_op = +1;
	return semop(semid,&operation,1);
}
/*Down semaphore function*/
int down(int semid){
	struct sembuf operation;
	operation.sem_flg = 0;
	operation.sem_num = 0;
	operation.sem_op = -1;
	return semop(semid,&operation,1);
}
/*Destroy semphore function*/
int semdestroySet(int semid){
	return semctl(semid,0,IPC_RMID, 0);
}

int main(int argc, char *argv[]){
	int n, i, j, fileNo, k;
	char message[SIZE];
	char filename[sizeof(int)+1];
	double num, exp_time, l;
	struct in *in;
	struct out *out;
	struct tms tb1, tb2;
	double request_t, respond_t;
	int shmin_id, shmout_id;
	int sem_in, sem_in2, sem_out, sem_unique;
	pid_t pid, tmpid, Cpid;
	FILE *fp;
	FILE *logfp;
	
	
	if(argc != 4){
		printf("Incorrect number of attributes!\n");
		exit(1);
	}
	/*number of C'/S' clones*/
	n = atoi(argv[1]);
	/*exponential parameter*/
	l = atof(argv[2]);
	/*Number of txt files we have*/
	k = atoi(argv[3]);
	if(n < 1){
		printf("Incorrect number of attributes!\n");
		exit(2);
	}
	if(l < 1){
		printf("L parameter should be a positive number!\n");
		exit(8);
	}
	if(k < 1){
		printf("We should have at least 1 txt file to read from!\n");
		exit(9);
	}		 
	printf("=> No of clones %d\n",n);
	memset(filename,0,sizeof(filename));
	memset(message,0,sizeof(message));
	
	shmin_id = shmget(IPC_PRIVATE,sizeof(struct in),0600|IPC_CREAT);
	if(shmin_id < 0)
		perror("shmget error: ");
	shmout_id = shmget(IPC_PRIVATE,sizeof(struct out),0600|IPC_CREAT);
	if(shmout_id < 0)
		perror("shmget error: ");
	
	in = (struct in *)shmat(shmin_id,NULL,0);
	out = (struct out *)shmat(shmout_id,NULL,0);
	if(in == (void *)-1 || out == (void *)-1)
		perror("shmat error: ");
	
	memset(message,'\0',sizeof(message));
	memset(filename,'\0',sizeof(filename));
	
	if((sem_in = semget((key_t) 54815645,1,0600|IPC_CREAT)) == -1){
		perror("semget error: ");
		exit(EXIT_FAILURE);
	}
	if((sem_in2 = semget((key_t) 95481974,1,0600|IPC_CREAT)) == -1){
		perror("semget error: ");
		exit(EXIT_FAILURE);
	}
	
	if((sem_out = semget((key_t) 18748457,1,0600|IPC_CREAT)) == -1){
		perror("semget error: ");
		exit(EXIT_FAILURE);
	}
	
	if(seminit(sem_in,1) < 0)
		perror("semclt error: ");
	if(seminit(sem_in2,0) < 0)
		perror("semclt error: ");
	if(seminit(sem_out,1) < 0)
		perror("semclt error: ");
		
	srand(time(NULL));
	
	if((pid = fork()) < 0){
		exit(EXIT_FAILURE);
	}
	/*Process C*/
	else if(pid){														
		printf("Process C was born with pid %d\n", getpid());
		for(i=0;i<n;i++){
			/*exponential time computation*/
			num = (double)rand()/(double)RAND_MAX;
			exp_time = (-1.0/l) * log((double)1 - num);
			//printf("Exp_time: %f\n", exp_time);
			//sleep(exp_time);
			/*sleep for exponential time till next C' is born*/
			for(j=0;j<exp_time+1000;j++);									
			if((tmpid = fork()) < 0){
				printf("PROBLEM\n");
				exit(3);
			}
			if(!tmpid){
				/*create a unique semaphore visible between on C' and S'*/
				sem_unique = semget((key_t)getpid(),1,0600|IPC_CREAT);
				//printf("Unique semaphores key: %d\n", sem_unique);
				seminit(sem_unique,0);
				
				//printf("SemIn value in C': %d\n",semctl(sem_in,0,GETVAL));
				printf("%s\n", strerror(errno));
				/*put new data in in buffer when S' finish reading from it*/
				down(sem_in);
				/*chose a fileNo*/
				fileNo = rand()%k+1;
				printf("------fileNo: %d\n", fileNo);
				/*Real start request time*/
				request_t = (double) times(&tb1);
				/*put in in buffer the fileNo and the pid of C'*/
				in->fileNo = fileNo;
				in->pid = getpid();
				/*now S can make new S'*/
				up(sem_in2);
				/*stop till S' finish putting new data to in buffer*/
				down(sem_unique);
				/*read respond from out*/
				respond_t = (double) times(&tb2);
				strncpy(message,out->buf,SIZE-1);
				printf("C' got context from S': %s\n",message);
				/*S' can write to out buffer*/
				up(sem_out);
				/*write in LOGfile*/
				logfp = fopen("log","a");
				fprintf(logfp,"In log file: request time %f, respond time %f, context: %s\n",request_t, respond_t, message);
				/*flush logfp stream*/
				fflush(logfp);
				
				/*destroy unique semaphore*/
				semdestroySet(sem_unique);
				/*close logfp*/
				fclose(logfp);
				
				exit(4);
			}
		}
		/*Process C*/
		/*waiting for all C'(n) and S to finish*/
		for(i=0;i<n;i++)
			wait(NULL);
		wait(NULL);
		printf("C is over with pid %d\n", getpid());
		
		/*destroy the sempahore set*/
		semdestroySet(sem_in);
		semdestroySet(sem_in2);
		semdestroySet(sem_out);
		
		/*detach-destroy memory set*/
		shmdt(in);
		shmdt(out);
		shmctl(shmin_id,IPC_RMID,(struct shmid_ds *)0);
		shmctl(shmout_id,IPC_RMID,(struct shmid_ds *)0);
		
		exit(5);
	}
	/*Process S*/
	else{																
		printf("Process S was born with pid %d\n", getpid());
		for(i=0;i<n;i++){
			/*block until request comes from C'*/
			down(sem_in2);
			if((tmpid = fork()) < 0){
				printf("Problem\n");
				exit(EXIT_FAILURE);
			}
			if(!tmpid){
				/*S' process*/
				printf("S' was born with tmpid %d\n", getpid());
				/*S' gets C's pid*/
				Cpid = in->pid;		
				/*fileNo of C' file*/
				fileNo = in->fileNo;										
				//printf("=> Got fileNo %d (Cpid = %d)\n",fileNo,Cpid);
				/*wake up C' so it can put new data in in buffer*/
				up(sem_in);
				/*get unique semaphore id*/
				sem_unique = semget((key_t)Cpid,1,0);
				//printf("Unique semaphores key: %d\n", sem_unique);
				/*open a random file*/
				sprintf(filename,"%d",fileNo);
				fp = fopen(filename,"r");
				/*S' cant write in out buffer till C' writes in log*/
				down(sem_out);
				/*write to out buffer*/
				fgets(out->buf,sizeof(out->buf)-1,fp);
				printf("S' sent context to C' : %s\n",out->buf);
				/*now C' can read from out buffer*/
				up(sem_unique);
				
				fclose(fp);
			
				exit(6);
			}
		}
		/*Process S*/
		/*waiting for all S'(n) to finish*/
		for(i=0;i<n;i++)
			wait(NULL);
		printf("S is over with pid %d\n", getpid());
		
		/*detach in and out memories*/
		shmdt(in);
		shmdt(out);
		exit(7);
	}
	
	/*never reached area*/
	return 0;
}
