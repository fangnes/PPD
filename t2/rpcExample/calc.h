/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _CALC_H_RPCGEN
#define _CALC_H_RPCGEN

#include <rpc/rpc.h>


#ifdef __cplusplus
extern "C" {
#endif


struct record {
	int first_num;
	int second_num;
};
typedef struct record record;
#define MAXSTRLEN 255

#define CALCPROG 0x20000003
#define CALCVERS 1

#if defined(__STDC__) || defined(__cplusplus)
#define ADD_ARGS 1
extern  int * add_args_1(record *, CLIENT *);
extern  int * add_args_1_svc(record *, struct svc_req *);
#define SUB_ARGS 2
extern  int * sub_args_1(record *, CLIENT *);
extern  int * sub_args_1_svc(record *, struct svc_req *);
#define STATE 3
extern  int * state_1(void *, CLIENT *);
extern  int * state_1_svc(void *, struct svc_req *);
extern int calcprog_1_freeresult (SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define ADD_ARGS 1
extern  int * add_args_1();
extern  int * add_args_1_svc();
#define SUB_ARGS 2
extern  int * sub_args_1();
extern  int * sub_args_1_svc();
#define STATE 3
extern  int * state_1();
extern  int * state_1_svc();
extern int calcprog_1_freeresult ();
#endif /* K&R C */

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_record (XDR *, record*);

#else /* K&R C */
extern bool_t xdr_record ();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_CALC_H_RPCGEN */