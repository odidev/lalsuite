/*----------------------------------------------------------------------- 
 * 
 * File Name: FindChirpExch.c
 *
 * Author: Allen, B., Brown, D. A. and Creighton, J. D. E.
 * 
 * Revision: $Id$
 * 
 *-----------------------------------------------------------------------
 */

#include <lal/LALStdlib.h>
#include <lal/DataBuffer.h>
#include <lal/FindChirpExch.h>

NRCSID (FINDCHIRPEXCHC, "$Id$");

void
LALExchangeDataSegment (
    LALStatus      *status,
    DataSegment *segment,
    ExchParams  *exchParams
    )
{
  CHARVector box; /* a box to hold some bytes of data */

  INITSTATUS (status, "LALExchangeDataSegment", FINDCHIRPEXCHC);
  ATTATCHSTATUSPTR (status);

  /* only do a minimal check to see if arguments are somewhat reasonable */
  ASSERT (exchParams, status, FINDCHIRPEXCH_ENULL, FINDCHIRPEXCH_MSGENULL);
  ASSERT (segment, status, FINDCHIRPEXCH_ENULL, FINDCHIRPEXCH_MSGENULL);
  ASSERT (segment->data, status, FINDCHIRPEXCH_ENULL, FINDCHIRPEXCH_MSGENULL);
  ASSERT (segment->spec, status, FINDCHIRPEXCH_ENULL, FINDCHIRPEXCH_MSGENULL);
  ASSERT (segment->resp, status, FINDCHIRPEXCH_ENULL, FINDCHIRPEXCH_MSGENULL);

  if (exchParams->send)  /* I am sending */
  {
    INT4 dest = exchParams->partnerProcNum;

    /* stuff the data segment into a box */
    box.length = sizeof (DataSegment);
    box.data   = (CHAR *) segment;

    /* send box (this sends too much, but it is simple) */
    LALMPISendCHARVector (status->statusPtr, &box, dest, exchParams->mpiComm);
    CHECKSTATUSPTR (status);

    /* send pointer fields of data segment */

    /* data */
    LALMPISendINT2TimeSeries (status->statusPtr, segment->data, dest,
        exchParams->mpiComm);
    CHECKSTATUSPTR (status);

    /* spec */
    LALMPISendREAL4FrequencySeries (status->statusPtr, segment->spec, dest,
        exchParams->mpiComm);
    CHECKSTATUSPTR (status);

    /* resp */
    LALMPISendCOMPLEX8FrequencySeries (status->statusPtr, segment->resp, dest,
        exchParams->mpiComm);
    CHECKSTATUSPTR (status);
  }
  else /* I am receiving */
  {
    DataSegment tmpSegment;
    INT4        source = exchParams->partnerProcNum;

    box.length = sizeof (DataSegment);
    box.data   = (CHAR *) &tmpSegment;

    /* receive box */
    LALMPIRecvCHARVector (status->statusPtr, &box, source,
        exchParams->mpiComm);
    CHECKSTATUSPTR (status);

    /* set relevant fields */
    segment->endOfData = tmpSegment.endOfData;
    segment->newLock   = tmpSegment.newLock;
    segment->newCal    = tmpSegment.newCal;
    segment->number    = tmpSegment.number;

    /* receive remaining fields */

    /* data */
    LALMPIRecvINT2TimeSeries (status->statusPtr, segment->data, source,
        exchParams->mpiComm);
    CHECKSTATUSPTR (status);

    /* spec */
    LALMPIRecvREAL4FrequencySeries (status->statusPtr, segment->spec, source,
        exchParams->mpiComm);
    CHECKSTATUSPTR (status);

    /* resp */
    LALMPIRecvCOMPLEX8FrequencySeries (status->statusPtr, segment->resp, source,
        exchParams->mpiComm);
    CHECKSTATUSPTR (status);
  }

  DETATCHSTATUSPTR (status);
  RETURN (status);
}

