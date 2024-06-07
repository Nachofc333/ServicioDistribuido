/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include "imprimir.h"

bool_t
imprimir_nf_1_svc(char *op, char *fecha, char *hora, char *user, int *result,  struct svc_req *rqstp)
{
	bool_t retval = TRUE;

	printf("%s    %s    %s %s\n", user, op, fecha, hora);

	return retval;
}

bool_t
imprimir_f_1_svc(char *op, char *fecha, char *hora, char *user, char *file, int *result,  struct svc_req *rqstp)
{
	bool_t retval = TRUE;

	printf("%s    %s %s    %s %s\n", user, op, file, fecha, hora);

	return retval;
}

int
imprimir_1_freeresult (SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
	xdr_free (xdr_result, result);

	/*
	 * Insert additional freeing code here, if needed
	 */

	return 1;
}
