#include "global.h"

/*
    Trabalho realizado por: 

    Miguel Pocinho Arieiro 2014197166
    Pedro Miguel Ferreira Caseiro 2014197267

    Tempo total: 40 horas
*/
 
CONFIG shared_var; //ponteiro para a estrutura Config

pid_t stats_process;
pid_t config_process;

pthread_t* ptr_threads;
off_t stsize;

char* data;

int estatisticas; //pipe writer

int sockfd;

//number of total denied requests, local + external
int total_denied_requests = 0;
//number of local requests
int local_requests = 0;
//number of external requests
int external_requests = 0;


Data_Hora arranque, ultima_modificacao;

//listas de pedidos
Dados* ptr_locais;
Dados* ptr_externos;
//localdns.txt domain/ip list
lDomains* ptr_domains;

//threads

void threadSIGINT(int sig){
   	pthread_exit((void *)NULL);
}

void* worker(void* idp){

    signal(SIGUSR2,threadSIGINT);

	lDomains* tempdomains=ptr_domains;
	FILE* pf;
	char tempstring [2048];
	#ifdef DEBUG
	int my_id=*((int*)idp);
    printf("Thread %d running\n",my_id);
    #endif
    while(1){
    	Dados* ptr_temp=NULL;
    	if (sem_trywait (sem_emptyl)==0){
	    	sem_wait (sem_mutex);
	    	ptr_temp=getRequest(ptr_locais);
	    	if (ptr_temp==NULL){
	    		perror("Error getting request (worker)\n");
	    		exit(1);
	    	}
	    	sem_post (sem_mutex);
    		while (tempdomains->next!=NULL){
    			tempdomains=tempdomains->next;
    			if (strcmp(tempdomains->dominio,(char*)ptr_temp->query)==0){
					#ifdef DEBUG
					printf("Internal ip (file): %s\n",tempdomains->ip);
					#endif
		            local_requests++;
    				sendReply(ptr_temp->id, ptr_temp->query, inet_addr(tempdomains->ip), sockfd, ptr_temp->dest);
    				break;
    			}else if (tempdomains->next==NULL){
    				sendReply(ptr_temp->id, ptr_temp->query, inet_addr("0.0.0.0"), sockfd, ptr_temp->dest);
    				total_denied_requests++;
    			}
    		}
    		tempdomains=ptr_domains;
	    	free(ptr_temp);
    	}else if ((shared_var->manutencao==0)&&(sem_trywait(sem_emptye)==0)){
    		sem_wait (sem_mutex);
            external_requests++;
	    	ptr_temp=getRequest(ptr_externos);
	    	if (ptr_temp==NULL){
	    		perror("Error getting request (worker)\n");
	    		exit(1);
	    	}
	    	sem_post (sem_mutex);
    		sprintf (tempstring,"dig %s A +short | tail -n1 ",(char*) ptr_temp->query);
    		pf = popen(tempstring,"r");
    		if(!pf){
    			perror("Could not open pipe for output.\n");
    			exit(1);
    		}
			// Grab data from process execution
			if(fscanf(pf,"%[^\n]",tempstring)!=1){
				sendReply(ptr_temp->id, ptr_temp->query, inet_addr("0.0.0.0"), sockfd, ptr_temp->dest);
			}else{
				#ifdef DEBUG
				printf("External ip received (dig): %s\n",tempstring);
				#endif
				sendReply(ptr_temp->id, ptr_temp->query, inet_addr(tempstring), sockfd, ptr_temp->dest);
			}
			if (pclose(pf) != 0){
				perror(" Error: Failed to close command stream \n");
				exit(1);
			}
	    	free(ptr_temp);
    	}else {
    		sleep(1);
    	}
    }
}

