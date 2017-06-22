
/** Simple client calculator using rpc

    Author: Avelino F. Zorzo
    avelino.zorzo@pucrs.br

    Date: 10.03.2001
    Modified: 10.03.2012

**/


#include <stdio.h>
#include <rpc/rpc.h>
#include "calc.h"

main(int argc, char *argv[]) {

    CLIENT *cl;
    int *answer;
    void *pvoid;

    record *rec = (record *) malloc(sizeof(record));

    if (argc != 4) {
        printf("Usage: %s server arg1 arg2\n", argv[0]); 
        return (1); 
    } 

    if (!(cl = clnt_create(argv[1], CALCPROG,CALCVERS,"tcp"))) { 
        clnt_pcreateerror(argv[1]); 
        return(1); 
    } 

    rec->first_num = atoi(argv[2]); 
    rec->second_num= atoi(argv[3]); 

    /** Sum **/

    answer = add_args_1(rec,cl); 

    if (answer == (int *) NULL) { 
        fprintf(stderr,"Error: could not produce meaningful results"); 
        return(1); 
    } 

    printf("%s + %s = %d\n", argv[2], argv[3], *answer); 

   /** Sub **/

    answer = sub_args_1(rec,cl); 

    if (answer == (int *) NULL) { 
        printf("error: could not produce meaningful results"); 
        return(1); 
    } 

    printf("%s - %s = %d\n", argv[2], argv[3], *answer); 

   /** State **/

    answer = state_1(pvoid,cl); 

    if (answer == (int *) NULL) { 
        printf("Error: state could not produce meaningful results"); 
        return(1); 
    } 

    printf("State = %d\n", *answer); 


    return (0);
} 
