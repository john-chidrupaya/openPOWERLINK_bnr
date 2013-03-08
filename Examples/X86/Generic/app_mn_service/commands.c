/**
********************************************************************************
\file   commands.c

\brief  {BRIEF_DESCRIPTION_OF_THE_FILE}

{DETAILED_DESCRIPTION_OF_THE_FILE}

\ingroup {MODULE_GROUP}
*******************************************************************************/

/*------------------------------------------------------------------------------
Copyright (c) 2013, Bernecker+Rainer Industrie-Elektronik Ges.m.b.H. (B&R)
Copyright (c) 2013, SYSTEC electronic GmbH
Copyright (c) 2013, Kalycito Infotech Private Ltd.All rights reserved.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holders nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
------------------------------------------------------------------------------*/

//------------------------------------------------------------------------------
// includes
//------------------------------------------------------------------------------
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include <Epl.h>
#include "local-types.h"
#include "parser.h"
#include "commands.h"
#include "app.h"

//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------
#define timespecadd(vvp, uvp)                                           \
        {                                                               \
                (vvp)->tv_sec += (uvp)->tv_sec;                         \
                (vvp)->tv_nsec += (uvp)->tv_nsec;                       \
                if ((vvp)->tv_nsec >= 1000000000) {                     \
                        (vvp)->tv_sec++;                                \
                        (vvp)->tv_nsec -= 1000000000;                   \
                }                                                       \
        }

//------------------------------------------------------------------------------
// module global vars
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// global function prototypes
//------------------------------------------------------------------------------


//============================================================================//
//            P R I V A T E   D E F I N I T I O N S                           //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// local types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------
static int waitSdoEvent(tEplSdoComFinished* comFinished_p);
static void printData(char* data_p, UINT32 len_p, tEplObdType type_p);
int parseData(char* dataStr_p, tEplObdType type_p, void* pData_p);

static BOOL exitApp(int argc_p, char** argv_p, UINT32 param_p);
static BOOL resetStack(int argc_p, char** argv_p, UINT32 param_p);
static BOOL startStack(int argc_p, char** argv_p, UINT32 param_p);
static BOOL cmdRunApp(int argc_p, char** argv_p, UINT32 param_p);
static BOOL stopStack(int argc_p, char** argv_p, UINT32 param_p);
static BOOL setCycleError(int argc_p, char** argv_p, UINT32 param_p);
static BOOL readSdo(int argc_p, char** argv_p, UINT32 param_p);
static BOOL writeSdo(int argc_p, char** argv_p, UINT32 param_p);
static BOOL readPi(int argc_p, char** argv_p, UINT32 param_p);
static BOOL writePi(int argc_p, char** argv_p, UINT32 param_p);

tCmdTbl commands_g[] =
{
    { "help",           "help",                                                     "Print this help",                      printHelp,              0},
    { "exit",           "exit",                                                     "Exit the application",                 exitApp,                0},
    { "start",          "start [<InputPiSize> <OutputPiSize>] [<CDC>]",             "Start stack operation",                startStack,             0},
    { "stop",           "stop",                                                     "Stop stack operation",                 stopStack,              0},
    { "run",            "run <1|0>",                                                "Run application code 1=enable 0=disable", cmdRunApp,           0},
    { "reset",          "reset",                                                    "Send NMT SW-Reset",                    resetStack,             0},
    { "cycleerr",       "cycleerr",                                                 "Set a cyle error",                     setCycleError,          0},
    { "sdoread",        "sdoread <DataType> <Node> <Index> <SubIndex>",             "Read an object through SDO",           readSdo,                0},
    { "sdowrite",       "sdowrite <DataType> <Node> <Index> <SubIndex> <Data>",     "Write an object through SDO",          writeSdo,               0},
    { "piread",         "piread <DataType> <Offset>",                               "Read data from input PI",              readPi,                 0},
    { "piwrite",        "pwrite <DataType> <Offset> <Data>",                        "Write data into output PI",            writePi,                0},

    { NULL,             NULL,                                                        NULL,                                  NULL,                   0},
};

//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//

//------------------------------------------------------------------------------
/**
\brief  Get pointer to command table

*/
//------------------------------------------------------------------------------
tCmdTbl* getCommands(void)
{
    return commands_g;
}

//============================================================================//
//            P R I V A T E   F U N C T I O N S                               //
//============================================================================//
/// \name Private Functions
/// \{