//inicializar semaforos
void main_init_semaphores(){

	sem_unlink("emptyl");
	sem_emptyl = sem_open("emptyl", O_CREAT | O_EXCL, 0700, 0);

	sem_unlink("emptye");
	sem_emptye = sem_open("emptye", O_CREAT | O_EXCL, 0700, 0);

	sem_unlink("mutex");
	sem_mutex = sem_open("mutex", O_CREAT | O_EXCL, 0700, 0);
}


 
//buscar data e hora do sistema
Data_Hora data_sistema(){

    Data_Hora data;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    data.dia = t->tm_mday;
    data.mes = t->tm_mon+1;
    data.ano = t->tm_year+1900;
    data.hora = t->tm_hour;
    data.min = t->tm_min;
    data.sec = t->tm_sec;
   
    return data;
}
 
 
// fork para iniciar o gestor de configurações
void main_init_config(CONFIG shared_var){

    config_process = fork();

    if (config_process == -1) {
    	fprintf(stderr, "Fork error creating config process.\n");
    } else if (config_process == 0) {
		printf("\n\nA iniciar Gestor de Configurações (%d)\n\n",getpid());
	    config_start(shared_var);
	    exit(1);
    }
}
 
 
// fork para iniciar o gestor de estatísticas
void main_init_stats(CONFIG shared_var) {

    stats_process = fork();

    if (stats_process == -1) {     
    	fprintf(stderr, "Fork error creating stats process.\n");
    } else if (stats_process == 0) {
		printf("\n\nA iniciar Gestor de Estatísticas (%d)\n\n",getpid());
        stats(shared_var);
        exit(1);
    }
}
 
/* MAPEAR O LOCALDNS.TXT PARA MEMÓRIA PARTILHADA */
 
void mmap_localdns(){

    int ldns;
    char *temp;
    struct stat size;
    lDomains* ptr_temp=NULL;
    ldns=open("localdns.txt", O_RDONLY);
    stat("localdns.txt", &size);
    data = mmap((caddr_t)0, size.st_size, PROT_WRITE, MAP_PRIVATE, ldns, 0);
    close(ldns);
    stsize=size.st_size;
    temp = data;
    char *pch;
    pch = strtok(data, " ");

    while(pch != NULL){
    	ptr_temp=newDomains(ptr_domains);
    	if (ptr_temp==NULL) {
    		perror("Error adding domains (mmap)");
    		exit(1);
    	}
    	#ifdef DEBUG
    	printf("Adding new domain to domain list:\n");
    	#endif
        strcpy(ptr_temp->dominio,pch);
        pch = strtok(NULL, "\n");
        strcpy(ptr_temp->ip,pch);
        pch = strtok(NULL, " ");

        #ifdef DEBUG
        printf("%s\t%s\n",ptr_temp->dominio,ptr_temp->ip);
        #endif
    }
    data = temp;
}

//stats
void sigAlarm(int sig) {
	#ifdef DEBUG
	printf("sigAlarm\n");
	#endif

    char buf[MAX_BUF];
    int total_requests=local_requests+external_requests+total_denied_requests;
    sprintf(buf,"\nHora de arranque: %d:%d:%d\t%d/%d/%d\nTotal requests: %d\nTotal denied requests: %d\nLocal requests: %d\nexternal requests:%d\nHora da última modificação: %d:%d:%d\t%d/%d/%d\n",arranque.hora,arranque.min,arranque.sec,arranque.dia,arranque.mes,arranque.ano,total_requests,total_denied_requests,local_requests,external_requests,ultima_modificacao.hora,ultima_modificacao.min,ultima_modificacao.sec,ultima_modificacao.dia,ultima_modificacao.mes,ultima_modificacao.ano);
    write(estatisticas,buf,MAX_BUF);
    alarm(30);
}

//terminar main
void sigINT(int sig) {
	#ifdef DEBUG
	printf("sigĨNT\n");
	#endif

	cleanup();
    exit(0);
}

