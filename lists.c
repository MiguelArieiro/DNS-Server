#include "global.h"

/*
    Trabalho realizado por: 
    
    Miguel Pocinho Arieiro 2014197166
    Pedro Miguel Ferreira Caseiro 2014197267

    Tempo total: 40 horas

*/

//deletes domains' list
int delDomains(lDomains* domains){

    lDomains* temp;
    if (domains!=NULL){
        while (domains->next!=NULL){
            temp=domains;
            domains=domains->next;
            free(temp);
        }
        return 1;
    }
    else return 0;
}

//adds a new element to the end of domains and returns it
lDomains* newDomains(lDomains* domains){

    if ((domains!=NULL)){
        while(domains->next!=NULL){
            domains=domains->next;
        }
        domains->next=(lDomains*) malloc (sizeof(lDomains));
        domains=domains->next;
        domains->next=NULL;
    	return domains;
    }
    else return NULL;
}

//deletes data;
int delDados (Dados* data){

    Dados* temp;
    if (data!=NULL){
        while (data->next!=NULL){
            temp=data;
            data=data->next;
            free(temp);
        }
        return 1;
    }
    else return 0;
}

 
//adds a new element containing query and dest to data;
int addDados (Dados* data, unsigned short id, unsigned char *query, struct sockaddr_in dest, short tipo){

    if ((data!=NULL)&&(query!=NULL)){
        while(data->next!=NULL){
            data=data->next;
        }
        data->next=(Dados*) malloc (sizeof(Dados));
        data=data->next;
        data->query = query;
        data->dest = dest;
        data->id = id;
        data->tipo = tipo;
        data->next=NULL;
    	return 1;
    }
    else return 0;
}
 
//removes and returns the first element of data; if data is empty returns NULL;
Dados* getRequest (Dados* data){

    Dados* temp=NULL;
    if ((data!=NULL)&&(data->next!=NULL)){
        temp=data->next;
        data->next=temp->next;
        temp->next=NULL;
    }
    return temp;
}
