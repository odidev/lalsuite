/**
 * \file 
 * \ingroup NormalizeSFTRngMed.h
 * \author Krishnan, B.
 * \date $Date$
 * \brief Header file for SFT normalization routines
 * 
 * Revision: $Id$
 *
 * History:   Moved from LALAPPS 31/7/05
 *            
 *
 *-----------------------------------------------------------------------
 */
 
/* *********************************** <lalVerbatim file="SFTbinHV">
Author: Krishnan, B 
$Id$
************************************* </lalVerbatim> */

/* <lalLaTeX>  *********************************************
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\section{Header \texttt{SFTClean.h}}
\label{s:SFTClean.h}

Routines for cleaning SFT files using known spectral disturbances. 

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\subsection*{Synopsis}

\begin{verbatim}
#include <lal/SFTClean.h>
\end{verbatim}

\noindent Format for list of known spectral disturbances and using 
them to clean SFT data

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\subsection*{Error conditions}
\vspace{0.1in}
\input{SFTCleanHErrorTable}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\vfill{\footnotesize\input{SFTCleanHV}}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\newpage\input{SFTCleanC}
%%%%%%%%%% Test program. %%
\newpage\input{SFTCleanTestC}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

*************************************************</lalLaTeX> */


/**
 * Routines for cleaning SFT files using known spectral disturbances. 
 *
 **/

/*
 * 4.  Protection against double inclusion (include-loop protection)
 *     Note the naming convention!
 */

#ifndef _NORMALIZESFTRNGMED_H
#define _NORMALIZESFTRNGMED_H

/*
 * 5. Includes. This header may include others; if so, they go immediately 
 *    after include-loop protection. Includes should appear in the following 
 *    order: 
 *    a. Standard library includes
 *    b. LDAS includes
 *    c. LAL includes
 */

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <lal/LALStdlib.h>
#include <lal/LALConstants.h>
#include <lal/AVFactories.h>
#include <lal/SeqFactories.h>
#include <lal/SFTfileIO.h>
#include <lal/PulsarDataTypes.h>
#include <lal/UserInput.h>
#include <lal/SFTutils.h>
#include <lal/RngMedBias.h>
#include <lal/LALRunningMedian.h>

/*
 *   Protection against C++ name mangling
 */

#ifdef  __cplusplus
extern "C" {
#endif

 /*
 * 6. Assignment of Id string using NRCSID()  
 */
  
NRCSID (NORMALIZESFTRNGMEDH, "$Id$");
  
/*
 * 7. Error codes and messages. This must be auto-extracted for 
 *    inclusion in the documentation.
 */
  
/* <lalErrTable file="SFTbinHErrorTable"> */
  
#define NORMALIZESFTRNGMEDH_ENULL 1
#define NORMALIZESFTRNGMEDH_EVAL 2
#define NORMALIZESFTRNGMEDH_EMEM 3

#define NORMALIZESFTRNGMEDH_MSGENULL "Null pointer"
#define NORMALIZESFTRNGMEDH_MSGEVAL  "Invalid value"
#define NORMALIZESFTRNGMEDH_MSGEMEM  "Memory allocation problem"

/* </lalErrTable>  */


/* ******************************************************
 * 8. Macros. But, note that macros are deprecated. 
 *    They could be moved to the modules where are needed 
 */  

/* *******************************************************
 * 9. Constant Declarations. (discouraged) 
 */
 


/* **************************************************************
 * 10. Structure, enum, union, etc., typdefs.
 */





/*
 * 11. Extern Global variables. (discouraged) 
 */
  

/*
 * 12. Functions Declarations (i.e., prototypes).
 */


void LALSFTtoPeriodogram (LALStatus    *status,
			  REAL8FrequencySeries    *periodo,
			  const COMPLEX8FrequencySeries *SFT);

void LALPeriodoToPSDRngMed (LALStatus  *status,
			    REAL8FrequencySeries  *psd,
			    const REAL8FrequencySeries  *periodo,
			    UINT4                  blockSize);

void LALSFTtoPSDRngMed (LALStatus  *status,
			REAL8FrequencySeries  *psd,
			const COMPLEX8FrequencySeries *SFT,
			UINT4                  blockSize);

void LALNormalizeSFT (LALStatus            *status,
		      REAL8FrequencySeries *out,		      
		      SFTtype              *sft,
		      UINT4                blockSize);

void LALNormalizeSFTVect (LALStatus  *status,
			  SFTVector  *sftVect,
			  UINT4     blockSize);

void LALNormalizeMultiSFTVect (LALStatus  *status,
			       MultiPSDVector **mult,
			       MultiSFTVector *multsft,
			       UINT4     blockSize);


void LALSFTstoCrossPeriodogram (LALStatus    *status,
				REAL8FrequencySeries    *periodo,
				const COMPLEX8FrequencySeries *sft1,
				const COMPLEX8FrequencySeries *sft2);

#ifdef  __cplusplus
}                /* Close C++ protection */
#endif

#endif     /* Close double-include protection _NORMALIZESFTRNGMED_H */
 







