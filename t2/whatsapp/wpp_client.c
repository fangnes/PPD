/*
	Node that will connect to server amd send messages

	Author: Felipe Angnes
	felipe.angnes@acad.pucrs.br

	Date: 29/05/2017
*/

#include <stdio.h>
//#include <stdlib.h>
#include <rpc/rpc.h>
#include "wpp.h"

int main(int argc, char **argv)
{
	CLIENT *cl;
	void *pvoid;

	if(!(cl = clnt_create("127.0.0.1", WPPPROG, WPPVERS, "tcp")))
	{
		clnt_pcreateerror(argv[1]);
		return 1;
	}

	start_server_1(pvoid, cl);

	return 0;
}