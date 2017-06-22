/*
	Server that will manage. whatsapp application

	Author: Felipe Angnes
	felipe.angnes@acad.pucrs.br

	Date: 29/05/2017

*/

// Para pegar o ip local
#include <ifaddrs.h>		// getifaddrs()
#include <netdb.h>			// NI_MAXHOST
//#include <sys/socket.h>		// getnameinfo()


#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// error()
#include "wpp.h"

#define IPSIZE 			15
#define MSGSIZE			32
#define MAXSIZE 		255
#define NAMESIZE		8
#define MAXUSERS		8
#define CONTACTSFILE 	"contacts.txt"

//Declaracao de estruturas
struct stConnectedContacts
{
	struct stContact *ctt;							// Estrutura contém nome e ip
	struct stMessage *msg;							// Buffer de mensagem
	CLIENT *cl;
};

// Variaveis globais
struct stConnectedContacts online[MAXUSERS]; 		// Array que contem dados dos contatos
struct stContact me;
int nContacts = 0;									// Numero de contatos no array

// Declaracoes de rotinas
void waitForCommand();												// Aguarda comando
void parseCommand(char *cmd);										// Faz a depuracao do comando e executa ele
char *adjustPointer(char *array, int pos);							// Ajusta o ponteiro de inicio do array
struct stContact *contactData(char *contactData);					// Coloca dados do array em uma struct stContact
void addContact(struct stContact *ctt);								// Adiciona contato no arquivo local
void connectToContact(struct stContact *ctt, int id);				// Tenta conectar (virar cliente) do contato
int checkExistentContact(struct stContact *ctt);					// Verifica se contato ja existe na lista de contatos
char *getName(char *array);											// Extrai nome do array
int searchForConnectedContacts(char *name);							// Procura contato conectado pelo nome, retorna o indice desse contato em online[MAXUSERS]
int sendAddRequest(struct stContact *me, struct stContact *ctt);	// Solicita ao 'ctt' para ser adicionado na lista de contatos "alvo"
void loadOnlineContacts();											// Carrega lista de contatos para o array de contatos online
void writeSentMessage(int cttIndex);								// Escreve mensagem enviada no arquivo
void writeNoSentMessage(char *name, char *msg);						// Escreve mensagem não enviada no arquivo
void sendNoSentMessage(struct stContact *ctt);						// Envia mensagens pendentes para 'ctt'

// Rotina que le o comando
void waitForCommand()
{
	char *cmd;

	cmd = (char*)malloc(MAXSIZE);

	// Temporario
	sprintf(me.ip, "127.0.0.1");

	printf("#");
	scanf("%[^\n]%*c", cmd);

	parseCommand(cmd);
}

// Rotina que computa o comando
void parseCommand(char *cmd)
{
	// Multiple cases variables
	char *cttData; 			// i, c
	char *name;				// s, l
	char *message;			// s, l
	ssize_t read;			// c, l
	size_t len = 0;			// c, l

	// i variables
	struct stContact *ctt;

	// s variable
	int i, *ret;
	char *conversationFileName;
	struct stMessage *msg;
	void *pvoid;

	// c variables
	FILE *contactsFile;
	struct stContact;
	
	// l variables
	FILE *conversationFile;

	switch(cmd[0])
	{
		case 'i':

			if(nContacts >= MAXUSERS){
				printf("Numero maximo de contatos atingido\n");
				break;
			}else{
				cttData = (char*)malloc(MAXSIZE);
				memset(cttData, 0, MAXSIZE);
				memcpy(cttData, cmd, strlen(cmd));
				cttData = adjustPointer(cttData, 2);
				ctt = contactData(cttData);

				if(checkExistentContact(ctt) == 0){	
					connectToContact(ctt, 1);
					//TODO: pegar dados do usuario corrente e colocar em 'cttMe'
					//if(sendAddRequest(&me, ctt) == 1)
					addContact(ctt);
					//TODO: else destruindo conexão previamente estabelecida
				}else{
					printf("stContact already exists\n");
				}
			}
			break;
		case 'g':
			printf("received 'group' command: %s\n", cmd);
			break;
		case 'l':
			name = (char*)malloc(NAMESIZE);
			message = (char*)malloc(MSGSIZE + 5);
			conversationFileName = (char*)malloc((NAMESIZE * 2) + 1);
			
			name = adjustPointer(cmd, 2);
			sprintf(conversationFileName, "%s_%s.txt", me.name, name);
			conversationFile = fopen(conversationFileName, "r");
			while((read = getline(&message, &len, conversationFile)) != -1){
				printf("%s", message);
			}
			break;
		case 's':
			
			name = (char*)malloc(NAMESIZE);
			message = (char*)malloc(MAXSIZE);
			msg = (struct stMessage*)malloc(sizeof(struct stMessage));
			memset(msg, 0, sizeof(struct stMessage));

			memcpy(message, cmd, strlen(cmd));
			message = adjustPointer(message, 2);						// retira o 's' da string
			name = getName(message);									// extrai o nome do contato da string
			message = adjustPointer(message, strlen(name) + 1);			// ajusta ponteiro para pular o nome
			i = searchForConnectedContacts(name);
			if(i == -1){			// verifica se o contato está na lista de conectados
				// TODO: verificar se 'name' é um contato (verificar em contacts.txt)	
				printf("try\n");
				writeNoSentMessage(name, message);
				printf("ERROR: stContact doesn't exists\n");
			}else{
				printf("aqui\n");
				sprintf(msg->message, "%s: %s", me.name, message);		// coloca a mensagem na estrutura que sera enviada ao contato
				online[i].msg = msg;
				send_message_1(online[i].msg, online[i].cl);
				sprintf(msg->message, "(S) %s: %s", me.name, message);
				writeSentMessage(i);
				if(*ret == 1){	// envia mensagem para o contato
					//TODO: MARCAR COMO ENVIADO

				}
			}
			waitForCommand();
			break;
		case 'c':
				cttData = (char*)malloc(NAMESIZE+IPSIZE+1);
				contactsFile = fopen(CONTACTSFILE, "r");

				while((read = getline(&cttData, &len, contactsFile)) != -1){
					printf("%s\n", cttData);
				}
			break;
		default:
			printf("Not valid command\n");
			break;
	}

	waitForCommand();
}