//------------------------------------------------------------------------------
/**
\brief  Print data
*/
//------------------------------------------------------------------------------
static void printData(char* data_p, UINT32 len_p, tEplObdType type_p)
{
    switch (type_p)
    {
        case kEplObdTypInt8:
            printf ("Data: %02Xh (%d)\n", *(INT8*)data_p, *(INT8*)data_p);
            break;

        case kEplObdTypUInt8:
            printf ("Data: %02Xh (%u)\n", *(UINT8*)data_p, *(UINT8*)data_p);
            break;

        case kEplObdTypInt16:
            printf ("Data: %04Xh (%d)\n", *(INT16*)data_p, *(INT16*)data_p);
            break;

        case kEplObdTypUInt16:
            printf ("Data: %04Xh (%u)\n", *(UINT16*)data_p, *(UINT16*)data_p);
            break;

        case kEplObdTypInt32:
            printf ("Data: %08Xh (%d)\n", *(INT32*)data_p, *(INT32*)data_p);
            break;

        case kEplObdTypUInt32:
            printf ("Data: %08Xh (%u)\n", *(UINT32*)data_p, *(UINT32*)data_p);
            break;

        case kEplObdTypInt64:
            printf ("Data: %0llXh (%lld)\n", *(long long*)data_p, *( long long*)data_p);
            break;

        case kEplObdTypUInt64:
            printf ("Data: %0llXh (%llu)\n", *(unsigned long long*)data_p, *(unsigned long long*)data_p);
            break;

        default:
            printf ("Unknown datatype!\n");
            break;
    }
}

//------------------------------------------------------------------------------
/**
\brief  Parse data
*/
//------------------------------------------------------------------------------
int parseData(char* dataStr_p, tEplObdType type_p, void* pData_p)
{
    int     base;

    if (strncmp(dataStr_p, "0x", 2) == 0)
        base = 16;
    else
        base = 10;

    switch (type_p)
    {
        case kEplObdTypInt8:
            *(INT8*)pData_p = (INT8)strtol(dataStr_p, NULL, base);
            break;
        case kEplObdTypUInt8:
            *(UINT8*)pData_p = (UINT8)strtoul(dataStr_p, NULL, base);
            break;
        case kEplObdTypInt16:
            *(INT16*)pData_p = (INT16)strtol(dataStr_p, NULL, base);
            break;
        case kEplObdTypUInt16:
            *(UINT16*)pData_p = (UINT16)strtoul(dataStr_p, NULL, base);
            break;
        case kEplObdTypInt32:
            *(INT32*)pData_p = (INT32)strtol(dataStr_p, NULL, base);
            break;
        case kEplObdTypUInt32:
            *(UINT32*)pData_p = (UINT32)strtoul(dataStr_p, NULL, base);
            break;
        case kEplObdTypInt64:
            *(unsigned long long*)pData_p = (unsigned long long)strtoll(dataStr_p, NULL, base);
            break;
        case kEplObdTypUInt64:
            *(unsigned long long*)pData_p = (unsigned long long)strtoull(dataStr_p, NULL, base);
            break;
        default:
            printf ("Invalid Data Type!\n");
            return -1;
            break;
    }
    return 0;
}

//------------------------------------------------------------------------------
/**
\brief  Parse data
*/
//------------------------------------------------------------------------------
int writePoData(int offset, char* dataStr_p, tEplObdType type_p)
{
    int     base;
    char*   pImage;

    if (strncmp(dataStr_p, "0x", 2) == 0)
        base = 16;
    else
        base = 10;

    pImage = getInputImage();

    switch (type_p)
    {
        case kEplObdTypInt8:
            *(INT8*)(pImage + offset) = (INT8)strtol(dataStr_p, NULL, base);
            break;
        case kEplObdTypUInt8:
            *(UINT8*)(pImage + offset)  = (UINT8)strtoul(dataStr_p, NULL, base);
            break;
        case kEplObdTypInt16:
            *(INT16*)(pImage + offset)  = (INT16)strtol(dataStr_p, NULL, base);
            break;
        case kEplObdTypUInt16:
            *(UINT16*)(pImage + offset)  = (UINT16)strtoul(dataStr_p, NULL, base);
            break;
        case kEplObdTypInt32:
            *(INT32*)(pImage + offset)  = (INT32)strtol(dataStr_p, NULL, base);
            break;
        case kEplObdTypUInt32:
            *(UINT32*)(pImage + offset)  = (UINT32)strtoul(dataStr_p, NULL, base);
            break;
        default:
            printf ("Invalid Data Type!\n");
            return -1;
            break;
    }
    return 0;
}