void
LALExchangeInspiralBankIn (
    LALStatus         *status,
    InspiralBankIn *bankIn,
    ExchParams     *exchParams
    )
{
  CHARVector box; /* a box to hold some bytes of data */

  INITSTATUS (status, "LALExchangeInspiralBankIn", FINDCHIRPEXCHC);
  ATTATCHSTATUSPTR (status);

  /* only do a minimal check to see if arguments are somewhat reasonable */
  ASSERT (exchParams, status, FINDCHIRPEXCH_ENULL, FINDCHIRPEXCH_MSGENULL);
  ASSERT (bankIn, status, FINDCHIRPEXCH_ENULL, FINDCHIRPEXCH_MSGENULL);

  /* stuff the template bank input into a box */
  box.length = sizeof (InspiralBankIn);
  box.data   = (CHAR *) bankIn;

  if (exchParams->send)  /* I am sending */
  {
    INT4 dest = exchParams->partnerProcNum;

    /* send box */
    LALMPISendCHARVector (status->statusPtr, &box, dest, exchParams->mpiComm);
    CHECKSTATUSPTR (status);
  }
  else /* I am receiving */
  {
    INT4 source = exchParams->partnerProcNum;

    /* receive box */
    LALMPIRecvCHARVector (status->statusPtr, &box, source, exchParams->mpiComm);
    CHECKSTATUSPTR (status);
  }

  DETATCHSTATUSPTR (status);
  RETURN (status);
}


void
LALExchangeInspiralTemplate (
    LALStatus           *status,
    InspiralTemplate *tmplt,
    ExchParams       *exchParams
    )
{
  CHARVector box; /* a box to hold some bytes of data */

  INITSTATUS (status, "LALExchangeInspiralTemplate", FINDCHIRPEXCHC);
  ATTATCHSTATUSPTR (status);

  /* only do a minimal check to see if arguments are somewhat reasonable */
  ASSERT (exchParams, status, FINDCHIRPEXCH_ENULL, FINDCHIRPEXCH_MSGENULL);
  ASSERT (tmplt, status, FINDCHIRPEXCH_ENULL, FINDCHIRPEXCH_MSGENULL);

  /* stuff the template into a box */
  box.length = sizeof (InspiralTemplate);
  box.data   = (CHAR *) tmplt;

  if (exchParams->send)  /* I am sending */
  {
    INT4 dest = exchParams->partnerProcNum;

    /* send box */
    LALMPISendCHARVector (status->statusPtr, &box, dest, exchParams->mpiComm);
    CHECKSTATUSPTR (status);
  }
  else /* I am receiving */
  {
    INT4 source = exchParams->partnerProcNum;

    /* receive box */
    LALMPIRecvCHARVector (status->statusPtr, &box, source, exchParams->mpiComm);
    CHECKSTATUSPTR (status);
  }

  DETATCHSTATUSPTR (status);
  RETURN (status);
}


void
LALExchangeInspiralEvent (
    LALStatus        *status,
    InspiralEvent *event,
    ExchParams    *exchParams
    )
{
  CHARVector box; /* a box to hold some bytes of data */

  INITSTATUS (status, "LALExchangeInspiralEvent", FINDCHIRPEXCHC);
  ATTATCHSTATUSPTR (status);

  /* only do a minimal check to see if arguments are somewhat reasonable */
  ASSERT (exchParams, status, FINDCHIRPEXCH_ENULL, FINDCHIRPEXCH_MSGENULL);
  ASSERT (event, status, FINDCHIRPEXCH_ENULL, FINDCHIRPEXCH_MSGENULL);

  /* stuff the event into a box */
  box.length = sizeof (InspiralEvent);
  box.data   = (CHAR *) event;

  if (exchParams->send)  /* I am sending */
  {
    INT4 dest = exchParams->partnerProcNum;

    /* send box */
    LALMPISendCHARVector (status->statusPtr, &box, dest, exchParams->mpiComm);
    CHECKSTATUSPTR (status);
  }
  else /* I am receiving */
  {
    INT4 source = exchParams->partnerProcNum;

    /* receive box */
    LALMPIRecvCHARVector (status->statusPtr, &box, source, exchParams->mpiComm);
    CHECKSTATUSPTR (status);
  }

  DETATCHSTATUSPTR (status);
  RETURN (status);
}