//manutenção
void sigUSR1(int sig){
    #ifdef DEBUG
    printf("sigUSR1\n");
    #endif

    if(shared_var->manutencao==0){
        shared_var->manutencao=1;
        printf("Modo de manutenção iniciado\n");
    } else if (shared_var->manutencao==1){
        shared_var->manutencao=2;
        printf("Modo de manutenção concluído\n");
    }
}

//redirecionar SIGUSR1 para config
void sigUSR1main (int sig){
	#ifdef DEBUG
	printf("sigUSR1main\n");
	#endif
    kill(config_process,SIGUSR1);
}

void cleanup (){

	#ifdef DEBUG
	printf("cleanup initialized\n");
	#endif

	//child processes' termination
    kill(config_process,SIGUSR2);
    kill(stats_process,SIGUSR2);

	int i;
	for(i=0; i < shared_var->n_threads; i++){
		pthread_kill(ptr_threads[i], SIGUSR2);
		#ifdef DEBUG
		printf("SIGUSR2 sent to thread %u\n",(unsigned int)ptr_threads[i]);
		#endif
	}

    //Esperar pelas threads
    for(i=0; i < shared_var->n_threads; i++){
        if (pthread_join(ptr_threads[i], NULL)!=0){
        	perror("pthread_join error\n");
        	exit(1);
        } else{
        	#ifdef DEBUG
			printf("Thread %u terminated\n",(unsigned int)ptr_threads[i]);
			#endif
        }
    }

    #ifdef DEBUG
   	printf("Threads something\n");
   	#endif

    //Esperar pelos processos
    while(wait(NULL) != -1){
    	#ifdef DEBUG
		printf("Child process terminated\n");
		#endif
    }

	//semaphores
	sem_unlink("emptyl");
	sem_unlink("emptye");
	sem_unlink("mutex");

	#ifdef DEBUG
	printf("Semaphores unlinked\n");
	#endif
	unlink (shared_var->Named_Pipe_Statistics);//fifo

	#ifdef DEBUG
	printf("FIFO unlinked\n");
	#endif

	//del lists
    delDados(ptr_locais);
    delDados(ptr_externos);
    delDomains(ptr_domains);

    #ifdef DEBUG
	printf("Lists deleted\n");
	#endif

    munmap(data, stsize);

    #ifdef DEBUG
	printf("Memory file unmapped\n");
	#endif

	shmctl(shmid, IPC_RMID, NULL);

	#ifdef DEBUG
	printf("Shared memory deleted\n");
	#endif
	printf ("\nDNSSERVER terminado\n");
}


