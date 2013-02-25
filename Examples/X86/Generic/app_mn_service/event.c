/**
********************************************************************************
\file   event.c

\brief  MN Application event handler

This file contains a demo MN application event handler.

\ingroup module_demo_mn_console
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
#include <Epl.h>
#include <console/console.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// module global vars
//------------------------------------------------------------------------------
static DWORD*           pCycle_l;
static BOOL*            pfGsOff_l;

//------------------------------------------------------------------------------
// global function prototypes
//------------------------------------------------------------------------------

//============================================================================//
//            P R I V A T E   D E F I N I T I O N S                           //
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
// local types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------
static tEplKernel processStateChangeEvent(tEplApiEventType EventType_p,
                                          tEplApiEventArg* pEventArg_p,
                                          void GENERIC* pUserArg_p);

static tEplKernel processErrorWarningEvent(tEplApiEventType EventType_p,
                                           tEplApiEventArg* pEventArg_p,
                                           void GENERIC* pUserArg_p);

static tEplKernel processHistoryEvent(tEplApiEventType EventType_p,
                                      tEplApiEventArg* pEventArg_p,
                                      void GENERIC* pUserArg_p);

static tEplKernel processNodeEvent(tEplApiEventType EventType_p,
                                   tEplApiEventArg* pEventArg_p,
                                   void GENERIC* pUserArg_p);

#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_CFM)) != 0)
static tEplKernel processCfmProgressEvent(tEplApiEventType EventType_p,
                                          tEplApiEventArg* pEventArg_p,
                                          void GENERIC* pUserArg_p);

static tEplKernel processCfmResultEvent(tEplApiEventType EventType_p,
                                        tEplApiEventArg* pEventArg_p,
                                        void GENERIC* pUserArg_p);
#endif

//#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_CFM)) == 0)
static tEplKernel processSdoEvent(tEplApiEventType EventType_p,
                                  tEplApiEventArg* pEventArg_p,
                                  void GENERIC* pUserArg_p);
//#endif

//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//


//------------------------------------------------------------------------------
/**
\brief  Initialize applications event module

The function initializes the applications event module

\param  pCycle_p                Pointer to cycle time.
\param  pfGsOff_p               Pointer to GsOff flag (determines that stack is down)

\ingroup module_demo_mn_console
*/
//------------------------------------------------------------------------------

void initEvents (UINT* pCycle_p, BOOL* pfGsOff_p)
{
    pCycle_l = pCycle_p;
    pfGsOff_l = pfGsOff_p;

    mkfifo("/sdoFifo", S_IRUSR| S_IWUSR);

}

void cleanupEvents (void)
{

}

//------------------------------------------------------------------------------
/**
\brief  Process openPOWERLINK events

The function implements the applications stack event handler.

\param  EventType_p         Type of event
\param  pEventArg_p         Pointer to union which describes the event in detail
\param  pUserArg_p          User specific argument

\return The function returns a tEplKernel error code.

\ingroup module_demo_mn_console
*/
//------------------------------------------------------------------------------
tEplKernel PUBLIC processEvents(tEplApiEventType EventType_p,
                                tEplApiEventArg* pEventArg_p,
                                void GENERIC* pUserArg_p)
{
    tEplKernel          ret = kEplSuccessful;

    UNUSED_PARAMETER(pUserArg_p);

    // check if NMT_GS_OFF is reached
    switch (EventType_p)
    {
        case kEplApiEventNmtStateChange:
            ret = processStateChangeEvent(EventType_p, pEventArg_p, pUserArg_p);
            break;

        case kEplApiEventCriticalError:
        case kEplApiEventWarning:
            ret = processErrorWarningEvent(EventType_p, pEventArg_p, pUserArg_p);
            break;

        case kEplApiEventHistoryEntry:
            ret = processHistoryEvent(EventType_p, pEventArg_p, pUserArg_p);
            break;

        case kEplApiEventNode:
            ret = processNodeEvent(EventType_p, pEventArg_p, pUserArg_p);
            break;

#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_CFM)) != 0)
        case kEplApiEventCfmProgress:
            ret = processCfmProgressEvent(EventType_p, pEventArg_p, pUserArg_p);
            break;

        case kEplApiEventCfmResult:
            ret = processCfmResultEvent(EventType_p, pEventArg_p, pUserArg_p);
            break;
#endif

//#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_CFM)) == 0)
        // Configuration Manager is not available,
        // so process SDO events
        case kEplApiEventSdo:
            ret = processSdoEvent(EventType_p, pEventArg_p, pUserArg_p);
            break;
//#endif

        default:
            break;
    }
    return ret;
}

