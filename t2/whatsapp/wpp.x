/*
	Author: Felipe Angnes
	felipe.angnes@acad.pucrs.br

	Date: 27/05/2017

	This code will be translated by RPC to a header file
	Use: rpcgen wpp.x
*/

/* Struct that will carry the app command */
#define MAXSIZE 255
#define MSGSIZE 42	// 32 (msg) + 8 (name) + 2 (": ")
#define NAMESIZE 8
#define IPSIZE 15

struct stMessage
{
	char message[MAXSIZE];
};

struct stGroupMessage
{
	char name[NAMESIZE];
	char message[MAXSIZE];
};

struct stContact
{
	char name[NAMESIZE];
	char ip[IPSIZE];
};

program WPPPROG {
	version WPPVERS {
		void START_SERVER(void) 				= 1;
		void ACK_SERVER(void)					= 2;
		void SEND_MESSAGE(stMessage)			= 3;
		int ADD_REQUEST(stContact) 				= 4;
		void READ_MESSAGE(void)					= 5;
		void I_AM_ONLINE(stContact)				= 6;
		void GROUP_REQUEST(stMessage)			= 7;
		void SEND_GROUP_MESSAGE(stGroupMessage)	= 8;
	} = 1;

} = 0x20000003;