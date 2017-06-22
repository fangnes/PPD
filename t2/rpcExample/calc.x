/** 
    Author: Avelino F. Zorzo
    avelino.zorzo@pucrs.br

    Date: 10.03.2001
    Modified: 10.03.2012

   This code will be translated into the needed stubs and headers
   Use: rpcgen calc.x

**/


struct record {       /* arguments for RPC must be one single */
  int first_num;      /* value or a structure of values */
  int second_num;     /*  */
}; 

const MAXSTRLEN = 255;	

program CALCPROG {                /* value to register the program */
    version CALCVERS {            /* version must be assigned a value */
        int ADD_ARGS(record) = 1;  /* this is a service function */
        int SUB_ARGS(record) = 2;  /* this is a service function */              
        int STATE(void)      = 3;           /* this is a service function */    
    } =1;                          /* version value */
} = 0x20000003;                    /* program value */   