//============================================================================//
//            P R I V A T E   F U N C T I O N S                               //
//============================================================================//
/// \name Private Functions
/// \{

//------------------------------------------------------------------------------
/**
\brief  Process state change events

The function processes state change events.

\param  EventType_p         Type of event
\param  pEventArg_p         Pointer to union which describes the event in detail
\param  pUserArg_p          User specific argument

\return The function returns a tEplKernel error code.
*/
//------------------------------------------------------------------------------
static tEplKernel processStateChangeEvent(tEplApiEventType EventType_p,
                                          tEplApiEventArg* pEventArg_p,
                                          void GENERIC* pUserArg_p)
{
    UINT                        varLen;
    tEplKernel                  ret = kEplSuccessful;
    tEplEventNmtStateChange*    pNmtStateChange = &pEventArg_p->m_NmtStateChange;

    UNUSED_PARAMETER(EventType_p);
    UNUSED_PARAMETER(pUserArg_p);

    if (pCycle_l == NULL)
    {
        console_printlog("Applications event module isn't initialized!\n");
        return kEplSuccessful;
    }

    switch (pNmtStateChange->m_NewNmtState)
    {
        case kEplNmtGsOff:
           // NMT state machine was shut down,
            // because of user signal (CTRL-C) or critical EPL stack error
            // -> also shut down EplApiProcess() and main()
            ret = kEplShutdown;

            console_printlog("StateChangeEvent:kEplNmtGsOff originating event = 0x%X (%s)\n",
                     pNmtStateChange->m_NmtEvent,
                     EplGetNmtEventStr(pNmtStateChange->m_NmtEvent));

            // signal that stack is off
            *pfGsOff_l = FALSE;
            break;

        case kEplNmtGsResetCommunication:
#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_CFM)) == 0)
            ret = setDefaultNodeAssignment();
#endif
            console_printlog("StateChangeEvent(0x%X) originating event = 0x%X (%s)\n",
                   pNmtStateChange->m_NewNmtState,
                   pNmtStateChange->m_NmtEvent,
                   EplGetNmtEventStr(pNmtStateChange->m_NmtEvent));
            break;

        case kEplNmtGsResetConfiguration:
            if (*pCycle_l != 0)
            {
                ret = EplApiWriteLocalObject(0x1006, 0x00, pCycle_l, sizeof(DWORD));
            }
            else
            {
                varLen = sizeof(DWORD);
                EplApiReadLocalObject(0x1006, 0x00, pCycle_l, &varLen);
            }
            console_printlog("StateChangeEvent(0x%X) originating event = 0x%X (%s)\n",
                   pNmtStateChange->m_NewNmtState,
                   pNmtStateChange->m_NmtEvent,
                   EplGetNmtEventStr(pNmtStateChange->m_NmtEvent));
            break;

        case kEplNmtCsNotActive:
        case kEplNmtMsNotActive:
        case kEplNmtGsInitialising:
        case kEplNmtGsResetApplication:
        case kEplNmtCsPreOperational1:
        case kEplNmtMsPreOperational1:
        case kEplNmtCsPreOperational2:
        case kEplNmtMsPreOperational2:
        case kEplNmtCsReadyToOperate:
        case kEplNmtMsReadyToOperate:
        case kEplNmtCsBasicEthernet:
        case kEplNmtMsBasicEthernet:
            console_printlog("StateChangeEvent(0x%X) originating event = 0x%X (%s)\n",
                   pNmtStateChange->m_NewNmtState,
                   pNmtStateChange->m_NmtEvent,
                   EplGetNmtEventStr(pNmtStateChange->m_NmtEvent));

            break;

        default:
            break;
    }

    return ret;
}

