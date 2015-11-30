#ifndef GLOBAL_H
#define GLOBAL_H

/*
    Trabalho realizado por: 
    
    Miguel Pocinho Arieiro 2014197166
    Pedro Miguel Ferreira Caseiro 2014197267

    Tempo total: 40 horas

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>

#define MAX_BUF 1024
#define DEBUG

sem_t *sem_emptyl; //0 se empty
sem_t *sem_emptye; //0 se empty
sem_t *sem_mutex;  //shared memory access

int shmid; //shared memory ID

typedef struct config *CONFIG;
//struct for the config
typedef struct config{
    int manutencao;
    int n_threads; //número de threads
    char local_domain[32]; //dominio local
    char Named_Pipe_Statistics[100]; //pipe de estatisticas
    int number_whitelisted_domains;
    char domains[][32]; //todos os dominios autorizados são colocados num array dinâmico
}Config;

//Estrutura para data e hora
typedef struct data_hora{
	int dia, mes, ano, hora, min, sec;
}Data_Hora;

//DNS header structure
struct DNS_HEADER{
    unsigned short id; // identification number

    unsigned char rd :1; // recursion desired
    unsigned char tc :1; // truncated message
    unsigned char aa :1; // authoritive answer
    unsigned char opcode :4; // purpose of message
    unsigned char qr :1; // query/response flag

    unsigned char rcode :4; // response code
    unsigned char cd :1; // checking disabled
    unsigned char ad :1; // authenticated data
    unsigned char z :1; // its z! reserved
    unsigned char ra :1; // recursion available

    unsigned short q_count; // number of question entries
    unsigned short ans_count; // number of answer entries
    unsigned short auth_count; // number of authority entries
    unsigned short add_count; // number of resource entries
};

//Constant sized fields of query structure
struct QUESTION{
    unsigned short qtype;
    unsigned short qclass;
};

//Constant sized fields of the resource record structure
#pragma pack(push, 1)
struct R_DATA{
    unsigned short type;
    unsigned short _class;
    unsigned int ttl;
    unsigned short data_len;
};
#pragma pack(pop)

//Pointers to resource record contents
struct RES_RECORD{
    unsigned char *name;
    struct R_DATA *resource;
    unsigned char *rdata;
};

//Structure of a Query
struct QUERY{
    unsigned char *name;
    struct QUESTION *ques;
};

typedef struct dados{
    unsigned short id; // identification number
    unsigned char *query;
    short tipo;
    struct sockaddr_in dest;
    struct dados* next;
}Dados;

typedef struct ldomains{
    char dominio[32];
    char ip[16];
    struct ldomains* next;
}lDomains;

void convertName2RFC (unsigned char*,unsigned char*);
unsigned char* convertRFC2Name (unsigned char*,unsigned char*,int*);
void sendReply(unsigned short, unsigned char*, int, int, struct sockaddr_in);


void main_init_semaphores(void);
void main_init_config(CONFIG);
void main_init_stats(CONFIG);
void mmap_localdns(void);

void* worker(void*);

int addDados (Dados*, unsigned short, unsigned char*, struct sockaddr_in, short);
int delDados (Dados*);
//Dados* removeFirstDados (Dados*);
Dados* getRequest (Dados*);
int delDomains(lDomains*);
lDomains* newDomains(lDomains*);

Data_Hora data_sistema(void);
void sigAlarm (int);
void sigINT (int);
void sigINTconfig (int);
void sigINTstats (int);
void sigUSR1 (int);
void sigUSR1main (int);
void threadSIGINT (int);

void cleanup(void);

void config_read (CONFIG);
void config_start (CONFIG);

void stats (CONFIG);

#endif /*GLOBAL_H*/
