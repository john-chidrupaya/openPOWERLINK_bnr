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

#include <Epl.h>
#include "local-types.h"
#include "parser.h"
#include "commands.h"

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

static BOOL exitApp(int argc, char** argv);
static BOOL resetStack(int argc, char** argv);
static BOOL setCycleError(int argc, char** argv);
static BOOL readSdo(int argc, char** argv);
static BOOL writeSdo(int argc, char** argv);

tCmdTbl commands_g[] =
{
    { "help",           "Print this help",              printHelp},
    { "exit",           "Exit the application",         exitApp},
    { "reset",          "Send NMT SW-Reset",            resetStack},
    { "cycleerr",       "Set a cyle error",             setCycleError},
    { "sdoread",        "Read an object through SDO",   readSdo},
    { "sdowrite",       "Write an object through SDO",  writeSdo},
    { NULL,             NULL,                           NULL},
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

static void printData(UINT32 oid, UINT32 sub, char* data, UINT32 len)
{
    tEplObdType         type;

    EplObdGetType(oid, sub, &type);

    switch (type)
    {
        case kEplObdTypInt8:
            printf ("Data: %02Xh (%d)\n", *(INT8*)data);
            break;

        case kEplObdTypUInt8:
            printf ("Data: %02Xh (%d)\n", *(UINT8*)data);
            break;

        case kEplObdTypInt16:
            printf ("Data: %04Xh (%d)\n", *(INT16*)data);
            break;

        case kEplObdTypUInt16:
            printf ("Data: %04Xh (%d)\n", *(UINT16*)data);
            break;

        case kEplObdTypInt32:
            printf ("Data: %08Xh (%d)\n", *(INT32*)data);
            break;

        case kEplObdTypUInt32:
            printf ("Data: %08Xh (%d)\n", *(UINT32*)data);
            break;

        default:
            printf ("Unknown datatype!\n");
            break;
    }
}


static tEplKernel getData(UINT32 oid, UINT32 sub, UINT32* data, UINT32 *len)
{
    tEplKernel          ret = kEplSuccessful;
    tEplObdType         type;

    EplObdGetType(oid, sub, &type);

    switch (type)
    {
        case kEplObdTypInt8:
            *data = readUint("8-Bit Data", 0x00, 0xFF, 16);
            *len = 1;
            break;

        case kEplObdTypUInt8:
            *data = readUint("8-Bit Data", 0x00, 0xFF, 16);
            *len = 1;
            break;

        case kEplObdTypInt16:
            *data = readUint("16-Bit Data", 0x00, 0xFFFF, 16);
            *len = 2;
            break;

        case kEplObdTypUInt16:
            *data = readUint("16-Bit Data", 0x00, 0xFFFF, 16);
            *len = 2;
            break;

        case kEplObdTypInt32:
            *data = readUint("32-Bit Data", 0x00, 0xFFFFFFFF, 16);
            *len = 4;
            break;

        case kEplObdTypUInt32:
            *data = readUint("32-Bit Data", 0x00, 0xFFFFFFFF, 16);
            *len = 4;
            break;

        default:
            printf ("Unknown datatype!\n");
            ret = kEplApiInvalidParam;
            break;
    }
    return ret;
}



//------------------------------------------------------------------------------
/**
\brief  Exit application

*/
//------------------------------------------------------------------------------
static BOOL exitApp(int argc, char** argv)
{
    return TRUE;
}

//------------------------------------------------------------------------------
/**
\brief  Reset the stack (SW Reset)

*/
//------------------------------------------------------------------------------
static BOOL resetStack(int argc, char** argv)
{
    tEplKernel  ret;

    ret = EplApiExecNmtCommand(kEplNmtEventSwReset);
    if (ret != kEplSuccessful)
    {
        return TRUE;
    }
    return FALSE;
}

//------------------------------------------------------------------------------
/**
\brief  Simulate a cycle error

*/
//------------------------------------------------------------------------------
static BOOL setCycleError(int argc, char** argv)
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
static BOOL readSdo(int argc, char**argv)
{
    tEplKernel          ret;
    UINT32              oid;
    UINT32              sub;
    UINT32              nodeId;
    tEplSdoComConHdl    comHdl;
    tEplSdoComFinished  comFinished;
    char                data[256];
    UINT32              size;
    sem_t*              sdoSem;

    if (argc != 4)
    {
        printf ("Usage: sdoread <NODE> <Index> <SubIndex>\n");
        return FALSE;
    }

    nodeId = (UINT32)strtoul(argv[1], NULL, 10);
    if (strncmp(argv[2], "0x", 2) == 0)
        oid = (UINT32)strtoul(argv[2], NULL, 16);
    else
        oid = (UINT32)strtoul(argv[2], NULL, 10);

    sub = (UINT32)strtoul(argv[3], NULL, 10);

    printf ("Read a SDO object:\n");

    EPL_MEMSET(&comFinished, 0, sizeof(tEplSdoComFinished));

    if ((sdoSem = sem_open("SdoSem", O_CREAT, S_IRWXG, 0)) == SEM_FAILED)
    {
        fprintf (stderr, "%s() creating sem failed!\n", __func__);
        return FALSE;
    }

    size = sizeof(UINT32);
    ret = EplApiReadObject(&comHdl, nodeId, oid, sub, data, &size, kEplSdoTypeAsnd, (void *)0x15f4329a);
    if (ret == kEplSuccessful)
    {
        printf( "Reading Node:%d, Index:%.4X, SubIndex:%d\n",
                nodeId, oid, sub);
        printData(oid, sub, data, size);
    }
    else if (ret == kEplApiTaskDeferred)
    {
        printf( "Reading Node:%d, Index:%.4X, SubIndex:%d ...\n",
                nodeId, oid, sub);
        if (waitSdoEvent(&comFinished) == 0)
        {
            printf ("Received %d Bytes - AbortCode:0x%08X\n", comFinished.m_uiTransferredByte, comFinished.m_dwAbortCode);
            if (comFinished.m_uiTransferredByte > 0)
                printData(oid, sub, data, comFinished.m_uiTransferredByte);
            else
                printf ("Read error!\n");
        }
        else
            printf ("Read error!\n");
    }
    else
        printf( "Node: %d\tIndex: %.4X\tSubIndex: %d:\tfailed with error %X\n",
                nodeId, oid, sub, ret);

    sem_close(sdoSem);
    sem_unlink("SdoSem");
    return FALSE;
}

//------------------------------------------------------------------------------
/**
\brief  Do a SDO write

*/
//------------------------------------------------------------------------------
static BOOL writeSdo(int argc, char** argv)
{
    tEplKernel          ret;
    DWORD               oid;
    WORD                sub;
    UINT32              nodeId;
    tEplSdoComConHdl    comHdl;
    tEplSdoComFinished  comFinished;
    UINT32              data;
    UINT32              size;
    sem_t*              sdoSem;


    printf ("Write a SDO object\n");
    nodeId  = readUint("NodeID", 0, 239, 10);
    oid = readUint("Object Index", 0x1000, 0xFFFF, 16);
    sub = readUint("SubIndex", 0, 255, 10);

    if((ret = getData(oid, sub, &data, &size)) != kEplSuccessful)
    {
        return FALSE;
    }

    if ((sdoSem = sem_open("SdoSem", O_CREAT, S_IRWXG, 0)) == SEM_FAILED)
    {
        fprintf (stderr, "%s() creating sem failed!\n", __func__);
        return FALSE;
    }

    EPL_MEMSET(&comFinished, 0, sizeof(tEplSdoComFinished));

    if ((ret = EplApiWriteObject(&comHdl, nodeId, oid, sub, &data, size, kEplSdoTypeAsnd, (void *)0xdeadbeef)) == kEplSuccessful)
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
            {
                printf ("Transfered %d Bytes:\n", comFinished.m_uiTransferredByte);
                printf ("Abort Code: %08x\n", comFinished.m_dwAbortCode);
                if (comFinished.m_uiTransferredByte > 0)
                    printf ("Ok!\n");
                else
                    printf ("Write error!\n");
            }
            else
            {
                printf ("Write error!\n");
            }
        }
    }

    sem_close(sdoSem);
    sem_unlink("SdoSem");
    return FALSE;
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

///\}