void
LALExchangeInspiralEventList (
    LALStatus     *status,
    InspiralEvent **eventHead,
    ExchParams    *exchParams
    )
{
  InspiralEvent   *eventCurrent = NULL;
  InspiralEvent   *prev         = NULL;

  INITSTATUS (status, "LALExchangeInspiralEvent", FINDCHIRPEXCHC);
  ATTATCHSTATUSPTR (status);

  if (exchParams->send)  /* I am sending */
  {

    /* check that we have a bank to send */
    ASSERT (*eventHead, status, FINDCHIRPEXCH_ENULL, FINDCHIRPEXCH_MSGENULL);

    /* exchange the template bank */
    for (eventCurrent = *eventHead;
        eventCurrent != NULL;
        eventCurrent = eventCurrent->next
        )
    {
      LALExchangeInspiralEvent (status->statusPtr, eventCurrent, exchParams);
      CHECKSTATUSPTR (status);
    }

    ASSERT (!eventCurrent, status, FINDCHIRPEXCH_ENNUL, FINDCHIRPEXCH_MSGENNUL);

  }
  else /* I am receiving */
  {

    /* check that this is a new list */
    ASSERT (!*eventHead, status, FINDCHIRPEXCH_ENNUL, FINDCHIRPEXCH_MSGENNUL);

    /* recieve the template bank */
    do
    {
      /* create memory for the template */
      eventCurrent = (InspiralEvent *) LALMalloc (sizeof(InspiralEvent));

      /* make a note of the first node in the list to return */
      if ( *eventHead == NULL ) *eventHead = eventCurrent;

      /* recieve the template */
      LALExchangeInspiralEvent (status->statusPtr, eventCurrent, exchParams);
      CHECKSTATUSPTR (status);

      /* point the previous node to this node */
      if ( prev != NULL ) prev->next = eventCurrent;
      prev = eventCurrent;

    } while ( eventCurrent->next != NULL );

  }

  DETATCHSTATUSPTR (status);
  RETURN (status);
}

void
LALExchangeTemplateBank (
    LALStatus         *status,
    InspiralTemplate **tmpltHead,
    ExchParams        *exchParms
                 )
{
  InspiralTemplate   *tmpltCurrent = NULL;
  InspiralTemplate   *tmpltFine    = NULL;
  InspiralTemplate   *prev         = NULL;

  INITSTATUS (status, "LALExchangeTemplateBank", FINDCHIRPEXCHC);
  ATTATCHSTATUSPTR (status);

  if (exchParms->send)  /* I am sending */
  {

    /* check that we have a bank to send */
    ASSERT (*tmpltHead, status, FINDCHIRPEXCH_ENULL, FINDCHIRPEXCH_MSGENULL);

    /* exchange the template bank */
    for (tmpltCurrent = *tmpltHead;
        tmpltCurrent != NULL;
        tmpltCurrent = tmpltCurrent->next
        )
    {
      LALExchangeInspiralTemplate (status->statusPtr, tmpltCurrent, exchParms);
      CHECKSTATUSPTR (status);

      /* exchange the fine grids */
      if (tmpltCurrent->fine != NULL)
      {
        LALExchangeTemplateBank (status->statusPtr, &(tmpltCurrent->fine), exchParms);
        CHECKSTATUSPTR (status);
      }
    }

    ASSERT (!tmpltCurrent, status, FINDCHIRPEXCH_ENNUL, FINDCHIRPEXCH_MSGENNUL);

  }
  else /* I am receiving */
  {

    /* check that this is a new list */
    ASSERT (!*tmpltHead, status, FINDCHIRPEXCH_ENNUL, FINDCHIRPEXCH_MSGENNUL);

    /* recieve the template bank */
    do
    {
      /* create memory for the template */
      tmpltCurrent = (InspiralTemplate *) LALMalloc (sizeof(InspiralTemplate));

      /* make a note of the first node in the list to return */
      if ( *tmpltHead == NULL ) *tmpltHead = tmpltCurrent;

      /* recieve the template */
      LALExchangeInspiralTemplate (status->statusPtr, tmpltCurrent, exchParms);
      CHECKSTATUSPTR (status);

      /* get the fine grids */
      if (tmpltCurrent->fine != NULL)
      {
        tmpltFine = NULL;
        LALExchangeTemplateBank (status->statusPtr, &tmpltFine, exchParms);
        CHECKSTATUSPTR (status);

        /* set the fine pointer */
        tmpltCurrent->fine = tmpltFine;
      }

      /* point the previous node to this node */
      if ( prev != NULL ) prev->next = tmpltCurrent;
      prev = tmpltCurrent;

    } while ( tmpltCurrent->next != NULL );

  }

  DETATCHSTATUSPTR (status);
  RETURN (status);
}