// Rotina que desloca o ponteiro de um array para ignorar
// as primeiras 'pos' posicoes
char *adjustPointer(char *array, int pos)
{
	int i;

	for(i = 1; i <= pos; i++)
		array++;

	return array;
}


// Rotina que faz o parse de uma string contendo nome e ip de um contato
// e coloca separadamente dentro de uma struct stContact
struct stContact *contactData(char *contactData)
{
	struct stContact *ctt;
	int i, index, nameIndex, ipIndex;

	nameIndex = 0;
	ipIndex = 0;

	ctt = (struct stContact*)malloc(sizeof(struct stContact));
	memset(ctt, 0, sizeof(struct stContact));
	index = strlen(contactData);

	for(i = 0; i < index; i++){
		if((contactData[i] > 0x40 && contactData[i] < 0x5B) ||
		   (contactData[i] > 0x60 && contactData[i] < 0x7B)) {
			ctt->name[nameIndex] = contactData[i];
			nameIndex++;
		}else{
			if((contactData[i] > 0x2F && contactData[i] < 0x3A) || 
				contactData[i] == 0x2E) {
				ctt->ip[ipIndex] = contactData[i];
				ipIndex++;
			}
		}
	}

	return ctt;
}

// Rotina que adiciona o contato localmente
void addContact(struct stContact *ctt)
{
	FILE *contactsFile;

	contactsFile = fopen(CONTACTSFILE, "a+");

	fprintf(contactsFile, "%s\t%s\n", ctt->name, ctt->ip);
	fclose(contactsFile);
}

// Rotina que tenta conectar a um determinado contato
void connectToContact(struct stContact *ctt, int id)
{
	void *pvoid;

	if(!(online[nContacts].cl = clnt_create(ctt->ip, WPPPROG, WPPVERS, "udp"))){
		clnt_pcreateerror(ctt->ip);
		printf("ERROR do add %s to contacts\n", ctt->name);
	}else{
		//(id == 1) ? ack_server_2(pvoid, online[nContacts].cl) : ack_server_1(pvoid, online[nContacts].cl);
		ack_server_1(pvoid, online[nContacts].cl);
		online[nContacts].ctt = ctt;
		nContacts++;

	}
}

int checkExistentContact(struct stContact *ctt)
{
	ssize_t read;
	size_t len = 0;
	char *cttData;
	FILE *contactsFile;
	struct stContact *cttLocal;

	cttData = (char*)malloc(NAMESIZE+IPSIZE+1);
	contactsFile = fopen(CONTACTSFILE, "r");

	while((read = getline(&cttData, &len, contactsFile)) != -1)
	{
		cttLocal = contactData(cttData);
		if((strcmp(ctt->name, cttLocal->name) == 0) ||
			strcmp(ctt->ip, cttLocal->ip) == 0)
			return -1;
	}
	return 0;
}

char *getName(char *array)
{
	char *name;
	int i = 0;

	name = (char*)malloc(NAMESIZE);
	memset(name, 0, NAMESIZE);

	while(array[i] != ' ')
	{
		name[i] = array[i];
		i++;
	}

	array = adjustPointer(array, i+1);
	return name;
}

