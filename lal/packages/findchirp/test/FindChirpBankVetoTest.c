#include <lal/LIGOMetadataTables.h>
#include <lal/LIGOMetadataUtils.h>
#include <lal/LALStdlib.h>
#include <lal/LALConstants.h>
#include <lal/AVFactories.h>
#include <lal/LALInspiral.h>
#include <lal/FindChirp.h>

#include <lal/LIGOLwXML.h>
#include <lal/LIGOLwXMLRead.h>
#include <lal/LIGOMetadataTables.h>

#define SUBBANKSIZE 16
#define STARTTEMPLATE -1
#define ENDTEMPLATE -1


/* Program to test the function XLALBankVetoCCMat */
int main(int argc, char ** argv)
{
    /* Input and output file names */
    CHAR tmpltBankFileName[100];
    CHAR sortedBankFileName[100];
    CHAR subBankFileName[100];
    CHAR ccOutFileName[100];
    sprintf(tmpltBankFileName,"%s","in.xml");
    sprintf(sortedBankFileName,"%s","sorted.xml");
    sprintf(subBankFileName,"%s","sub.xml");
    sprintf(ccOutFileName,"%s","ccOut.xml");

    /* Constructs needed for writing xml output */
    LIGOLwXMLStream *xmlStreamOut;
    MetadataTable templateBank;
    SnglInspiralTable *tmplt  = NULL;

    /* Input to sub bank functions */
    InspiralTemplate *bankHead = NULL;
    InspiralTemplate *bank = NULL;
    UINT4 numberTemplates;
    UINT4 subBankSize = SUBBANKSIZE;
    UINT4 maxSubBankSize = SUBBANKSIZE;

    FindChirpSubBank *subBankHead = NULL;

    xmlStreamOut = XLALOpenLIGOLwXMLFile(sortedBankFileName);

    numberTemplates = InspiralTmpltBankFromLIGOLw( &bankHead, tmpltBankFileName,
						   STARTTEMPLATE,ENDTEMPLATE);

    /* Sort templates and create subbanks */
    bankHead = XLALFindChirpSortTemplates( bankHead, numberTemplates);
    
    tmplt = (SnglInspiralTable *) calloc(1,sizeof(SnglInspiralTable));
    templateBank.snglInspiralTable = tmplt;


    for ( bank = bankHead; bank; bank = bank->next)
    {
	tmplt->mass1 = bank->mass1;
	tmplt->mass2 = bank->mass2;

	tmplt->snr = 3.7;
	tmplt->chisq = 3.8;
	tmplt->chisq_dof = 100;
	tmplt->bank_chisq = 3.9;
	tmplt->bank_chisq_dof = 100;
	tmplt->cont_chisq = 4.0;
	tmplt->cont_chisq_dof = 100;

	tmplt->next = (SnglInspiralTable *) calloc(1,sizeof(SnglInspiralTable));
	tmplt = tmplt->next;
    }

    XLALWriteLIGOLwXMLSnglInspiralTable(xmlStreamOut,templateBank.snglInspiralTable);

    XLALCloseLIGOLwXMLFile(xmlStreamOut);    

    while ( templateBank.snglInspiralTable )
    {
	tmplt = templateBank.snglInspiralTable;
	templateBank.snglInspiralTable = templateBank.snglInspiralTable->next;
	LALFree( tmplt );
    }

    subBankHead = XLALFindChirpCreateSubBanks(&maxSubBankSize,
					      subBankSize,
					      numberTemplates,
					      bankHead);

    printf("maxsubbank size = %d\n",maxSubBankSize);





}