//------------------------------------------------------------------------------
/**
\brief  Wait for a sdo event

The function waits for a sync event.

\param  timeout_p       Specifies a timeout in microseconds. If 0 it waits
                        forever.

\return The function returns a tEplKernel error code.
\retval kEplSuccessful      Successfully received sync event
\retval kEplGeneralError    Error while waiting on sync event
*/
//------------------------------------------------------------------------------
static int waitSdoEvent(tEplSdoComFinished* comFinished_p)
{
    int fd;

    if ((fd = open ("/sdoFifo", O_RDONLY)) == -1)
    {
        return -1;
    }
    read (fd, comFinished_p, sizeof(tEplSdoComFinished));
    close (fd);
    return 0;
}

//==============================================================================
// Commands
//==============================================================================

//------------------------------------------------------------------------------
/**
\brief  Exit application

*/
//------------------------------------------------------------------------------
static BOOL exitApp(int argc_p, char** argv_p, UINT32 param_p)
{
    return TRUE;
}

//------------------------------------------------------------------------------
/**
\brief  Reset the stack (SW Reset)
*/
//------------------------------------------------------------------------------
static BOOL resetStack(int argc_p, char** argv_p, UINT32 param_p)
{
    tEplKernel          ret;
    UINT32              oid;
    UINT32              sub;
    UINT32              nodeId;
    UINT8               data;
    UINT32              len;
    tEplSdoComConHdl    comHdl;
    tEplSdoComFinished  comFinished;

    if (argc_p != 2)
    {
        printf ("Usage: reset <NODE>\n");
        return FALSE;
    }

    nodeId = strtoul(argv_p[1], NULL, 10);

    if (nodeId == 0)
    {
        ret = EplApiExecNmtCommand(kEplNmtEventSwReset);
    }
    else
    {
        oid = 0x1F9E;
        sub = 0;
        data = 0x28;
        len = 1;

        EPL_MEMSET(&comFinished, 0, sizeof(tEplSdoComFinished));
        if ((ret = EplApiWriteObject(&comHdl, nodeId, oid, sub, &data, len,
                                     kEplSdoTypeAsnd, (void *)0xdeadbeef)) == kEplApiTaskDeferred)
        {
            if (waitSdoEvent(&comFinished) == 0)
                printf ("Reset Command: Transfered %d Bytes - Abort Code:%08x (%s)\n",
                        comFinished.m_uiTransferredByte, comFinished.m_dwAbortCode,
                        EplGetAbortCodeStr(comFinished.m_dwAbortCode));
            else
                printf ("Write error!\n");
        }
    }
    return FALSE;
}


//------------------------------------------------------------------------------
/**
\brief  Run application
*/
//------------------------------------------------------------------------------
static BOOL cmdRunApp(int argc_p, char** argv_p, UINT32 param_p)
{
    tEplKernel          ret;
    UINT32              runFlag;

    if (argc_p != 2)
    {
        printf ("Usage: run <1|0>\n");
        return FALSE;
    }

    runFlag = strtoul(argv_p[1], NULL, 10);
    runApp((BOOL)runFlag);
    return FALSE;
}