//------------------------------------------------------------------------------
/**
\brief  Process error and warning events

The function processes error and warning events.

\param  EventType_p         Type of event
\param  pEventArg_p         Pointer to union which describes the event in detail
\param  pUserArg_p          User specific argument

\return The function returns a tEplKernel error code.
*/
//------------------------------------------------------------------------------
static tEplKernel processErrorWarningEvent(tEplApiEventType EventType_p,
                                           tEplApiEventArg* pEventArg_p,
                                           void GENERIC* pUserArg_p)
{
    // error or warning occurred within the stack or the application
    // on error the API layer stops the NMT state machine

    tEplEventError*         pInternalError = &pEventArg_p->m_InternalError;

    UNUSED_PARAMETER(EventType_p);
    UNUSED_PARAMETER(pUserArg_p);

    console_printlog("Err/Warn: Source = %s (%02X) EplError = %s (0x%03X)\n",
                EplGetEventSourceStr(pInternalError->m_EventSource),
                pInternalError->m_EventSource,
                EplGetEplKernelStr(pInternalError->m_EplError),
                pInternalError->m_EplError);

    FTRACE_MARKER("Err/Warn: Source = %s (%02X) EplError = %s (0x%03X)\n",
                EplGetEventSourceStr(pInternalError->m_EventSource),
                pInternalError->m_EventSource,
                EplGetEplKernelStr(pInternalError->m_EplError),
                pInternalError->m_EplError);

    // check additional argument
    switch (pInternalError->m_EventSource)
    {
        case kEplEventSourceEventk:
        case kEplEventSourceEventu:
            // error occurred within event processing
            // either in kernel or in user part
            console_printlog(" OrgSource = %s %02X\n",
                     EplGetEventSourceStr(pInternalError->m_Arg.m_EventSource),
                     pInternalError->m_Arg.m_EventSource);

            FTRACE_MARKER(" OrgSource = %s %02X\n",
                     EplGetEventSourceStr(pInternalError->m_Arg.m_EventSource),
                     pInternalError->m_Arg.m_EventSource);
            break;

        case kEplEventSourceDllk:
            // error occurred within the data link layer (e.g. interrupt processing)
            // the DWORD argument contains the DLL state and the NMT event
            console_printlog(" val = %X\n", pInternalError->m_Arg.m_dwArg);
            FTRACE_MARKER(" val = %X\n", pInternalError->m_Arg.m_dwArg);
            break;

        default:
            console_printlog("\n");
            break;
    }
    return kEplSuccessful;
}

