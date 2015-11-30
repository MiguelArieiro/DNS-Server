#include "global.h"

/*
    Trabalho realizado por: 
    
    Miguel Pocinho Arieiro 2014197166
    Pedro Miguel Ferreira Caseiro 2014197267

    Tempo total: 40 horas

*/


int fd;

//terminar stats
void sigINTstats(int sig){

	#ifdef DEBUG
	printf("sigINTstats\n");
	#endif

    close (fd);

    exit(0);
}


// Leitura do que está no pipe para o ecrã
void stats(CONFIG shared_var){ //sem_t *full
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGUSR2);
    sigprocmask(SIG_BLOCK,&mask,NULL);
    signal(SIGUSR2,sigINTstats);
    char buf[MAX_BUF];

    #ifdef DEBUG
    printf("Opening fifo (reader) (stats process)\n");
    #endif
    if ((fd = open(shared_var->Named_Pipe_Statistics, O_RDONLY)) < 0) {
    	perror("Cannot open pipe for reading: ");
    	exit(1);
    }
    #ifdef DEBUG
    printf("Gestor de estatísticas iniciado...\n");
    #endif
    signal(SIGUSR2, sigINTstats);
    sem_post(sem_mutex);

    sigprocmask(SIG_UNBLOCK,&mask,NULL);
    //reading and printing
    while(1){
        read(fd, buf, MAX_BUF);
        printf("\n%s\n", buf);
    }
    close(fd);
}