//------------------------------------------------------------------------------
/**
\brief  Start the stack (SW Reset)
*/
//------------------------------------------------------------------------------
static BOOL startStack(int argc_p, char** argv_p, UINT32 param_p)
{
    tEplKernel  ret;
    UINT32      inSize;
    UINT32      outSize;
    static char cdcFile[256];
    FILE*       file;

    if ((argc_p != 3) && (argc_p != 4) && (argc_p != 1))
    {
        printf ("Usage: start [<InputPiSize> <OutputPiSize>] [<CDC>]\n");
        return FALSE;
    }

    if (argc_p == 4)
    {
        if (realpath(argv_p[3], cdcFile) != NULL)
        {
            //strncpy(cdcFile, argv_p[3], 256);

            if ((file = fopen(cdcFile, "r")) == NULL)
            {
                printf ("CDC file %s does not exist!\n", cdcFile);
                return FALSE;
            }
            fclose(file);

            ret = EplApiSetCdcFilename(cdcFile);
            if (ret != kEplSuccessful)
            {
                printf("EplApiSetCdcFilename() failed (Error:0x%x!\n", ret);
                return FALSE;
            }
        }
        else
        {
            printf ("Couldn't get real path of CDC file!\n");
            return FALSE;
        }
    }

    if ((argc_p == 3) || (argc_p == 4))
    {
        printf ("Reading sizes!\n");
        if (strncmp(argv_p[1], "0x", 2) == 0)
            inSize = (UINT32)strtoul(argv_p[1], NULL, 16);
        else
            inSize = (UINT32)strtoul(argv_p[1], NULL, 10);

        if (strncmp(argv_p[2], "0x", 2) == 0)
            outSize = (UINT32)strtoul(argv_p[2], NULL, 16);
        else
            outSize = (UINT32)strtoul(argv_p[2], NULL, 10);
    }
    else
    {
        inSize = getInputSize();
        outSize = getOutputSize();
    }

    if((ret = initApp(inSize, outSize)) != kEplSuccessful)
    {
        printf ("Couldn't init application!\n");
        return FALSE;
    }

    // start stack processing by sending a NMT reset command
    EplApiExecNmtCommand(kEplNmtEventSwReset);
    return FALSE;
}

//------------------------------------------------------------------------------
/**
\brief  Start the stack (SW Reset)
*/
//------------------------------------------------------------------------------
static BOOL stopStack(int argc_p, char** argv_p, UINT32 param_p)
{
    // start stack processing by sending a NMT reset command
    EplApiExecNmtCommand(kEplNmtEventSwitchOff);

    shutdownApp();
#ifdef CONFIG_POWERLINK_USERSTACK
    return FALSE;
#else
    /* kernel stack needs to be completely shut off! */
    return TRUE;
#endif
}

//------------------------------------------------------------------------------
/**
\brief  Simulate a cycle error

*/
//------------------------------------------------------------------------------
static BOOL setCycleError(int argc_p, char** argv_p, UINT32 param_p)
{
    tEplKernel  ret;

    ret = EplApiExecNmtCommand(kEplNmtEventNmtCycleError);
    if (ret != kEplSuccessful)
    {
        return TRUE;
    }
    return FALSE;
}

//------------------------------------------------------------------------------
/**
\brief  Do a SDO read
*/
//------------------------------------------------------------------------------
static BOOL readSdo(int argc_p, char**argv_p, UINT32 param_p)
{
    tEplKernel          ret;
    UINT32              oid;
    UINT32              sub;
    UINT32              nodeId;
    tEplSdoComConHdl    comHdl;
    tEplSdoComFinished  comFinished;
    char                data[256];
    UINT32              size;
    tEplObdType         type;
    size_t              len;

    if (argc_p != 5)
    {
        printf ("Usage: sdoread <DataType> <NODE> <Index> <SubIndex>\n");
        return FALSE;
    }

    nodeId = (UINT32)strtoul(argv_p[2], NULL, 10);
    if (strncmp(argv_p[3], "0x", 2) == 0)
        oid = (UINT32)strtoul(argv_p[3], NULL, 16);
    else
        oid = (UINT32)strtoul(argv_p[3], NULL, 10);
    sub = (UINT32)strtoul(argv_p[4], NULL, 10);
    parseDataType(argv_p[1], &type, &len);

    EPL_MEMSET(&comFinished, 0, sizeof(tEplSdoComFinished));
    size = sizeof(unsigned long long);
    ret = EplApiReadObject(&comHdl, nodeId, oid, sub, data, &size,
                           kEplSdoTypeAsnd, (void *)0x15f4329a);
    if (ret == kEplSuccessful)
    {
        printf( "Reading Node:%d, Index:%.4X, SubIndex:%d size:%d\n", nodeId, oid, sub, size);
        printData(data, size, type);
    }
    else if (ret == kEplApiTaskDeferred)
    {
        printf( "Reading Node:%d, Index:%.4X, SubIndex:%d size:%d ...\n",
                nodeId, oid, sub, size);
        if (waitSdoEvent(&comFinished) == 0)
        {
            printf ("Received %d Bytes - AbortCode:0x%08x (%s)\n",
                    comFinished.m_uiTransferredByte, comFinished.m_dwAbortCode,
                    EplGetAbortCodeStr(comFinished.m_dwAbortCode));
            if (comFinished.m_uiTransferredByte > 0)
                printData(data, comFinished.m_uiTransferredByte, type);
            else
                printf ("Read error (0 Bytes)!\n");
        }
        else
            printf ("Read error!\n");
    }
    else
        printf( "Node: %d\tIndex: %.4X\tSubIndex: %d:\tfailed with error %X\n",
                nodeId, oid, sub, ret);

    return FALSE;
}