//------------------------------------------------------------------------------
/**
\brief  Process history events

The function processes history events.

\param  EventType_p         Type of event
\param  pEventArg_p         Pointer to union which describes the event in detail
\param  pUserArg_p          User specific argument

\return The function returns a tEplKernel error code.
*/
//------------------------------------------------------------------------------
static tEplKernel processHistoryEvent(tEplApiEventType EventType_p,
                                      tEplApiEventArg* pEventArg_p,
                                      void GENERIC* pUserArg_p)
{
    tEplErrHistoryEntry*    pHistoryEntry = &pEventArg_p->m_ErrHistoryEntry;

    UNUSED_PARAMETER(EventType_p);
    UNUSED_PARAMETER(pUserArg_p);

    console_printlog("HistoryEntry: Type=0x%04X Code=0x%04X (0x%02X %02X %02X %02X %02X %02X %02X %02X)\n",
             pHistoryEntry->m_wEntryType, pHistoryEntry->m_wErrorCode,
            (WORD)pHistoryEntry->m_abAddInfo[0], (WORD)pHistoryEntry->m_abAddInfo[1],
            (WORD)pHistoryEntry->m_abAddInfo[2], (WORD)pHistoryEntry->m_abAddInfo[3],
            (WORD)pHistoryEntry->m_abAddInfo[4], (WORD)pHistoryEntry->m_abAddInfo[5],
            (WORD)pHistoryEntry->m_abAddInfo[6], (WORD)pHistoryEntry->m_abAddInfo[7]);

    FTRACE_MARKER("HistoryEntry:m_pUserArg Type=0x%04X Code=0x%04X (0x%02X %02X %02X %02X %02X %02X %02X %02X)\n",
            pHistoryEntry->m_wEntryType, pHistoryEntry->m_wErrorCode,
            (WORD)pHistoryEntry->m_abAddInfo[0], (WORD)pHistoryEntry->m_abAddInfo[1],
            (WORD)pHistoryEntry->m_abAddInfo[2], (WORD)pHistoryEntry->m_abAddInfo[3],
            (WORD)pHistoryEntry->m_abAddInfo[4], (WORD)pHistoryEntry->m_abAddInfo[5],
            (WORD)pHistoryEntry->m_abAddInfo[6], (WORD)pHistoryEntry->m_abAddInfo[7]);

    return kEplSuccessful;
}

//------------------------------------------------------------------------------
/**
\brief  Process node events

The function processes node events.

\param  EventType_p         Type of event
\param  pEventArg_p         Pointer to union which describes the event in detail
\param  pUserArg_p          User specific argument

\return The function returns a tEplKernel error code.
*/
//------------------------------------------------------------------------------
static tEplKernel processNodeEvent(tEplApiEventType EventType_p,
                                   tEplApiEventArg* pEventArg_p,
                                   void GENERIC* pUserArg_p)
{
    tEplApiEventNode*   pNode = &pEventArg_p->m_Node;

    UNUSED_PARAMETER(EventType_p);
    UNUSED_PARAMETER(pUserArg_p);

    // check additional argument
    switch (pNode->m_NodeEvent)
    {
        case kEplNmtNodeEventCheckConf:
            console_printlog("NodeEvent: (Node=%u, CheckConf)\n", pNode->m_uiNodeId);
            break;

        case kEplNmtNodeEventUpdateConf:
            console_printlog("NodeEvent: (Node=%u, UpdateConf)\n", pNode->m_uiNodeId);
            break;

        case kEplNmtNodeEventNmtState:
            console_printlog("NodeEvent: (Node=%u, NmtState=%s)\n",
                     pNode->m_uiNodeId,
                     EplGetNmtStateStr(pNode->m_NmtState));
            break;

        case kEplNmtNodeEventError:
            console_printlog("NodeEvent: (Node=%u): Error=%s (0x%.4X)\n",
                    pNode->m_uiNodeId,
                    EplGetEmergErrCodeStr(pNode->m_wErrorCode),
                    pNode->m_wErrorCode);
            break;

        case kEplNmtNodeEventFound:
            console_printlog("NodeEvent: (Node=%u, Found)\n", pNode->m_uiNodeId);
            break;

        default:
            break;
    }
    return kEplSuccessful;
}

