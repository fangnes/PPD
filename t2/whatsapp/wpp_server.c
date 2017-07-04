/*
	Server that will manage. whatsapp application

	Author: Felipe Angnes
	felipe.angnes@acad.pucrs.br

	Date: 29/05/2017

*/

// Para pegar o ip local
#include <ifaddrs.h>		// getifaddrs()
#include <netdb.h>			// NI_MAXHOST

// Threads
#include <pthread.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// error()
#include <unistd.h>			// sleep()
#include "wpp.h"

#define IPSIZE 			15
#define MSGSIZE			32
#define MAXSIZE 		255
#define NAMESIZE		8
#define MAXUSERS		8
#define MAXGROUPS		4
#define CONTACTSFILE 	"contacts.txt"

//Declaracao de estruturas
struct stConnectedContacts
{
	struct stContact *ctt;							// Estrutura contém nome e ip
	CLIENT *cl;
};

struct stGroup
{
	char name[NAMESIZE + 1];
	int countMembers;
	struct stConnectedContacts gpCtts[MAXUSERS];	// Estrutura utilizada para gerenciar grupos
};

// Variaveis globais
static struct stConnectedContacts online[MAXUSERS]; // Array que contem dados dos contatos
static struct stGroup groups[MAXGROUPS];			// Array que contem dados dos grupos
static struct stContact me;							// Struct com dados do usuario
static int nContacts = 0;							// Numero de contatos no array
static int nGroups = 0;								// Numero de grupos do usuario

// Threads
pthread_t mainThread;								// Thread que processara comandos

// Declaracoes de rotinas
void *waitForCommand();									// Aguarda comando
void parseCommand(char *cmd);							// Faz a depuracao do comando e executa ele
char *adjustPointer(char *array, int pos);				// Ajusta o ponteiro de inicio do array
struct stContact *contactData(char *contactData);		// Coloca dados do array em uma struct stContact
void addContact(struct stContact *ctt);					// Adiciona contato no arquivo local
void connectToContact(struct stContact *ctt);			// Tenta conectar (virar cliente) do contato
int checkExistentContact(struct stContact *ctt);		// Verifica se contato ja existe na lista de contatos
char *getName(char *array);								// Extrai nome do array
int searchForConnectedContacts(char *name);				// Procura contato conectado pelo nome, retorna o indice desse contato em online[MAXUSERS]
int sendAddRequest(struct stContact *ctt);				// Solicita ao 'ctt' para ser adicionado na lista de contatos "alvo"
void loadOnlineContacts();								// Carrega lista de contatos para o array de contatos online
void writeSentMessage(char *name, char *msg);			// Escreve mensagem enviada no arquivo
void writeNoSentMessage(char *name, char *msg);			// Escreve mensagem não enviada no arquivo
void sendNoSentMessage(struct stContact *ctt);			// Envia mensagens pendentes para 'ctt'
void writeReceivedMessage(char *message);				// Escreve mensagem recebida
void groupMembers(char *groupName);						// Identifica membros do grupo e os adiciona no array de dados dos grupos
void sendGroupRequest(char *groupName);					// Solicita ao 'ctt' para criar o grupo localmente	
int checkExistentGroup(char *groupName);				// Verifica se o grupo ja existe
int searchForGroups(char *groupName);					// Procura grupo pelo nome, retorna o indice desse contato em groups[MAXGROUPS]
void sendGroupMessage(char *groupName, char *message);	// Envia mensagem para o grupo

