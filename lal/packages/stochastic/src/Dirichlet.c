/*----------------------------------------------------------------------- 
 * 
 * File Name: Dirichlet.c 
 * 
 * Author: UTB Relativity Group
 * 
 * Revision: $Id$
 * 
 *----------------------------------------------------------------------- 
 *
 * NAME
 * Dirichlet
 *
 * SYNOPSIS
 *
 * void LALDirichlet( LALStatus*, REAL4Vector*, DirichletParameters* )
 *
 * typedef struct tagDirichletParameters{
 *   INT4   n; 
 *   INT4   length;
 *   REAL8  deltaX;
 * } DirichletParameters; 
 *
 * DESCRIPTION
 * Calculates the values of the LALDirichlet kernel for a discrete set of
 * of values starting at x = 0.
 *
 * DIAGNOSTICS
 * null pointer to input parameters
 * LALDirichlet parameter N <= 0
 * specified length of output vector <= 0
 * spacing of x values <= 0
 * null pointer to output vector 
 * length of output vector not equal to length specified in input parameters
 * null pointer to data member of output vector
 *
 * CALLS
 *
 * NOTES
 *
 *------------------------------------------------------------------------
 */

#include <lal/LALStdlib.h>
#include <lal/LALConstants.h>
#include <math.h>
#include <lal/Dirichlet.h>

NRCSID (DIRICHLETC, "$Id$");

void 
LALDirichlet(LALStatus*              pstatus, 
	  REAL4Vector*         poutput, 
	  DirichletParameters* pparameters )

{
  INT4  i;
  INT4  n;	   /* LALDirichlet parameter N */
  INT4  length;	   /* specified length of output vector */
  REAL8 deltaX;    /* spacing of x values */
  REAL8 x;  
  REAL8 top;
  REAL8 bot;

  /* initialize status structure */
  INITSTATUS( pstatus, "LALDirichlet", DIRICHLETC );

  /* check that pointer to input parameters is not null */
  ASSERT(pparameters != NULL,pstatus,DIRICHLET_ENULLIP,DIRICHLET_MSGENULLIP);
	
  /* check that LALDirichlet parameter N is > 0 */
  ASSERT(pparameters->n > 0,pstatus,DIRICHLET_ENVALUE,DIRICHLET_MSGENVALUE);

  /* assign valid data to LALDirichlet parameter N */
  n=pparameters->n;

  /* check that specified length of output vector is > 0 */
  ASSERT(pparameters->length > 0,pstatus,DIRICHLET_ESIZE,DIRICHLET_MSGESIZE);

  /* assign valid data to specified length */
  length=pparameters->length;

  /* check that spacing of x values is > 0 */
  ASSERT(pparameters->deltaX > 0,pstatus,DIRICHLET_EDELTAX,
	 DIRICHLET_MSGEDELTAX);

  /* assign valid data to delta x */
  deltaX=pparameters->deltaX;

  /* check that pointer to output vector is not null */
  ASSERT(poutput != NULL,pstatus,DIRICHLET_ENULLOP,DIRICHLET_MSGENULLOP);

  /* check that output vector length agrees with length specified in */
  /* input parameters */
  ASSERT((INT4)poutput->length==length,pstatus,DIRICHLET_ESIZEMM,
	 DIRICHLET_MSGESIZEMM);

  /* check that pointer to data member of output vector is not null */
  ASSERT(poutput->data != NULL,pstatus,DIRICHLET_ENULLD,DIRICHLET_MSGENULLD);

  /* everything okay here ----------------------------------------------*/

  /* calculate the values of the LALDirichlet kernel D_N(x) */

  poutput->data[0] = 1;  /* D_N(0)=1 */

  for ( i = 1; i < length; i++) {
    x = i * deltaX;
    
    if ( x-floor(x) == 0 ) {  /* D_N(x)=(-1)^(x * (N-1)) */
      poutput->data[i] = pow(-1.0, ( (INT4)(x * (n-1)) ) );
    }
    else {
      top = sin(LAL_PI * n * deltaX * i);
      bot = n * sin(LAL_PI * deltaX * i);
      poutput->data[i] = top/bot;
    }
  } 

  return;
}