//------------------------------------------------------------------------------
/**
\brief  Do a PI read
*/
//------------------------------------------------------------------------------
static BOOL readPi(int argc_p, char**argv_p, UINT32 param_p)
{
    tEplKernel          ret;
    UINT32              size;
    tEplObdType         type;
    size_t              len;
    int                 offset;
    char*               pImage;

    if (argc_p != 3)
    {
        printf ("Usage: piread <DataType> <Offset>\n");
        return FALSE;
    }

    parseDataType(argv_p[1], &type, &len);
    if (strncmp(argv_p[2], "0x", 2) == 0)
        offset = (UINT32)strtoul(argv_p[2], NULL, 16);
    else
        offset = (UINT32)strtoul(argv_p[2], NULL, 10);

    pImage = getOutputImage();
    printData(pImage + offset, len, type);

    return FALSE;
}

//------------------------------------------------------------------------------
/**
\brief  Do a SDO write
*/
//------------------------------------------------------------------------------
static BOOL writeSdo(int argc_p, char** argv_p, UINT32 param_p)
{
    tEplKernel          ret = kEplSuccessful;
    DWORD               oid;
    WORD                sub;
    UINT32              nodeId;
    tEplSdoComConHdl    comHdl;
    tEplSdoComFinished  comFinished;
    unsigned long long  data;
    tEplObdType         type;
    size_t              len;

    if (argc_p != 6)
    {
        printf ("Usage: sdowrite <DataType> <NODE> <Index> <SubIndex> <Data>\n");
        return FALSE;
    }
    parseDataType(argv_p[1], &type, &len);

    nodeId = (UINT32)strtoul(argv_p[2], NULL, 10);
    if (strncmp(argv_p[3], "0x", 2) == 0)
        oid = (UINT32)strtoul(argv_p[3], NULL, 16);
    else
        oid = (UINT32)strtoul(argv_p[3], NULL, 10);
    printf ("Write a SDO object\n");
    sub = (UINT32)strtoul(argv_p[4], NULL, 10);

    if (parseData(argv_p[5], type, &data) == -1)
        return FALSE;

    EPL_MEMSET(&comFinished, 0, sizeof(tEplSdoComFinished));
    if ((ret = EplApiWriteObject(&comHdl, nodeId, oid, sub, &data, len,
                                 kEplSdoTypeAsnd, (void *)0xdeadbeef)) == kEplSuccessful)
    {
        printf ("Data successfully written!\n");
    }
    else
    {
        if (ret == kEplApiTaskDeferred)
        {
            printf( "Writing Node:%d, Index:%.4X, SubIndex:%d ...\n",
                    nodeId, oid, sub);
            if (waitSdoEvent(&comFinished) == 0)
                printf ("Transfered %d Bytes - Abort Code:%08x (%s)\n",
                        comFinished.m_uiTransferredByte, comFinished.m_dwAbortCode,
                        EplGetAbortCodeStr(comFinished.m_dwAbortCode));
            else
                printf ("Write error!\n");
        }
    }

    return FALSE;
}

//------------------------------------------------------------------------------
/**
\brief  Do a PI write
*/
//------------------------------------------------------------------------------
static BOOL writePi(int argc_p, char** argv_p, UINT32 param_p)
{
    tEplKernel          ret = kEplSuccessful;
    UINT32              data;
    tEplObdType         type;
    size_t              len;
    int                 offset;
    char*               pImage;

    if (argc_p != 4)
    {
        printf ("Usage: pwrite <DataType> <Offset> <Data>\n");
        return FALSE;
    }

    parseDataType(argv_p[1], &type, &len);

    if (strncmp(argv_p[2], "0x", 2) == 0)
        offset = (UINT32)strtoul(argv_p[2], NULL, 16);
    else
        offset = (UINT32)strtoul(argv_p[2], NULL, 10);

    writePoData(offset, argv_p[3], type);

    return FALSE;
}

///\}







