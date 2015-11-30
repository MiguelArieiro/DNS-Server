#include "global.h"

/*
    Trabalho realizado por: 
    
    Miguel Pocinho Arieiro 2014197166
    Pedro Miguel Ferreira Caseiro 2014197267

    Tempo total: 40 horas

*/

//terminar config
void sigINTconfig(int sig){

        #ifdef DEBUG
        printf("sigINTconfig\n");
        #endif

        exit(0);
}

//Função para leitura do ficheiro de configurações. É usada a estrutura Config
void config_read(CONFIG shared_var){

        int endman=0;
        int k = 0;
        char temp[256];
        shared_var->number_whitelisted_domains=0;
        #ifdef DEBUG
        printf("Loading config\n");
        #endif
        FILE *f = fopen("config.txt", "r");

        //reading the number of threads
        fscanf(f, "Threads = %d\n;", &(shared_var->n_threads));
        //just moving the scanner
        fscanf(f, "Domains = ");

        fgets(temp,256,f);
        //reading the whitelisted domains
        char* token = strtok(temp, "; ");
        while((token!=NULL)){
                strcpy(shared_var->domains[shared_var->number_whitelisted_domains],token);
                token = strtok(NULL, "; ");
                (shared_var->number_whitelisted_domains)++;
        }
        while(shared_var->domains[shared_var->number_whitelisted_domains-1][k] != '\n'){
                k++;
        }
        shared_var->domains[(shared_var->number_whitelisted_domains)-1][k]='\0';
        fscanf(f, "LocalDomain = %[^\n]\n", shared_var->local_domain);
        fscanf(f, "NamedPipeEstatisticas = %[^\n]", shared_var->Named_Pipe_Statistics);

        fclose(f);

        #ifdef DEBUG
        printf("Config loaded\n");
        #endif

        sem_post(sem_mutex);

        //Prints de teste!
        #ifdef DEBUG
        k=0;
        printf("Número de Threads: %d\n", shared_var->n_threads);
        printf("Domínios autorizados: (%d)\n",shared_var->number_whitelisted_domains);
        while (k<shared_var->number_whitelisted_domains){
                printf("\t%s\n", shared_var->domains[k]);
                k++;
        }
        printf("Domínio local: %s\n", shared_var->local_domain);
        printf("Nome do pipe de estatísticas: %s\n", shared_var->Named_Pipe_Statistics);
        #endif

        //manutenção
        while(1){
                if (shared_var->manutencao==1){//checking maintenance status
                        endman=1;
                }else if (endman==1){//checking maintenance status
                        #ifdef DEBUG
                        printf("Loading config\n");
                        #endif
                        sem_wait(sem_mutex);
                        shared_var->number_whitelisted_domains=0;
                        FILE *f = fopen("config.txt", "r");

                        //just moving the scanner
                        fscanf(f, "Threads = %*d\nDomains = ");

                        fgets(temp,256,f);
                        //reading the whitelisted domains
                        char* token = strtok(temp, "; ");
                        while(token!=NULL){
                                strcpy(shared_var->domains[shared_var->number_whitelisted_domains],token);
                                token = strtok(NULL, "; ");
                                (shared_var->number_whitelisted_domains)++;
                        }
                        k=0;
                        while(shared_var->domains[(shared_var->number_whitelisted_domains)-1][k] != '\n'){
                                k++;
                        }
                        shared_var->domains[(shared_var->number_whitelisted_domains)-1][k]='\0';
                        fscanf(f, "LocalDomain = %[^\n]\n", shared_var->local_domain);

                        fclose(f);

                        #ifdef DEBUG
                        printf("Config loaded\n");
                        #endif
                        endman=0;
                        shared_var->manutencao=0;
                        sem_post(sem_mutex);

                        //Prints de teste!
                        #ifdef DEBUG
                        k=0;
                        printf("Número de Threads: %d\n", shared_var->n_threads);
                        printf("Domínios autorizados: (%d)\n",shared_var->number_whitelisted_domains);
                        while (k<shared_var->number_whitelisted_domains){
                                printf("\t%s\n", shared_var->domains[k]);
                                k++;
                        }
                        printf("Domínio local: %s\n", shared_var->local_domain);
                        printf("Nome do pipe de estatísticas: %s\n", shared_var->Named_Pipe_Statistics);
                        #endif

                }
                sleep (1);
        }
}




//Inicializaçao através do fork
void config_start(CONFIG shared_var){
        signal(SIGUSR2, sigINTconfig);
        signal(SIGUSR1,sigUSR1);
        config_read(shared_var);
}
