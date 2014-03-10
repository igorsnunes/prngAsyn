#include <stdlib.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <aiRecord.h>

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <osiUnistd.h>
#include <osiSock.h>
#include <cantProceed.h>
#include <errlog.h>
#include <iocsh.h>
#include <epicsAssert.h>
#include <epicsExit.h>
#include <epicsStdio.h>
#include <epicsString.h>
#include <epicsThread.h>
#include <epicsTime.h>
#include <osiUnistd.h>
#include "asynInt32.h"
#include <epicsExport.h>
#include "asynDriver.h"
#include "asynOctet.h"
#include "asynInterposeCom.h"
#include "asynInterposeEos.h"
#include <asynEpicsUtils.h>

static long init_record(aiRecord *pao);
static long read_ai(aiRecord *pao);

typedef struct {
    asynUser *pasynUser;
    asynInt32 *pasynInt32;
    void *asynInt32Pvt;
    size_t nread;
    int *data;
} prngPvt;



struct {
  long num;
  DEVSUPFUN  report;
  DEVSUPFUN  init;
  DEVSUPFUN  init_record;
  DEVSUPFUN  get_ioint_info;
  DEVSUPFUN  read_ai;
  DEVSUPFUN  special_linconv;
} devAiPrng = {
  6, /* space for 6 functions */
  NULL,
  NULL,
  init_record,
  NULL,
  read_ai,
  NULL
};
epicsExportAddress(dset,devAiPrng);

static long init_record(aiRecord *pao)
{

    asynUser *pasynUser;
    char *port, *userParam;
    asynStatus status;
    asynInterface *pasynInterface;
    prngPvt *pPvt;
    int addr;

    pPvt = callocMustSucceed(1, sizeof(prngPvt), "devMcaAsyn init_record()");
    /* Create asynUser */
    pasynUser = pasynManager->createAsynUser(0, 0);
    pasynUser->userPvt = pPvt;
    pPvt->pasynUser = pasynUser;
    pao->dpvt = pPvt;

    status = pasynEpicsUtils->parseLink(pasynUser, &pao->inp,
                                    &port, &addr, &userParam);

    if (status != asynSuccess) {
        errlogPrintf("devMcaAsyn::init_record  bad link %s\n",
                      pasynUser->errorMessage);
        return 1;
    }

    /* Connect to device */
    status = pasynManager->connectDevice(pasynUser, port, addr);
    if (status != asynSuccess) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                  "devMcaAsyn::init_record, connectDevice failed to %s\n",
                   port);
        return 1;
    }

    /* Get the asynInt32 interface */
    pasynInterface = pasynManager->findInterface(pasynUser, asynInt32Type, 1);
    if (!pasynInterface) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,"devMcaAsyn::init_record, find int32 interface failed\n");
        return 1;
    }
    pPvt->pasynInt32 = (asynInt32 *)pasynInterface->pinterface;
    pPvt->asynInt32Pvt = pasynInterface->drvPvt;

	return 0;
}
static long read_ai(aiRecord *pao)
{
	prngPvt *pPvt = (prngPvt *)pao->dpvt;
	pPvt = (prngPvt *)pPvt->pasynUser->userPvt;
	pPvt->pasynInt32->read(pPvt->asynInt32Pvt, pPvt->pasynUser,&pao->rval);
	return 0;
}