#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_CFM)) != 0)
//------------------------------------------------------------------------------
/**
\brief  Process CFM progress events

The function processes CFM progress events.

\param  EventType_p         Type of event
\param  pEventArg_p         Pointer to union which describes the event in detail
\param  pUserArg_p          User specific argument

\return The function returns a tEplKernel error code.
*/
//------------------------------------------------------------------------------
static tEplKernel processCfmProgressEvent(tEplApiEventType EventType_p,
                                          tEplApiEventArg* pEventArg_p,
                                          void GENERIC* pUserArg_p)
{
    tEplCfmEventCnProgress*     pCfmProgress = &pEventArg_p->m_CfmProgress;

    UNUSED_PARAMETER(EventType_p);
    UNUSED_PARAMETER(pUserArg_p);

    console_printlog("CFM Progress: (Node=%u, CFM-Progress: Object 0x%X/%u, ",
                                                 pCfmProgress->m_uiNodeId,
                                                 pCfmProgress->m_uiObjectIndex,
                                                 pCfmProgress->m_uiObjectSubIndex);

    console_printlogadd("%lu/%lu Bytes", (ULONG)pCfmProgress->m_dwBytesDownloaded,
                            (ULONG)pCfmProgress->m_dwTotalNumberOfBytes);

    if ((pCfmProgress->m_dwSdoAbortCode != 0)
        || (pCfmProgress->m_EplError != kEplSuccessful))
    {
        console_printlogadd(" -> SDO Abort=0x%lX, Error=0x%X)\n",
               (ULONG) pCfmProgress->m_dwSdoAbortCode,
               pCfmProgress->m_EplError);
    }
    else
    {
        console_printlogadd(")\n");
    }
    return kEplSuccessful;
}

//------------------------------------------------------------------------------
/**
\brief  Process CFM result events

The function processes CFM result events.

\param  EventType_p         Type of event
\param  pEventArg_p         Pointer to union which describes the event in detail
\param  pUserArg_p          User specific argument

\return The function returns a tEplKernel error code.
*/
//------------------------------------------------------------------------------
static tEplKernel processCfmResultEvent(tEplApiEventType EventType_p,
                                        tEplApiEventArg* pEventArg_p,
                                        void GENERIC* pUserArg_p)
{
    tEplApiEventCfmResult*       pCfmResult = &pEventArg_p->m_CfmResult;

    UNUSED_PARAMETER(EventType_p);
    UNUSED_PARAMETER(pUserArg_p);

    switch (pCfmResult->m_NodeCommand)
    {
        case kEplNmtNodeCommandConfOk:
            console_printlog("CFM Result: (Node=%d, ConfOk)\n", pCfmResult->m_uiNodeId);
            break;

        case kEplNmtNodeCommandConfErr:
            console_printlog("CFM Result: (Node=%d, ConfErr)\n", pCfmResult->m_uiNodeId);
            break;

        case kEplNmtNodeCommandConfReset:
            console_printlog("CFM Result: (Node=%d, ConfReset)\n", pCfmResult->m_uiNodeId);
            break;

        case kEplNmtNodeCommandConfRestored:
            console_printlog("CFM Result: (Node=%d, ConfRestored)\n", pCfmResult->m_uiNodeId);
            break;

        default:
            console_printlog("CFM Result: (Node=%d, CfmResult=0x%X)\n", pCfmResult->m_uiNodeId,
                                                                pCfmResult->m_NodeCommand);
            break;
    }
    return kEplSuccessful;
}
#endif