int searchForConnectedContacts(char *name)
{
	int i;

	for(i = 0; i < MAXUSERS; i++)
	{
		if(online[i].ctt != NULL)
			if(strcmp(online[i].ctt->name, name) == 0)
				return i;
	}
	return -1;
}

int sendAddRequest(struct stContact *me, struct stContact *ctt)
{
	int i, *r;

	i = searchForConnectedContacts(ctt->name);

	r = add_request_1(ctt, online[i].cl);

	return 1;
}

void loadOnlineContacts()
{
	ssize_t read;
	size_t len = 0;
	char *cttData;
	FILE *contactsFile;
	struct stContact *ctt;

	cttData = (char*)malloc(NAMESIZE+IPSIZE+1);
	contactsFile = fopen(CONTACTSFILE, "r");

	while((read = getline(&cttData, &len, contactsFile)) != -1)
	{
		ctt = contactData(cttData);
		connectToContact(ctt, 1);
	}
}

void writeSentMessage(int cttIndex)
{
	FILE *conversationFile;
	char *conversationFileName;

	conversationFileName = (char*)malloc((NAMESIZE * 2) + 1);

	sprintf(conversationFileName, "%s_%s.txt", me.name, online[cttIndex].ctt->name);	// forma char array que possui nome do arquivo de mensagens
	conversationFile= fopen(conversationFileName, "a+");								// abre arquivo de mensagens
	fprintf(conversationFile, "%s\n", online[cttIndex].msg->message);				// adiciona campo no final da mensagem para status de não recebido, recebido e vizualizado
	fclose(conversationFile);
}

void writeNoSentMessage(char *name, char *msg)
{
	FILE *conversationFile;
	char *conversationFileName;

	conversationFileName = (char*)malloc((NAMESIZE * 2) + 1);

	sprintf(conversationFileName, "%s_%s.txt", me.name, name);	// forma char array que possui nome do arquivo de mensagens
	conversationFile = fopen(conversationFileName, "a+");		// abre arquivo de mensagens
	fprintf(conversationFile, "(X) %s\n", msg);					// adiciona campo no final da mensagem para status de não recebido, recebido e vizualizado
	fclose(conversationFile);
}

void sendNoSentMessage(struct stContact *ctt)
{
	FILE *conversationFile;
	char *conversationFileName;

	online[i].ctt = ctt;

	conversationFileName = (char*)malloc((NAMESIZE * 2) + 1);

	sprintf(conversationFileName, "%s_%s.txt", me.name, online[i].ctt->name);	// forma char array que possui nome do arquivo de mensagens
	conversationFile = fopen(conversationFileName, "r");						// abre arquivo de mensagens

	//TODO: implementar envio de mensagens pendentes
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


// Rotina para inicializar o servidor
void *start_server_1_svc(void *pvoid, struct svc_req *rqstp)
{
	struct ifaddrs *ifaddr, *ifa;
	int family, s, n;
	char host[NI_MAXHOST], name[NAMESIZE];

	if(getifaddrs(&ifaddr) == -1){
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	for(ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++){
		if(ifa->ifa_addr == NULL)
			continue;

		family = ifa->ifa_addr->sa_family;

		if(family == AF_INET && strcmp(ifa->ifa_name, "eth0") == 0){
			s = getnameinfo(ifa->ifa_addr,
							sizeof(struct sockaddr_in),
							me.ip, NI_MAXHOST,
							NULL, 0, NI_NUMERICHOST);
			if(s != 0){
				printf("getnameinfo() failed: %s\n", gai_strerror(s));
				exit(EXIT_FAILURE);
			}
		}
	}

	printf("Name: ");
	fgets(me.name, NAMESIZE, stdin);
	me.name[strcspn(me.name, "\n")] = 0;

	loadOnlineContacts();
	waitForCommand();
}

void *ack_server_1_svc(void *pvoid, struct svc_req *rqstp)
{
	printf("CONNECTED1 OK\n");
}

void *send_message_1_svc(struct stMessage *msg, struct svc_req *rqstp)
{
	printf("%s\n", msg->message);
}

int *add_request_1_svc(struct stContact *ctt, struct svc_req *rqstp)
{
	int *ret;

	if(nContacts >= MAXUSERS){
		printf("Numero maximo de contatos atingido\n");
		*ret = -1;
		return ret;
	}else{
		connectToContact(ctt, 1);
		addContact(ctt);
		*ret = 1;
		return ret;
	}
}

void *read_message_1_svc()
{
	//TODO: implementar rotina que marca mensagem como lida
}

void *i_am_online_1_svc(struct stContact *ctt)
{

}