int main( int argc , char *argv[]){
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGINT);
    sigaddset(&mask,SIGALRM);
    sigprocmask(SIG_BLOCK,&mask,NULL);
    signal(SIGINT,sigINT);
    signal(SIGUSR1,sigUSR1main);
    signal(SIGALRM,sigAlarm);
    alarm(30);

    arranque = data_sistema();
    ultima_modificacao=arranque;

	unsigned char buf[65536], *reader;
	int stop;
	struct DNS_HEADER *dns = NULL;
	
	struct sockaddr_in servaddr,dest;
	socklen_t len;

    int port=9000;

    printf("\n\nA iniciar DNSSERVER (%d)\n\n",getpid());

	main_init_semaphores();

	#ifdef DEBUG
    printf("Initializing semaphores\n");
    #endif

    #ifdef DEBUG
    printf("Creating domain lists\n");
    #endif

    ptr_locais=(Dados*) malloc (sizeof(Dados));
    ptr_locais->next=NULL;
    ptr_externos=(Dados*) malloc (sizeof(Dados));
    ptr_externos->next=NULL;
    ptr_domains=(lDomains*) malloc (sizeof(lDomains));
    ptr_domains->next=NULL;


    // create shared memory for the config
    // shmid it's the id of a shared memory
    // shgmet obtains an identifier to an existing shared memory or creates a new one. 0766 is for read only purposes

    #ifdef DEBUG
    printf("Creating shared memory\n");
    #endif

    if ((shmid=shmget(IPC_PRIVATE, sizeof(Config), IPC_CREAT | 0777))<0){
        perror("Error in shmget with IPC_CREAT\n"); // erro caso o id < 0
        exit(1);
    }

    //attach shared memory
    // shmat maps a certain shared memory region into the current process address space
    shared_var=(CONFIG)shmat(shmid,NULL,0);

    if (shared_var==(Config *)-1){ // if (shared_var = shmat(id)) == - 1, erro. ISTO PODE TER ERRO!!
            perror("Shmat error!");
            exit(1);
    }else{
        //inicializar o gestor de configurações, através do fork
        shared_var->manutencao=0;
        main_init_config(shared_var);
    }
    sem_wait(sem_mutex);

    if (shared_var->n_threads<1){
    	perror("Error: number of threads <1\n");
    	exit(1);
    }

    pthread_t my_thread[shared_var->n_threads];
    ptr_threads=&my_thread[0];

    #ifdef DEBUG
    printf("Creating fifo\n");
    #endif

 	unlink (shared_var->Named_Pipe_Statistics);
    if ((mkfifo(shared_var->Named_Pipe_Statistics, O_CREAT|O_EXCL|0600)<0) && (errno!= EEXIST))  {
        perror("Cannot create pipe: ");
        exit(1);
    }
    sem_post(sem_mutex);

    /* INICIALIZAÇAO DO GESTOR DE ESTATISTICAS */
	
    main_init_stats(shared_var);

    sleep(1);

    #ifdef DEBUG
    printf("A mapear locadns.txt em memória...\n");
    #endif
    mmap_localdns();

    #ifdef DEBUG
    printf("Opening fifo (writter) (main)\n");
    #endif
    if ((estatisticas = open(shared_var->Named_Pipe_Statistics, O_WRONLY)) < 0) {
        perror("Cannot open pipe for writing: ");
        exit(1);
    }
 
    char temp_buf[MAX_BUF];
	sprintf(temp_buf,"\nHora de arranque: %d:%d:%d\t%d/%d/%d\n",arranque.hora,arranque.min,arranque.sec,arranque.dia,arranque.mes,arranque.ano);
    write(estatisticas,temp_buf,MAX_BUF);
 
  	//pool de threads
    //criar shared_var->n_threads threads. valor lido do config.txt
 	#ifdef DEBUG
    printf("Creating threads\n");
    #endif

    int id[shared_var->n_threads];
    int i;
    for(i = 0; i < shared_var->n_threads; i++){
        id[i] = i;
        if (pthread_create(&my_thread[i], NULL, worker, &(id[i]))!=0){
        	perror("Error creating thread\n");
        	exit(1);
        }
    }

    /*SINAIS*/
    sigprocmask(SIG_UNBLOCK,&mask,NULL);

    // Check arguments
    if((argc >= 2)) {
        port = atoi(argv[1]);
        if(port <= 0) {
            port=9000;
        }
    }

 
    // ****************************************
    // Create socket & bind
    // ****************************************

    // Create UDP socket
    sockfd = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP); //UDP packet for DNS queries
 
    if (sockfd < 0) {
    	printf("ERROR opening socket.\n");
        cleanup();
    	exit(1);
    }

    // Prepare UDP to bind port
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_port=htons(port);

    // Bind application to UDP port
    #ifdef DEBUG
    printf("Binding application to UDP port\n");
    #endif

    int res = bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

    if(res < 0) {
    	printf("Error binding to port %d.\n", servaddr.sin_port);
		if(servaddr.sin_port <= 1024) {
	        printf("To use ports below 1024 you may need additional permitions. Try to use a port higher than 1024.\n");
	    } else {
	        printf("Please make sure this UDP port is not being used.\n");
	    }
        cleanup();
    	exit(1);
    }

    // ****************************************
    // Receive questions
    // ****************************************
    #ifdef DEBUG
    printf("Receiving questions\n");
    #endif
    while(1) {

        // Receive questions
        len = sizeof(dest);
        printf("\n\n-- Wating for DNS message --\n\n");

        if(recvfrom (sockfd,(char*)buf , 65536 , 0 , (struct sockaddr*)&dest , &len) < 0) {
                printf("Error while waiting for DNS message. Exiting...\n");
                cleanup();
                exit(1);
        }

        printf("DNS message received\n");

        // Process received message
        dns = (struct DNS_HEADER*) buf;
        //qname =(unsigned char*)&buf[sizeof(struct DNS_HEADER)];
        reader = &buf[sizeof(struct DNS_HEADER)];

        printf("\nThe query %d contains: ", ntohs(dns->id));
        printf("\n %d Questions.",ntohs(dns->q_count));
        printf("\n %d Answers.",ntohs(dns->ans_count));
        printf("\n %d Authoritative Servers.",ntohs(dns->auth_count));
        printf("\n %d Additional records.\n\n",ntohs(dns->add_count));

        // We only need to process the questions
        // We only process DNS messages with one question
        // Get the query fields according to the RFC specification
        struct QUERY query;
        if(ntohs(dns->q_count) == 1) {
            // Get NAME
            query.name = convertRFC2Name(reader,buf,&stop);
            reader = reader + stop;

            // Get QUESTION structure
            query.ques = (struct QUESTION*)(reader);
            reader = reader + sizeof(struct QUESTION);

            // Check question type. We only need to process A records.
            if(ntohs(query.ques->qtype) == 1) {
                printf("A record request.\n\n");
            } else {
                printf("NOT A record request!! Ignoring DNS message!\n");
                total_denied_requests++;
                continue;
            }

        } else {
            printf("\n\nDNS message must contain one question!! Ignoring DNS message!\n\n");
            total_denied_requests++;
            continue;
        }

        // Received DNS message fulfills all requirements.


        // ****************************************
        // Print received DNS message QUERY
        // ****************************************
        printf(">> QUERY: %s\n", query.name);
        printf(">> Type (A): %d\n", ntohs(query.ques->qtype));
        printf(">> Class (IN): %d\n\n", ntohs(query.ques->qclass));

        // ****************************************
        // Example reply to the received QUERY
        // (Currently replying 10.0.0.2 to all QUERY names)
        // ****************************************

        //verificar se o pedido é local
       
        if(strstr((const char*)query.name, (shared_var->local_domain)) != NULL){
        	#ifdef DEBUG
            printf("A processar pedido local\n");
            #endif
            ultima_modificacao=data_sistema();
            sem_wait(sem_mutex);
            if (addDados(ptr_locais, dns->id, query.name, dest,1)==0){
            	printf("Erro addDados\n");
            	exit(1);
            }
            sem_post(sem_emptyl);
            sem_post(sem_mutex);
        }else{
        	#ifdef DEBUG
            printf("A processar pedido externo\n");
            #endif
            if (shared_var->manutencao!=0){
            	printf("Modo de manutenção - pedido externo negado\n");
            	sendReply(dns->id, query.name, inet_addr("0.0.0.0"), sockfd, dest);
            	total_denied_requests++;
            	continue;
            }
            for(i=0; i < (shared_var->number_whitelisted_domains); i++){
                if(strstr((const char*)query.name, shared_var->domains[i]) != NULL){
                    printf("pedido externo whitelisted\n");
                    ultima_modificacao=data_sistema();
                    sem_wait(sem_mutex);
                    if (addDados(ptr_externos, dns->id, query.name, dest,2)==0){
		            	printf("Erro addDados\n");
		            	exit(1);
		            }
                    sem_post(sem_emptye);
            		sem_post(sem_mutex);
                   	break;
                }else if (i==(shared_var->number_whitelisted_domains)-1){
                	sendReply(dns->id, query.name, inet_addr("0.0.0.0"), sockfd, dest);
                	total_denied_requests++;
                }
            }
        }
    }
    return 0;
}