// Rotina que le o comando
void *waitForCommand()
{
	char *cmd;

	cmd = (char*)malloc(MAXSIZE);
	memset(cmd, 0, MAXSIZE);
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

	// g variables
	char *groupData;
	char *groupName;

	// s variable
	int i, *ret;
	char *conversationFileName;
	struct stMessage *stMsg;
	char *msg;
	void *pvoid;

	// c variables
	FILE *contactsFile;
	struct stContact;
	
	// l variables
	FILE *conversationFile;

	switch(cmd[0])
	{
		case 'i':

			if(nContacts >= MAXUSERS){							// Verifica se numero maximo de contatos foi atingido
				printf("Numero maximo de contatos atingido\n");
				break;
			}else{
				cttData = (char*)malloc(MAXSIZE);
				memset(cttData, 0, MAXSIZE);
				memcpy(cttData, cmd, strlen(cmd));				// Copia conteudo de 'cmd' para 'cttData'
				cttData = adjustPointer(cttData, 2);			// Retira o 'i ' do inicio do char array
				ctt = contactData(cttData);						// Extrai nome e ip de 'cttData' e coloca dentro da estrutura 'ctt'

				if(checkExistentContact(ctt) == 0)				// Verifica se o contato ja foi adicionado
				{
					connectToContact(ctt);						// Conecta ao contato
					if(sendAddRequest(ctt) == 1)				// Envia requisicao para contato add usuario localmente
						addContact(ctt);						// Adiciona contato no arquivto 'contacts.txt'
					//TODO: else destruindo conexão previamente estabelecida
				}
				else
				{
					printf("Contact already added\n");			// Alerta de contato ja adicionado
				}
			}
			break;
		case 'g':
			
			groupData = (char*)malloc(MAXSIZE);
			groupName = (char*)malloc(NAMESIZE);
			memset(groupData, 0, MAXSIZE);
			memset(groupName, 0, NAMESIZE);

			groupData = adjustPointer(cmd, 2);										// Retira o 'g' do inicio do char array
			groupName = getName(groupData);											// 'groupName' contém o nome do grupo
			if(checkExistentGroup(groupName) == 1)
			{
				memcpy(groups[nGroups].name, groupName, strlen(groupName));				// Coloca nome do grupo na estrutura referente ao grupo
				groupData = adjustPointer(groupData, strlen(groupName) + 1);			// Ajusta ponteiro de 'groupData' para pular o nome do grupo
				groupMembers(groupData);												// Monta estrutura do grupo
				sendGroupRequest(groupName);											// Envia requisições para integrantes do grupo também criarem o grupo localmente
			}
			else
				printf("Grupo ja existente.\n");
			break;
		case 'l':
			name = (char*)malloc(NAMESIZE);
			message = (char*)malloc(MSGSIZE + 5);
			conversationFileName = (char*)malloc((NAMESIZE * 2) + 1);
			
			name = adjustPointer(cmd, 2);											// Retira o 'l ' do inicio do char array
			sprintf(conversationFileName, "%s_%s.txt", me.name, name);				// Monta nome de arquivo que ira conter o historico de conversa entre os contatos
			conversationFile = fopen(conversationFileName, "r");					// Abre arquivo de histórico de conversas
			if(conversationFile != NULL){
				while((read = getline(&message, &len, conversationFile)) != -1){	// Exibe o historico de conversas linha por linha
					printf("%s", message);
				}
			}
			break;
		case 's':
			
			printf("cmd: %s\n", cmd);

			name = (char*)malloc(NAMESIZE);
			message = (char*)malloc(MSGSIZE);
			msg = (char*)malloc(NAMESIZE + MSGSIZE + 6);
			stMsg = (struct stMessage*)malloc(sizeof(struct stMessage));
			
			memset(message, 0, MSGSIZE);
			memset(msg, 0, NAMESIZE + MSGSIZE + 6);
			memset(stMsg, 0, sizeof(struct stMessage));

			memcpy(message, cmd, strlen(cmd));
			
			message = adjustPointer(message, 2);					// retira o 's' da string
			name = getName(message);								// extrai o nome do contato da string
			message = adjustPointer(message, strlen(name) + 1);		// ajusta ponteiro para pular o nome
			i = searchForConnectedContacts(name);					// Procura pelo index do contato no array de contatos 'online'
			if(i == -1){											// verifica se o contato está na lista de conectados
				// TODO: verificar se 'name' é um contato (verificar em contacts.txt)	
				// TODO: verificar se a mensagem não é para um grupo
				i = searchForGroups(name);
				if(i == -1)
					writeNoSentMessage(name, message);
				else
				{
					//TODO: chamar funcao que envia mensagem para membros do grupo
				}
			}else{
				sprintf(msg, "%s: %s", me.name, message);			// coloca a mensagem na estrutura que sera enviada ao contato
				memcpy(stMsg->message, msg, strlen(msg));
				send_message_1(stMsg, online[i].cl);				// envia mensagem chamando procedimento remoto
				sprintf(msg, "(S) %s: %s", me.name, message);		// monta mensagem que sera colocada no arquivo com historicos de mensagens
				writeSentMessage(name, msg);									// escreve a mensagem no arquivo
			}
			break;
		case 'c':
				cttData = (char*)malloc(NAMESIZE+IPSIZE+1);
				contactsFile = fopen(CONTACTSFILE, "r");

				if(contactsFile != NULL){
					while((read = getline(&cttData, &len, contactsFile)) != -1){
						printf("%s\n", cttData);
					}
				}
				fclose(contactsFile);
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
	index = strlen(contactData);									// verifica tamanho total do char array

	for(i = 0; i < index; i++){										// percorre array verificando cada caractere
		if((contactData[i] > 0x40 && contactData[i] < 0x5B) ||		// verifica, atraves do valor hex do caractere, se corresponde a uma letra ou numero
		   (contactData[i] > 0x60 && contactData[i] < 0x7B)) {		// se for uma letra, comeca a preencher o char array ctt->name
			ctt->name[nameIndex] = contactData[i];
			nameIndex++;
		}else{
			if((contactData[i] > 0x2F && contactData[i] < 0x3A) || 	// se for um numero, comeca a preencher o char array ctt->ip
				contactData[i] == 0x2E) {
				ctt->ip[ipIndex] = contactData[i];
				ipIndex++;
			}
		}
	}
	return ctt;														// retorna estrutura com nome e ip do contato
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
void connectToContact(struct stContact *ctt)
{
	void *pvoid;

	if(!(online[nContacts].cl = clnt_create(ctt->ip, WPPPROG, WPPVERS, "tcp"))){
		clnt_pcreateerror(ctt->ip);
		printf("ERROR do add %s to contacts\n", ctt->name);
	}else{
		online[nContacts].ctt = ctt;
		nContacts++;
	}
}

// Verifica se contato ja existe na lista de contatos
int checkExistentContact(struct stContact *ctt)
{
	ssize_t read;
	size_t len = 0;
	char *cttData;
	FILE *contactsFile;
	struct stContact *cttLocal;

	cttData = (char*)malloc(NAMESIZE+IPSIZE+1);
	contactsFile = fopen(CONTACTSFILE, "r");

	if(contactsFile != NULL){											// se o arquivo foi aberto com sucesso
		while((read = getline(&cttData, &len, contactsFile)) != -1)		// percorre linha por linha
		{
			cttLocal = contactData(cttData);							// organiza a informacao do arquivto na esctrutura 'ctt'
			if((strcmp(ctt->name, cttLocal->name) == 0) ||				// verifica se o nome extraido do arquivo e igual ao recebido por parametro
				strcmp(ctt->ip, cttLocal->ip) == 0)						// verifica se o ip extraido do arquivo e igual ao recebido por parametro
				return -1;												// se algum nome ou ip presente no arquivo for igual ao recebido por parametro retorna -1
		}
	}
	return 0;															// se nao houver nome ou ip, igual ao recebido por parametro, no arquivo retorna 0
}

// Extrai nome do array
char *getName(char *array)
{
	char *name;
	int i = 0;

	name = (char*)malloc(NAMESIZE);
	memset(name, 0, NAMESIZE);
	while(array[i] != ' ' && array[i] != ':')					// os primeiros caracteres do array recebido por parametro corresponderao ao nome do contato
	{
		name[i] = array[i];
		i++;
	}
	//array = adjustPointer(array, i+1);		// ajusta o ponteiro do array para excluir o nome e o espaco logo apos o nome
	return name;
}

// Procura contato conectado pelo nome, retorna o indice desse contato em online[MAXUSERS]
int searchForConnectedContacts(char *name)
{
	int i;

	for(i = 0; i < MAXUSERS; i++)
	{
		if(online[i].ctt != NULL)						// percorre array de contatos online
			if(strcmp(online[i].ctt->name, name) == 0)	// analisa o nome dos contatos
				return i;
	}
	return -1;
}

// Solicita ao 'ctt' para ser adicionado na lista de contatos "alvo"
int sendAddRequest(struct stContact *ctt)
{
	int i, *r;

	i = searchForConnectedContacts(ctt->name);	// busca indice do contato "alvo"
	r = add_request_1(&me, online[i].cl);		// envia requisicao para ser adicionado na lista de contatos do contato "alvo"

	return 1;
}

// Carrega lista de contatos para o array de contatos online
void loadOnlineContacts()
{
	ssize_t read;
	size_t len = 0;
	char *cttData;
	FILE *contactsFile;
	struct stContact *ctt;

	cttData = (char*)malloc(NAMESIZE+IPSIZE+1);
	contactsFile = fopen(CONTACTSFILE, "r");						// abre arquivo de contatos

	if(contactsFile != NULL){
		while((read = getline(&cttData, &len, contactsFile)) != -1)	// percorre o arquivo linha por linha
		{
			ctt = contactData(cttData);								// coloca informacoes extraidas do arquivo em uma estrutura 'ctt'
			connectToContact(ctt);								// tenta conectar no contato
		}
	}
}

// Escreve mensagem enviada no arquivo
void writeSentMessage(char *name, char *msg)
{
	FILE *conversationFile;
	char *conversationFileName;

	conversationFileName = (char*)malloc((NAMESIZE * 2) + 1);

	sprintf(conversationFileName, "%s_%s.txt", me.name, name);	// forma char array que possui nome do arquivo de mensagens
	conversationFile = fopen(conversationFileName, "a+");		// abre arquivo de mensagens
	fprintf(conversationFile, "%s\n", msg);						// adiciona campo no final da mensagem para status de não recebido, recebido e vizualizado
	fclose(conversationFile);
}

// Escreve mensagem não enviada no arquivo
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

// Envia mensagens pendentes para 'ctt' ** NOT READY **
void sendNoSentMessage(struct stContact *ctt)
{
	ssize_t read;
	size_t len = 0;
	FILE *conversationFile;
	char *conversationFileName, *message;

	online[nContacts].ctt = ctt;

	conversationFileName = (char*)malloc((NAMESIZE * 2) + 1);
	message = (char*)malloc(MAXSIZE);

	sprintf(conversationFileName, "%s_%s.txt", me.name, online[nContacts].ctt->name);	// forma char array que possui nome do arquivo de mensagens
	conversationFile = fopen(conversationFileName, "w+");	// verificar w+				// abre arquivo de mensagens

	/*while((read = getline(&message, &len, contactsFile)) != -1){
		printf("%s\n", cttData);
	}*/
	//TODO: implementar envio de mensagens pendentes
}

/*void writeReceivedMessage(char *message)
{
	FILE *conversationFile;
	char *conversationFileName;

	conversationFileName = (char*)malloc((NAMESIZE * 2) + 1);

	sprintf(conversationFileName, "%s_%s.txt", me.name, name);	// forma char array que possui nome do arquivo de mensagens
	conversationFile = fopen(conversationFileName, "a+");		// abre arquivo de mensagens
	fprintf(conversationFile, "%s\n", msg);					// adiciona campo no final da mensagem para status de não recebido, recebido e vizualizado
	fclose(conversationFile);
}*/

void groupMembers(char *groupData)
{
	int i, nameIndex, contactIndex, memberIndex;
	char *name;

	memberIndex = 0;
	nameIndex = 0;

	name = (char*)malloc(NAMESIZE);
	memset(name, 0, NAMESIZE);

	for(i = 0; i < strlen(groupData) + 1; i++)
	{
		if(groupData[i] != ' ' && groupData[i] != '\0')
		{
			name[nameIndex] = groupData[i];
			nameIndex++;
		}
		else
		{
			if(memberIndex <= MAXUSERS)
			{
				contactIndex = searchForConnectedContacts(name);
				memcpy(&groups[nGroups].gpCtts[memberIndex], &online[contactIndex], sizeof(struct stConnectedContacts));
				groups[nGroups].countMembers++;
				memberIndex++;
			}
			else
			{
				printf("Numero maximo de participantes atingido\n");
			}
			nameIndex = 0;
			memset(name, 0, NAMESIZE);
		}
	}
}

void sendGroupRequest(char *groupName)
{
	int i, j, groupIndex;
	char *groupData;
	struct stMessage *groupPackage;

	groupData = (char*)malloc(MAXSIZE);
	groupPackage = (struct stMessage*)malloc(sizeof(struct stMessage));

	memset(groupData, 0, MAXSIZE);
	memset(groupPackage, 0, sizeof(struct stMessage));


	for(i = 0; i < MAXGROUPS; i++)
	{
		if(strcmp(groupName, groups[i].name) == 0)
		{	
			groupIndex = i;
			break;
		}
	}

	for(i = 0; i < groups[groupIndex].countMembers; i++)
	{
		sprintf(groupData, "%s %s ", groupName, me.name);
		for(j = 0; j < groups[groupIndex].countMembers; j++)
		{
			if(strcmp(groups[groupIndex].gpCtts[i].ctt->name, 
					  groups[groupIndex].gpCtts[j].ctt->name) != 0)
			{
				strcat(groupData, groups[groupIndex].gpCtts[j].ctt->name);
				strcat(groupData, " ");
			}
		}
		memcpy(groupPackage->message, groupData, strlen(groupData));
		group_request_1(groupPackage, groups[groupIndex].gpCtts[i].cl);
		memset(groupData, 0, MAXSIZE);
	}
}

int checkExistentGroup(char *groupName)
{
	int i = 0;

	for(i = 0; i < nGroups; i++)
	{
		if(groups[i].name != NULL)
			if(strcmp(groupName, groups[i].name) == 0)
				return -1;
	}
	return 1;
}

int searchForGroups(char *groupName)
{
	int i;

	for(i = 0; i < nGroups; i++)
	{
		if(strcmp(groupName, groups[i].name) == 0)
			return i;
	}
	return -1;
}

void sendGroupMessage(char *groupName, char *message)
{
	int i, groupIndex;
	struct stGroupMessage *gpMessage;

	gpMessage = (struct stGroupMessage*)malloc(sizeof(struct stGroupMessage));
	memset(gpMessage, 0, sizeof(struct stGroupMessage));
	memcpy(gpMessage->name, groupName, strlen(groupName));
	memcpy(gpMessage->message, message, strlen(message));

	groupIndex = searchForGroups(groupName);

	for(i = 0; i < groups[groupIndex].countMembers; i++)
	{
		send_group_message_1(gpMessage, groups[groupIndex].gpCtts[i].cl);
		// TODO: implementar send_group_message_1
	}
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
	int family, s, n, result;
	char host[NI_MAXHOST], name[NAMESIZE];

	memset(&online, 0, sizeof(online));
	memset(&groups, 0, sizeof(groups));

	if(getifaddrs(&ifaddr) == -1){
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	for(ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++){
		if(ifa->ifa_addr == NULL)
			continue;

		family = ifa->ifa_addr->sa_family;

		if(family == AF_INET && strcmp(ifa->ifa_name, "eno1") == 0){
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
	printf("IP: %s\n", me.ip);

	//startThreade();
	
	
	loadOnlineContacts();
	result = pthread_create(&mainThread, NULL, waitForCommand, NULL);
	//waitForCommand();
}


void *ack_server_1_svc(void *pvoid, struct svc_req *rqstp)
{
	printf("CONNECTED OK\n");
}

void *send_message_1_svc(struct stMessage *msg, struct svc_req *rqstp)
{
	FILE *conversationFile;
	char *conversationFileName, *name, *message;
	int i;

	name = (char*)malloc(NAMESIZE);
	message = (char*)malloc(MSGSIZE + NAMESIZE + 2);			// mensagem + nome + ": "
	conversationFileName = (char*)malloc((NAMESIZE * 2) + 1);
	memset(name, 0, NAMESIZE);
	memset(message, 0, MSGSIZE);
	memset(conversationFileName, 0, (NAMESIZE * 2) + 1);

	memcpy(message, msg->message, strlen(msg->message));
	printf("Message: %s\n", message);
	name = getName(msg->message);

	sprintf(conversationFileName, "%s_%s.txt", me.name, name);	// forma char array que possui nome do arquivo de mensagens
	conversationFile = fopen(conversationFileName, "a+");		// abre arquivo de mensagens
	fprintf(conversationFile, "%s\n", message);
	fclose(conversationFile);
}

int *add_request_1_svc(struct stContact *ctt, struct svc_req *rqstp)
{
	int *ret, i;
	struct stContact *cttLocal;

	cttLocal = (struct stContact*)malloc(sizeof(struct stContact));
	memcpy(cttLocal, ctt, sizeof(struct stContact));

	if(nContacts >= MAXUSERS){
		printf("Numero maximo de contatos atingido\n");
		*ret = -1;
		return ret;
	}else{
		if(checkExistentContact(cttLocal) == 0){
			connectToContact(cttLocal);
			addContact(cttLocal);
		}
		*ret = 1;
		return ret;
	}
}

void *read_message_1_svc(void *pvoid, struct svc_req *rqstp)
{
	// TODO: enviar notificação de mensagem lida
}

void *i_am_online_1_svc(struct stContact *ctt, struct svc_req *rqstp)
{
	// TODO: enviar notificação de que estou online
}

void *group_request_1_svc(struct stMessage *msg, struct svc_req *rqstp)
{
	char *groupData, *groupName;
	int i;

	groupData = (char*)malloc(MAXSIZE);
	groupName = (char*)malloc(NAMESIZE);
	memset(groupData, 0, MAXSIZE);
	memset(groupName, 0, NAMESIZE);

	memcpy(groupData, msg->message, strlen(msg->message));

	groupName = getName(groupData);											// 'groupName' contém o nome do grupo
	memcpy(groups[nGroups].name, groupName, strlen(groupName));				// Coloca nome do grupo na estrutura referente ao grupo
	groupData = adjustPointer(groupData, strlen(groupName) + 1);			// Ajusta ponteiro de 'groupData' para pular o nome do grupo
	groupMembers(groupData);												// Monta estrutura do grupo

	printf("MEMBROS DO GRUPO: \n");
	for(i = 0; i < groups[nGroups].countMembers; i++)
		printf("%s\n", groups[nGroups].gpCtts[i].ctt->name);

	nGroups++;
}

void *send_group_message_1_svc(struct stGroupMessage *gpMsg, struct svc_req *rqstp)
{
	// TODO: implementar gravacao da mensagem recebida no arquivo do grupo
}