//------------------------------------------------------------------------------
/**
\brief  Process SDO events

The function processes SDO events.

\param  EventType_p         Type of event
\param  pEventArg_p         Pointer to union which describes the event in detail
\param  pUserArg_p          User specific argument

\return The function returns a tEplKernel error code.
*/
//------------------------------------------------------------------------------
static tEplKernel processSdoEvent(tEplApiEventType EventType_p,
                                  tEplApiEventArg* pEventArg_p,
                                  void GENERIC* pUserArg_p)
{
    tEplSdoComFinished*       pSdo = &pEventArg_p->m_Sdo;
    tEplKernel                ret = kEplSuccessful;
    sem_t*                    sdoSem;

    UNUSED_PARAMETER(EventType_p);
    UNUSED_PARAMETER(pUserArg_p);

#if 0
    printf ("Sdo event! %p\n", pSdo->m_pUserArg);
    if (pSdo->m_pUserArg == (void *)0x15f4329a)
    {
        if ((sdoSem = sem_open("SdoSem", 0)) == SEM_FAILED)
        {
            fprintf (stderr, "%s() creating sem failed!\n", __func__);
            return kEplNoResource;
        }
        sem_post(sdoSem);
        sem_close(sdoSem);

        if ((ret = EplApiFreeSdoChannel(pSdo->m_SdoComConHdl)) != kEplSuccessful)
        {
            return ret;
        }
    }
#endif

    if (pSdo->m_pUserArg == (void *)0x15f4329a)
    {
        int fd;

        if ((fd = open("/sdoFifo", O_WRONLY)) == -1)
        {
            fprintf(stderr, "Error opening fifo\n");
            return ret;
        }
        write (fd, pSdo, sizeof(tEplSdoComFinished));
        close (fd);
    }

#if 0
    // SDO transfer finished
    if ((ret = EplApiFreeSdoChannel(pSdo->m_SdoAccessType)) != kEplSuccessful)
    {
        return ret;
    }

    if (pSdo->m_SdoComConState == kEplSdoComTransferFinished)
    {   // continue boot-up of CN with NMT command Reset Configuration
        ret = EplApiMnTriggerStateChange(pSdo->m_uiNodeId, kEplNmtNodeCommandConfReset);
    }
    else
    {   // indicate configuration error CN
        ret = EplApiMnTriggerStateChange(pSdo->m_uiNodeId, kEplNmtNodeCommandConfErr);
    }
#endif
    return ret;
}

#if (((EPL_MODULE_INTEGRATION) & (EPL_MODULE_CFM)) == 0)
//------------------------------------------------------------------------------
/**
\brief  Set default node assignment

Set default node assignment in object dictionary if configuration manager is
not available.

\return The function returns a tEplKernel error code.
*/
//------------------------------------------------------------------------------
static tEplKernel setDefaultNodeAssignment(void)
{
    tEplKernel  ret = kEplSuccessful;
    DWORD       nodeAssignment;

    nodeAssignment = (EPL_NODEASSIGN_NODE_IS_CN | EPL_NODEASSIGN_NODE_EXISTS);    // 0x00000003L
    ret = EplApiWriteLocalObject(0x1F81, 0x01, &nodeAssignment, sizeof (nodeAssignment));
    ret = EplApiWriteLocalObject(0x1F81, 0x02, &nodeAssignment, sizeof (nodeAssignment));
    ret = EplApiWriteLocalObject(0x1F81, 0x03, &nodeAssignment, sizeof (nodeAssignment));
    ret = EplApiWriteLocalObject(0x1F81, 0x04, &nodeAssignment, sizeof (nodeAssignment));
    ret = EplApiWriteLocalObject(0x1F81, 0x05, &nodeAssignment, sizeof (nodeAssignment));
    ret = EplApiWriteLocalObject(0x1F81, 0x06, &nodeAssignment, sizeof (nodeAssignment));
    ret = EplApiWriteLocalObject(0x1F81, 0x07, &nodeAssignment, sizeof (nodeAssignment));
    ret = EplApiWriteLocalObject(0x1F81, 0x08, &nodeAssignment, sizeof (nodeAssignment));
    ret = EplApiWriteLocalObject(0x1F81, 0x20, &nodeAssignment, sizeof (nodeAssignment));
    ret = EplApiWriteLocalObject(0x1F81, 0xFE, &nodeAssignment, sizeof (nodeAssignment));
    ret = EplApiWriteLocalObject(0x1F81, 0x6E, &nodeAssignment, sizeof (nodeAssignment));

    nodeAssignment = (EPL_NODEASSIGN_MN_PRES | EPL_NODEASSIGN_NODE_EXISTS);    // 0x00010001L
    ret = EplApiWriteLocalObject(0x1F81, 0xF0, &nodeAssignment, sizeof (nodeAssignment));
}
#endif

///\}




