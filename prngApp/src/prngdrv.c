#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>


#include <errlog.h>
#include <iocsh.h>
#include <cantProceed.h>
#include <epicsString.h>
#include <epicsTime.h>
#include <asynDriver.h>
#include <asynInt32.h>
#include <asynFloat64.h>
#include <asynInt32Array.h>
#include <asynDrvUser.h>

#include <epicsExport.h>

typedef struct {
    asynInterface common;
    asynInterface int32;
    asynInterface float64;
    asynInterface int32Array;
    asynInterface drvUser;
    unsigned int seed;
    char *portName;
} prngDrvPvt;

/* These functions are in public interfaces */
static asynStatus int32Write         (void *drvPvt, asynUser *pasynUser,
                                     epicsInt32 value);
static asynStatus int32Read         (void *drvPvt, asynUser *pasynUser,
                                     epicsInt32 *value);
static void prngReport               (void *drvPvt, FILE *fp, int details);
static asynStatus prngConnect        (void *drvPvt, asynUser *pasynUser);
static asynStatus prngDisconnect     (void *drvPvt, asynUser *pasynUser);

/*
 * asynCommon methods
 */
static struct asynCommon prngDrvCommon = {
    prngReport,
    prngConnect,
    prngDisconnect
};

/* asynInt32 methods */
static asynInt32 prngDrvInt32 = {
    int32Write,
    int32Read,
    NULL,
    NULL,
    NULL
};

int prngConfig(const char *portName, unsigned int seed)
{

    prngDrvPvt *pPvt;
    int status;
    pPvt = callocMustSucceed(1, sizeof(prngDrvPvt), "prngDrvPvt");
    pPvt->portName = epicsStrDup(portName);
    /*
     *  Link with higher level routines
     */
    pPvt->common.interfaceType = asynCommonType;
    pPvt->common.pinterface  = (void *)&prngDrvCommon;
    pPvt->common.drvPvt = pPvt;
    pPvt->int32.interfaceType = asynInt32Type;
    pPvt->int32.pinterface  = (void *)&prngDrvInt32;
    pPvt->int32.drvPvt = pPvt;
    pPvt->seed = seed;
    status = pasynManager->registerPort(pPvt->portName,
                                        ASYN_MULTIDEVICE | ASYN_CANBLOCK,
                                        1,  /* autoconnect */
                                        0,  /* medium priority */
                                        0); /* default stacksize */
    if (status != asynSuccess) {
        errlogPrintf("prngConfig ERROR: Can't register myself.\n");
        return -1;
    }
    status = pasynManager->registerInterface(pPvt->portName, &pPvt->common);
    if (status != asynSuccess) {
        errlogPrintf("prngConfig: Can't register common.\n");
        return -1;
    }
    status = pasynInt32Base->initialize(pPvt->portName, &pPvt->int32);
    if (status != asynSuccess) {
        errlogPrintf("prngConfig: Can't register int32.\n");
        return -1;
    }

    return(0);
}


/* Report  parameters */
static void prngReport(void *drvPvt, FILE *fp, int details)
{
    prngDrvPvt *pPvt = (prngDrvPvt *)drvPvt;

    //assert(pPvt);
    fprintf(fp, "prng %s: connected on drv device \n",
            pPvt->portName);
    if (details >= 1) {
        fprintf(fp, "              maxChans: \n");
    }
}

/* Connect */
static asynStatus prngConnect(void *drvPvt, asynUser *pasynUser)
{
    /* Does nothing for now.  
     * May be used if connection management is implemented */
    pasynManager->exceptionConnect(pasynUser);
    return(asynSuccess);
}

/* Connect */
static asynStatus prngDisconnect(void *drvPvt, asynUser *pasynUser)
{
    /* Does nothing for now.  
     * May be used if connection management is implemented */
    pasynManager->exceptionDisconnect(pasynUser);
    return(asynSuccess);
}

static asynStatus int32Read(void *drvPvt, asynUser *pasynUser,
                            epicsInt32 *value)
{
    prngDrvPvt *priv = (prngDrvPvt*)drvPvt;
    *value=rand_r(&priv->seed);
    return asynSuccess;
}

static asynStatus int32Write(void *drvPvt, asynUser *pasynUser,
                            epicsInt32 value)
{
    return asynSuccess;
}

/* iocsh functions */

static const iocshArg prngConfigArg0 = {"portName",iocshArgString};
static const iocshArg prngConfigArg1 = {"seed",iocshArgInt};
static const iocshArg * const prngConfigArgs[2] = {&prngConfigArg0,
                                                  &prngConfigArg1};
static const iocshFuncDef prngConfigDef = {"prngConfig",2,prngConfigArgs};
static void prngConfigFunc(const iocshArgBuf *args)
{
    prngConfig(args[0].sval, args[1].ival);
}

static void prngRegister(void)
{
    iocshRegister(&prngConfigDef,prngConfigFunc);
}

epicsExportRegistrar(prngRegister);
 

