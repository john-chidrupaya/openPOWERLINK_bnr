/****************************************************************************

  (c) Bernecker + Rainer Industrie-Elektronik Ges.m.b.H.
      B&R Strasse 1, A-5142 Eggelsberg
      www.br-automation.com

  Project:      openPOWERLINK

  Description:  Provide some debug functions to test features

  License:

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    3. Neither the name of the copyright holders nor the names of its
       contributors may be used to endorse or promote products derived
       from this software without prior written permission. For written
       permission, please contact info@systec-electronic.com.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

    Severability Clause:

        If a provision of this License is or becomes illegal, invalid or
        unenforceable in any jurisdiction, that shall not affect:
        1. the validity or enforceability in that jurisdiction of any other
           provision of this License; or
        2. the validity or enforceability in other jurisdictions of that or
           any other provision of this License.

****************************************************************************/


/***************************************************************************/

/* includes */
#include "stdio.h"

#include "Epl.h"

#include "debugFunctions.h"
#include "EplTgtConio.h"

/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          G L O B A L   D E F I N I T I O N S                            */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

#define StoreObdID          0x1010
#define LoadObdID           0x1011

#define StoreSignature      0x65766173       // 'S' 'A' 'V' 'E'
#define LoadSignature       0x64616F6C       // 'L' 'O' 'A' 'D'


//---------------------------------------------------------------------------
// local types
//---------------------------------------------------------------------------

typedef struct tNmtCmd  tNmtCmd;
struct tNmtCmd
{
    char    Key;
    BYTE    Cmd;
    char    Name[ 30 ];
};

typedef struct
{
    unsigned    int     uiIndex;
    unsigned    int     uiSubIndex;
    unsigned    int     uiSize;
    DWORD               ReadData;
    DWORD               WriteData;
    char                Name[30];
} tParamStoreCap;

typedef  struct
{
    unsigned    int     StoreCapsID;
    tObdTransfer        ObtTransfer;
} tDebugInst;

typedef struct
{
    char    ShortCut;
    char    Entry[30];
} tMenuEntry;

//---------------------------------------------------------------------------
// module global vars
//---------------------------------------------------------------------------

tDebugInitParam     DebugParam_g;

tNmtCmd ExtCmds[]   =
{
    {   '-',    0x00,   "----------------"  },
    {   '1',    0x41,   "StartNodeEx"       },
    {   '2',    0x42,   "StopNodeEx"        },
    {   '3',    0x43,   "EnterPreop2Ex"     },
    {   '4',    0x44,   "EnableReadyToOpEx" },
    {   '-',    0x00,   "----------------"  },
    {   '5',    0x48,   "ResetNodeEx"       },
    {   '6',    0x49,   "ResetCommEx"       },
    {   '7',    0x4A,   "ResetConfEx"       },
    {   '8',    0x4B,   "SwResetEx"         },
    {   '-',    0x00,   "----------------"  },
    {   '-',    0x00,   "----------------"  },
};

tNmtCmd PlainCmds[]   =
{
    {   '-',    0x00,   "----------------"  },
    {   '1',    0x21,   "StartNode"         },
    {   '2',    0x22,   "StopNode"          },
    {   '3',    0x23,   "EnterPreop2"       },
    {   '4',    0x24,   "EnableReadyToOp"   },
    {   '-',    0x00,   "----------------"  },
    {   '5',    0x28,   "ResetNode"         },
    {   '6',    0x29,   "ResetComm"         },
    {   '7',    0x2A,   "ResetConf"         },
    {   '8',    0x2B,   "SwReset"           },
    {   '-',    0x00,   "----------------"  },
    {   '-',    0x00,   "----------------"  },
};

unsigned int ExtCmdCnt      = sizeof(ExtCmds) / sizeof(ExtCmds[0]);
unsigned int PlainCmdCnt    = sizeof(PlainCmds) / sizeof(PlainCmds[0]);

tParamStoreCap  ParamStoreCaps[]    =
{//     Index           SubId   Size    ReadData    WriteData           Name
    {   StoreObdID,     0x01,   4,      0x0000,     StoreSignature,     "AllParam_U32"                  },
    {   StoreObdID,     0x02,   4,      0x0000,     StoreSignature,     "CommunicationParam_U32"        },
    {   StoreObdID,     0x03,   4,      0x0000,     StoreSignature,     "ApplicationParam_U32"          },
    {   StoreObdID,     0x04,   4,      0x0000,     StoreSignature,     "ManufacturerParam_01h_U32"     },
    {   StoreObdID,     0x05,   4,      0x0000,     StoreSignature,     "ManufacturerParam_02h_U32"     },
    {   StoreObdID,     0x06,   4,      0x0000,     StoreSignature,     "ManufacturerParam_03h_U32"     },
    {   StoreObdID,     0x07,   4,      0x0000,     StoreSignature,     "ManufacturerParam_04h_U32"     },
    {   StoreObdID,     0x08,   4,      0x0000,     StoreSignature,     "ManufacturerParam_05h_U32"     },
    {   StoreObdID,     0x09,   4,      0x0000,     StoreSignature,     "ManufacturerParam_06h_U32"     },

    {   LoadObdID,      0x01,   4,      0x0000,     LoadSignature,      "AllParam_U32"                  },
    {   LoadObdID,      0x02,   4,      0x0000,     LoadSignature,      "CommunicationParam_U32"        },
    {   LoadObdID,      0x03,   4,      0x0000,     LoadSignature,      "ApplicationParam_U32"          },
    {   LoadObdID,      0x04,   4,      0x0000,     LoadSignature,      "ManufacturerParam_01h_U32"     },
    {   LoadObdID,      0x05,   4,      0x0000,     LoadSignature,      "ManufacturerParam_02h_U32"     },
    {   LoadObdID,      0x06,   4,      0x0000,     LoadSignature,      "ManufacturerParam_03h_U32"     },
    {   LoadObdID,      0x07,   4,      0x0000,     LoadSignature,      "ManufacturerParam_04h_U32"     },
    {   LoadObdID,      0x08,   4,      0x0000,     LoadSignature,      "ManufacturerParam_05h_U32"     },
    {   LoadObdID,      0x09,   4,      0x0000,     LoadSignature,      "ManufacturerParam_06h_U32"     },
};

tMenuEntry   ReStoreMenu[]  =
{
    {   's',    "Store",    },
    {   'r',    "Restore",  },
};

tMenuEntry   ParamMenu[]  =
{
    {   'a',    "All",              },
    {   'c',    "Communication",    },
    {   'p',    "App",              },
    {   'm',    "Manufacturar",     },
};

tDebugInst  DebugInst;

/*----------------------------------------------------------------------------*/
/* application defines and variables */


BYTE                        abCmdData[32];

/*----------------------------------------------------------------------------*/

//---------------------------------------------------------------------------
// local function prototypes
//---------------------------------------------------------------------------
tEplKernel          EplDebugReadObj( tObdTransfer   *pObdTrans );

unsigned int    EplDebugReadUint(  char*           strName_p,
                                    unsigned    int uiLow_p,
                                    unsigned    int uiHigh_p,
                                                int Base_p
                                    );

unsigned int    EplDebugSelectMenu( tMenuEntry Menu[],
                                    unsigned int Cnt,
                                    char MenuName[] );

//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
void EplDebugInit
(
        tDebugInitParam*      pDebugInit_p
)
{
    DebugParam_g.m_pAppPI_In        = pDebugInit_p->m_pAppPI_In;
    DebugParam_g.m_pAppPI_Out       = pDebugInit_p->m_pAppPI_Out;
    DebugParam_g.m_pAppPCopyJob     = pDebugInit_p->m_pAppPCopyJob;

#ifndef CONFIG_POWERLINK_USERSTACK
    DebugParam_g.m_pEventThreadId   = pDebugInit_p->m_pEventThreadId;
    DebugParam_g.m_pSyncThreadId    = pDebugInit_p->m_pSyncThreadId;

    DebugParam_g.m_pfnEventThread   = pDebugInit_p->m_pfnEventThread;
    DebugParam_g.m_pfnSyncThread    = pDebugInit_p->m_pfnSyncThread;
#endif
}

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
unsigned int EplDebugReadUint(  char*           strName_p,
                                unsigned    int uiLow_p,
                                unsigned    int uiHigh_p,
                                            int Base_p
                                )
{
    unsigned    int     uiInput     = 0;
    unsigned    int     fInputValid = 0;
    char                Buffer[ 30 ];
    char*               pEnd;


    fInputValid  = 0;
    do
    {
        printf( "Enter %s (Base %d):\n", strName_p, Base_p );

        fgets( Buffer, sizeof(Buffer), stdin );

        uiInput = (unsigned int) strtoul( Buffer, &pEnd, Base_p );

        if
        (
            ( uiInput >= uiLow_p   ) &&
            ( uiInput <= uiHigh_p  )
        )
        {
            fInputValid  = 1;
        }
    }
    while( 1 != fInputValid );

    while( EplTgtKbhit() )
    {
        EplTgtGetch();
    }

    return  uiInput;
}

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
void EplDebugSetNode( BYTE CmdData [ 32 ], unsigned int NodeId )
{
    unsigned int Offset;
    unsigned int Shift;

    Offset  = NodeId / 8;
    Shift   = NodeId % 8;

    CmdData[ Offset ]   |= (1 << Shift);
}

DWORD               NMT_FeatureFlags;

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
tEplKernel  EplDebugSendNmtCmdEx()
{
    tEplKernel          Ret             = kEplSuccessful;
    unsigned    int     EnableNode_1    = 1;
    unsigned    int     EnableNode_32   = 0;
    unsigned    int     EnableNode_110  = 0;
    char                cKey            = 0;
    unsigned    int     CmdId           = 0;
    unsigned    int     InputValid      = 0;
    unsigned    int     MoreInput       = 0;
    unsigned    int     i;
    BYTE                NodeIdBrdCst    = EPL_C_ADR_BROADCAST;
    BYTE                CmdData [ 32 ]  = { 0 };
    BYTE                Enable          = 1;

    unsigned    int     NmtFF_Size;
    tEplSdoType         SdoType;


    printf( "Send extended command to nodes:\n");

    // Get Nodes
    printf( "Toggle nodes:\n"   );
    printf( "Key\tNodeId\n"     );
    printf( "1\t1\n"            );
    printf( "2\t32\n"           );
    printf( "3\t110\n"          );
    printf( "Other\tAccept\n"   );

    printf( "Current setting:\n");
    printf( "1: %s\t32: %s\t110: %s\t\n",   EnableNode_1    == 1 ? "On" : "Off",
                                            EnableNode_32   == 1 ? "On" : "Off",
                                            EnableNode_110  == 1 ? "On" : "Off");

    MoreInput   = 1;
    do
    {
        if( EplTgtKbhit() )
        {
            cKey = EplTgtGetch();

            switch (cKey)
            {
                case '1':   EnableNode_1    = !EnableNode_1;
                            break;

                case '2':   EnableNode_32   = !EnableNode_32;
                            break;

                case '3':   EnableNode_110  = !EnableNode_110;
                            break;

                case 0x0A:
                {
                    // Ignore newline
                    break;
                }

                default:    MoreInput       = 0;
                            break;
            }

            switch (cKey)
            {
                case '1':
                case '2':
                case '3':
                    printf( "1: %s\t32: %s\t110: %s\t\n",   EnableNode_1    == 1 ? "On" : "Off",
                                                            EnableNode_32   == 1 ? "On" : "Off",
                                                            EnableNode_110  == 1 ? "On" : "Off");

                    break;

                default:
                            break;
            }
        }
    }
    while( MoreInput == 1 );

    if( 1 == EnableNode_1 )
    {
        EplDebugSetNode( CmdData, 1 );
    }

    if( 1 == EnableNode_32 )
    {
        EplDebugSetNode( CmdData, 32 );
    }

    if( 1 == EnableNode_110 )
    {
        EplDebugSetNode( CmdData, 110 );
    }

    printf( "CmdData:\n");
    for( i = 0; i < 32; i ++ )
    {
//        CmdData[i]  = 0xFF;

        printf( "%.2X ", CmdData[i] );

        if( (i+1) % 8 == 0 )
        {
            printf( "\n" );
        }
    }
    printf( "\n" );

    printf( "Select command:\n"         );
    for( i = 0; i < ExtCmdCnt; i ++ )
    {
        if( '-' == ExtCmds[i].Key )
        {
            printf( "\n" );
        }
        else
        {
            printf( "%c\t%s\t", ExtCmds[i].Key, ExtCmds[i].Name);
        }
    }

    // Get Command
    do
    {
        if( EplTgtKbhit() )
        {
            cKey = EplTgtGetch();

            InputValid  = 0;

            for( i = 0; i < ExtCmdCnt; i++ )
            {
                if( ExtCmds[i].Key == cKey )
                {
                    InputValid  = 1;
                    CmdId       = i;
                    break;
                }
            }
        }
    }
    while( InputValid != 1 );

    printf( "Sending command %s\n", ExtCmds[CmdId].Name );
    printf( "Node1:   %s\n",   EnableNode_1    == 1 ? "On" : "Off" );
    printf( "Node32:  %s\n",   EnableNode_32    == 1 ? "On" : "Off" );
    printf( "Node110: %s\n",   EnableNode_110    == 1 ? "On" : "Off" );

    // Check if nodes support extended commands
    NmtFF_Size  = sizeof(NMT_FeatureFlags);
    SdoType     = kEplSdoTypeAsnd;
    // 0x01     Release_BOOL
    // 0x02     CmdID_U8
    // 0x03     CmdTarget_U8
    // 0x04     CmdData_DOM

    // Write ObjDict entries
    Ret = EplApiWriteLocalObject(0x1F9F, 0x02, &ExtCmds[CmdId].Cmd,   sizeof(ExtCmds[CmdId].Cmd));
    if( kEplSuccessful != Ret )
    {
        printf( "SubIndex 2 failed\n");
        goto Exit;
    }

    Ret = EplApiWriteLocalObject(0x1F9F, 0x03, &NodeIdBrdCst,       sizeof(NodeIdBrdCst) );
    if( kEplSuccessful != Ret )
    {
        printf( "SubIndex 3 failed\n");
        goto Exit;
    }

    Ret = EplApiWriteLocalObject(0x1F9F, 0x04, &CmdData,            sizeof(CmdData));
    if( kEplSuccessful != Ret )
    {
        printf( "SubIndex 4 failed\n");
        goto Exit;
    }

    Ret = EplApiWriteLocalObject(0x1F9F, 0x01, &Enable,             sizeof(Enable));
    if( kEplSuccessful != Ret )
    {
        printf( "SubIndex 1 failed\n");
        goto Exit;
    }

Exit:
    Ret = kEplSuccessful;

    return  Ret;
}

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
tEplKernel  EplDebugSendNmtCmd()
{
    tEplKernel          Ret             = kEplSuccessful;
    char                cKey            = 0;
    unsigned    int     CmdId           = 0;
    unsigned    int     InputValid      = 0;
    unsigned    int     i;
    BYTE                NodeId;
    BYTE                Enable          = 1;

    printf( "Send plain command to nodes:\n");

    // Get NodeID
    NodeId  = EplDebugReadUint( "NodeID", 1, 255, 10 );

    printf( "Select command:\n"         );
    for( i = 0; i < PlainCmdCnt; i ++ )
    {
        if( '-' == PlainCmds[i].Key )
        {
            printf( "\n" );
        }
        else
        {
            printf( "%c\t%s\t", PlainCmds[i].Key, PlainCmds[i].Name);
        }
    }

    // Get Command
    InputValid  = 0;
    do
    {
        if( EplTgtKbhit() )
        {
            cKey = EplTgtGetch();

            InputValid  = 0;

            for( i = 0; i < PlainCmdCnt; i++ )
            {
                if( PlainCmds[i].Key == cKey )
                {
                    InputValid  = 1;
                    CmdId       = i;
                    break;
                }
            }
        }
    }
    while( InputValid != 1 );

    printf( "Sending command %s to Node %d\n", PlainCmds[CmdId].Name, NodeId );

    // 0x01     Release_BOOL
    // 0x02     CmdID_U8
    // 0x03     CmdTarget_U8
    // 0x04     CmdData_DOM

    // Write ObjDict entries
    Ret = EplApiWriteLocalObject(0x1F9F, 0x02, &PlainCmds[CmdId].Cmd,   sizeof(PlainCmds[CmdId].Cmd));
    if( kEplSuccessful != Ret )
    {
        printf( "SubIndex 2 failed\n");
        goto Exit;
    }

    Ret = EplApiWriteLocalObject(0x1F9F, 0x03, &NodeId,       sizeof(NodeId) );
    if( kEplSuccessful != Ret )
    {
        printf( "SubIndex 3 failed\n");
        goto Exit;
    }

    Ret = EplApiWriteLocalObject(0x1F9F, 0x01, &Enable,             sizeof(Enable));
    if( kEplSuccessful != Ret )
    {
        printf( "SubIndex 1 failed\n");
        goto Exit;
    }

Exit:
    Ret = kEplSuccessful;

    return  Ret;
}

void EplDebugPrintMnNmtState()
{
    tEplKernel      Ret;
    BYTE            MnNmtState;
    tEplObdSize     MnNmtStateSize;

    MnNmtStateSize  = sizeof(MnNmtState);
    Ret = EplApiReadLocalObject( 0x1F8E, 240, &MnNmtState, &MnNmtStateSize );

    printf( "MnNmtState: %.2X\n", MnNmtState );

    if( kEplSuccessful == Ret )
    {
        printf( "MN Nmt-State (0x1F8E/240):\t%s\n", EplGetNmtStateStr(EPL_NMT_TYPE_MS | MnNmtState) );
    }
    else
    {
        printf( "Reading MN Nmt-State(0x1F8E/240) failed.\n" );
    }

    MnNmtStateSize  = sizeof(MnNmtState);
    Ret = EplApiReadLocalObject( 0x1F8C, 0, &MnNmtState, &MnNmtStateSize );

    if( kEplSuccessful == Ret )
    {
        printf( "MN Nmt-State (0x1F8C/0):\t%s\n", EplGetNmtStateStr(EPL_NMT_TYPE_MS | MnNmtState) );
    }
    else
    {
        printf( "Reading MN Nmt-State(0x1F8C/0) failed.\n" );
    }
}

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
void EplDebugSwitchSdoStack( tEplApiInitParam *pEplApiInitParam )
{
    tEplKernel  RetSwitchOff            = kEplSuccessful;
    tEplKernel  RetShutdown             = kEplSuccessful;
    tEplKernel  RetProcessImageFree     = kEplSuccessful;
    tEplKernel  RetEplApiInitialize     = kEplSuccessful;
    tEplKernel  RetObdLink              = kEplSuccessful;
    tEplKernel  RetProcessImageAlloc    = kEplSuccessful;
    tEplKernel  RetProcessImageSetup    = kEplSuccessful;
    tEplKernel  RetNmtCmdReset          = kEplSuccessful;

    tEplObdSize                 ObdSize;
    unsigned int                uiVarEntries;

    RetSwitchOff = EplApiExecNmtCommand(kEplNmtEventSwitchOff);
    if( kEplSuccessful != RetSwitchOff )
    {
        printf( "Exec NMT Switch Off failed\n" );
        goto Exit;
    }

    EplTgtMilliSleep( 1000 );

    RetProcessImageFree = EplApiProcessImageFree();
    if( kEplSuccessful != RetProcessImageFree )
    {
        printf( "EplApiProcessImageFree failed: %.4X\n", RetProcessImageFree );
        goto Exit;
    }

    RetShutdown = EplApiShutdown();
    if( kEplSuccessful != RetShutdown )
    {
        printf( "Exec Shutdown failed\n" );
        goto Exit;
    }

    EplTgtMilliSleep( 1000 );

    switch( pEplApiInitParam->m_SdoStackType )
    {
        case kEplApiTestSdoCmd:
            pEplApiInitParam->m_SdoStackType    = kEplApiStdSdoStack;
            break;

        case kEplApiTestSdoSequ:
            pEplApiInitParam->m_SdoStackType    = kEplApiTestSdoCmd;
            break;

        case kEplApiStdSdoStack:
            pEplApiInitParam->m_SdoStackType    = kEplApiTestSdoSequ;
            break;

        default:
            pEplApiInitParam->m_SdoStackType    = kEplApiStdSdoStack;
            break;
    }

#ifndef CONFIG_POWERLINK_USERSTACK
    // Wait for threads
    pthread_join( *DebugParam_g.m_pEventThreadId,   NULL );
//    pthread_join( *DebugParam_g.m_pSyncThreadId,    NULL );

    // Restart threads
    // create event thread
    if (pthread_create(DebugParam_g.m_pEventThreadId, NULL,
            DebugParam_g.m_pfnEventThread, NULL) != 0)
    {
        printf( "Starting Event thread failed\n" );
    }
/*
    // create sync thread
    if (pthread_create(*DebugParam_g.m_pSyncThreadId, NULL,
            DebugParam_g.m_pfnSyncThread, NULL) != 0)
    {
        printf( "Starting Sync thread failed\n" );
    }
*/
#endif

    RetEplApiInitialize = EplApiInitialize( pEplApiInitParam );
    if( kEplSuccessful != RetEplApiInitialize )
    {
        goto Exit;
    }

    // Object 1F9F, Sub-ID 4 is of type domain
    // Domain objects needs to be linked via EplApiLinkObject
/*    ObdSize         = sizeof(abCmdData);
    uiVarEntries    = 1;
    RetObdLink          = EplApiLinkObject(0x1F9F, abCmdData, &uiVarEntries, &ObdSize, 0x04);
    if (RetObdLink != kEplSuccessful)
    {
        goto Exit;
    }
*/
    RetProcessImageAlloc    = EplApiProcessImageAlloc(sizeof (*DebugParam_g.m_pAppPI_In), sizeof (*DebugParam_g.m_pAppPI_Out), 2, 2);
    if( kEplSuccessful != RetProcessImageAlloc )
    {
        printf( "Exec PI alloc failed\n" );
        goto Exit;
    }

    RetProcessImageSetup    = EplApiProcessImageSetup();
    if( kEplSuccessful != RetProcessImageSetup )
    {
        printf( "Exec PI setup failed\n" );
        goto Exit;
    }

    EplTgtMilliSleep( 1000 );

    RetNmtCmdReset          = EplApiExecNmtCommand(kEplNmtEventSwReset);
    if( kEplSuccessful != RetNmtCmdReset )
    {
        printf( "Exec NMT Reset failed\n" );
        goto Exit;
    }

Exit:
    printf( "======================================================================\n");

    switch( pEplApiInitParam->m_SdoStackType )
    {
        case kEplApiTestSdoCmd:
            printf( "New SDO stack is Testing stack for SDO Command layer.\n" );
            break;

        case kEplApiTestSdoSequ:
            printf( "New SDO stack is Testing stack for SDO Sequence layer.\n" );
            break;

        case kEplApiStdSdoStack:
            printf( "New SDO stack is Standard SDO stack.\n" );
            break;

        default:
            printf( "New SDO stack is unknown.\n" );
            break;
    }

    printf( "SwitchOff .......... %s\n",    RetSwitchOff == kEplSuccessful ? "ok" : "failed");
    printf( "Shutdown API ....... %s\n",    RetShutdown == kEplSuccessful ? "ok" : "failed");
    printf( "ProcessImageFree ... %s\n",    RetProcessImageFree == kEplSuccessful ? "ok" : "failed");
    printf( "Re-starting API .... %s\n",    RetEplApiInitialize == kEplSuccessful ? "ok" : "failed");
    printf( "ObjDic link ........ %s\n",    RetObdLink == kEplSuccessful ? "ok" : "failed");
    printf( "PI Alloc ........... %s\n",    RetProcessImageAlloc == kEplSuccessful ? "ok" : "failed");
    printf( "PI Setup ........... %s\n",    RetProcessImageSetup == kEplSuccessful ? "ok" : "failed");
    printf( "SwReset API ........ %s\n",    RetNmtCmdReset == kEplSuccessful ? "ok" : "failed");

    printf( "======================================================================\n");
}

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
tEplKernel  EplDebugTestSdoComu()
{
    tEplKernel          Ret;
    tEplAsySdoCom       *pSdoCom;
    unsigned    int     uiNodeId;
    tEplSdoType         SdoType;
    size_t              SdoSize;

    // Variables for frame generation
    BYTE                abBuffer[ 1500 ];
    BYTE                Payload[ 10 ]  = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9    };

    WORD                Index;
    BYTE                SubIndex;

    Ret = kEplSuccessful;

    // Set up SDO Cmd layer frame
    EPL_MEMSET( abBuffer, 0, sizeof(abBuffer) );

    pSdoCom      = (tEplAsySdoCom*)  abBuffer;

    Index       = 0x1006;
    SubIndex    = 0x00;

    pSdoCom->m_le_bTransactionId    = 0x55;
    pSdoCom->m_le_bFlags            = 0x00;
    pSdoCom->m_le_bCommandId        = 0x02;     // Read by Index
//    pSdoCom->m_le_bCommandId        = 0x00;     // NIL
    pSdoCom->m_le_wSegmentSize      = 0x00;
    SdoSize  = 8;
    EPL_MEMCPY(&pSdoCom->m_le_abCommandData[0], &Index,     sizeof(Index) );
    SdoSize  += sizeof(Index);
    EPL_MEMCPY(&pSdoCom->m_le_abCommandData[2], &SubIndex,  sizeof(SubIndex) );
    SdoSize  += sizeof(SubIndex);
    EPL_MEMCPY(&pSdoCom->m_le_abCommandData[3], Payload,    sizeof(Payload) );
    SdoSize  += sizeof(Payload);

    uiNodeId    = 32;
    SdoType     = kEplSdoTypeAsnd;
//    SdoType     = kEplSdoTypeUdp;

    if( kEplSuccessful == EplApiTestSdoComuSend( uiNodeId, SdoType, pSdoCom, SdoSize ) )
    {
        printf( "Triggering SDO test command was successful.\n" );
    }
    else
    {
        printf( "Triggering SDO test command rejected.\n" );
    }

    return  Ret;
}

tEplKernel  EplDebugTestSdoComuDelCon()
{
    tEplKernel  Ret;

    Ret = EplApiTestSdoComuDelCon();

    if( kEplSuccessful == Ret )
    {
        printf( "SDO Command layer connection successfully closed\n" );
    }
    else
    {
        printf( "Failed to close SDO Command layer connection\n" );
    }

    return Ret;
}

//---------------------------------------------------------------------------
//
// Function:            main
//
// Description:         main function of demo application
//
// Parameters:
//
// Returns:
//
//---------------------------------------------------------------------------
void    EplDebugTestSdoSequ()
{
    unsigned    int     uiNodeId;
    tEplSdoType         SdoType;
    tEplAsySdoSeq*      pSdoSeq;
    tEplAsySdoCom*      pSdoCom;
    size_t              SdoSize;
    BYTE                abBuffer[ 1500 ];
    tEplKernel          Ret;
    BYTE                PayLoad[]   = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0xC0, 0xFF, 0xEE };

    BYTE                SendCon     = 0x01;
    BYTE                RecvCon     = 0x00;
    BYTE                SendSequ    = 0;
    BYTE                RecvSequ    = 0;

    BYTE                SendSequCon = (SendSequ << 2) | SendCon;
    BYTE                RecvSequCon = (RecvSequ << 2) | RecvCon;;

    // Init transfer
    uiNodeId    = 1;
    SdoType     = kEplSdoTypeAsnd;

    // Set up frame
    EPL_MEMSET( abBuffer, 0, sizeof(abBuffer) );
    pSdoSeq = (tEplAsySdoSeq*)  abBuffer;
    pSdoCom = (tEplAsySdoCom*)  &pSdoSeq->m_le_abSdoSeqPayload;

    // SDO command part
    pSdoCom->m_le_bTransactionId    = 0xAA;
    pSdoCom->m_le_bFlags            = 0x00;
    pSdoCom->m_le_bCommandId        = 0x02;     // Read by index
    pSdoCom->m_le_wSegmentSize      = sizeof(PayLoad);

    EPL_MEMCPY( &pSdoCom->m_le_abCommandData, PayLoad, sizeof(PayLoad) );

    // SDO sequence part
    pSdoSeq->m_le_bRecSeqNumCon     = RecvSequCon;
    pSdoSeq->m_le_bSendSeqNumCon    = SendSequCon;

    SdoSize = 500;

    // Send frame
    Ret = EplApiTestSdoSequSend( uiNodeId, SdoType, pSdoSeq, SdoSize );
    if( kEplSuccessful != Ret )
    {
        printf( "Sending SDO Sequence frame failed\n" );
    }
    else
    {
        printf( "Sending SDO Sequence frame successful\n" );
    }

    EplTgtMilliSleep( 3000 );

    // Shutdown test connection
    Ret = EplApiTestSdoSequDelCon();
    if( kEplSuccessful != Ret )
    {
        printf( "Shutdown of SDO sequence testing connection failed\n" );
    }
    else
    {
        printf( "Shutdown of SDO sequence testing connection successful\n" );
    }
}

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
tEplKernel  EplDebugTriggerReadStoreCaps( void )
{
    tParamStoreCap      *pStoreCap;
    unsigned    int     uiNodeId;

    printf( "Triggering reading of parameter storage capabilities\n");

    // Get NodeID
    uiNodeId    = EplDebugReadUint( "NodeID", 1, 255, 10 );

    // Set up first transfer
    pStoreCap   = &ParamStoreCaps[DebugInst.StoreCapsID];

    DebugInst.StoreCapsID               = 0;
    DebugInst.ObtTransfer.SdoType       = kEplSdoTypeAsnd;
    DebugInst.ObtTransfer.uiNodeId      = uiNodeId;
    DebugInst.ObtTransfer.uiIndex       = pStoreCap->uiIndex;
    DebugInst.ObtTransfer.uiSubIndex    = pStoreCap->uiSubIndex;
    DebugInst.ObtTransfer.uiObdSize     = pStoreCap->uiSize;
    DebugInst.ObtTransfer.pvData        = &pStoreCap->ReadData;
    DebugInst.ObtTransfer.pName         = pStoreCap->Name;
    DebugInst.ObtTransfer.pUserArg      = pStoreCap;

    EplDebugReadObj( &DebugInst.ObtTransfer );

    return  kEplSuccessful;
}

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
void    EplDebugReadStoreCapsCb( tEplSdoComFinished Sdo_p )
{
    tParamStoreCap      *pStoreCap;

    pStoreCap   = (tParamStoreCap *) Sdo_p.m_pUserArg;

    printf( "Node: %d\tIndex: %.4X\tSubIndex: %d (%s):\t0x%.4X\n",
            Sdo_p.m_uiNodeId,
            pStoreCap->uiIndex,
            pStoreCap->uiSubIndex,
            pStoreCap->Name,
            pStoreCap->ReadData );

    DebugInst.StoreCapsID ++;

    // Set up next transfer
    if( tabentries(ParamStoreCaps) > DebugInst.StoreCapsID )
    {
        pStoreCap   = &ParamStoreCaps[DebugInst.StoreCapsID];

        DebugInst.ObtTransfer.uiIndex       = pStoreCap->uiIndex;
        DebugInst.ObtTransfer.uiSubIndex    = pStoreCap->uiSubIndex;
        DebugInst.ObtTransfer.uiObdSize     = pStoreCap->uiSize;
        DebugInst.ObtTransfer.pvData        = &pStoreCap->ReadData;
        DebugInst.ObtTransfer.pName         = pStoreCap->Name;
        DebugInst.ObtTransfer.pUserArg      = pStoreCap;

        EplDebugReadObj( &DebugInst.ObtTransfer );
    }
}

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
tEplKernel  EplDebugReadObj( tObdTransfer   *pObdTrans )
{
    tEplKernel  Ret = kEplSuccessful;

    // Read ObjDict entry
    Ret = EplApiReadObject( &pObdTrans->SdoComConHdl,
                            pObdTrans->uiNodeId,
                            pObdTrans->uiIndex,
                            pObdTrans->uiSubIndex,
                            pObdTrans->pvData,
                            &pObdTrans->uiObdSize,
                            pObdTrans->SdoType,
                            pObdTrans->pUserArg
                            );

    if( kEplSuccessful == Ret )
    {
        printf( "Node: %d\tIndex: %.4X\tSubIndex: %d (%s):\t0x%.4X\n",
                pObdTrans->uiNodeId,
                pObdTrans->uiIndex,
                pObdTrans->uiSubIndex,
                pObdTrans->pName,
                *((unsigned int*) pObdTrans->pvData) );
    }
    else if ( kEplApiTaskDeferred == Ret )
    {
        printf( "Node: %d\tIndex: %.4X\tSubIndex: %d (%s):\tdeferred\n",
                pObdTrans->uiNodeId,
                pObdTrans->uiIndex,
                pObdTrans->uiSubIndex,
                pObdTrans->pName );
    }
    else
    {
        printf( "Node: %d\tIndex: %.4X\tSubIndex: %d (%s):\tfailed with error %X\n",
                pObdTrans->uiNodeId,
                pObdTrans->uiIndex,
                pObdTrans->uiSubIndex,
                pObdTrans->pName,
                Ret );
    }

    return  Ret;
}

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
tEplKernel  EplDebugWriteObj( tObdTransfer   *pObdTrans )
{
    tEplKernel  Ret = kEplSuccessful;

    // Read ObjDict entry
    Ret = EplApiWriteObject( &pObdTrans->SdoComConHdl,
                            pObdTrans->uiNodeId,
                            pObdTrans->uiIndex,
                            pObdTrans->uiSubIndex,
                            pObdTrans->pvData,
                            pObdTrans->uiObdSize,
                            pObdTrans->SdoType,
                            pObdTrans->pUserArg
                            );

    if( kEplSuccessful == Ret )
    {
        printf( "Node: %d\tIndex: %.4X\tSubIndex: %d (%s):\t0x%.4X\n",
                pObdTrans->uiNodeId,
                pObdTrans->uiIndex,
                pObdTrans->uiSubIndex,
                pObdTrans->pName,
                *((unsigned int*) pObdTrans->pvData) );
    }
    else if ( kEplApiTaskDeferred == Ret )
    {
        printf( "Node: %d\tIndex: %.4X\tSubIndex: %d (%s):\tdeferred\n",
                pObdTrans->uiNodeId,
                pObdTrans->uiIndex,
                pObdTrans->uiSubIndex,
                pObdTrans->pName );
    }
    else
    {
        printf( "Node: %d\tIndex: %.4X\tSubIndex: %d (%s):\tfailed\n",
                pObdTrans->uiNodeId,
                pObdTrans->uiIndex,
                pObdTrans->uiSubIndex,
                pObdTrans->pName );
    }

    return  Ret;
}

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
unsigned int    EplDebugSelectMenu( tMenuEntry Menu[],
                                    unsigned int Cnt,
                                    char MenuName[] )
{
    unsigned    int     i;
    unsigned    int     InputValid  = 0;
    char                Select;

    printf( "Select %s:\n", MenuName );

    for( i = 0; i < Cnt; i ++ )
    {
        printf( "%c\t%s\n", Menu[i].ShortCut, Menu[i].Entry );
    }

    do
    {
        Select  = EplTgtGetch();

        for( i = 0; i < Cnt; i ++ )
        {
            if( Menu[i].ShortCut == Select )
            {
                InputValid  = 1;
                break;
            }
        }

        if( InputValid == 0 )
        {
            printf( "Invalid selection: retry\n" );
        }
    }
    while( InputValid == 0 );

    return  i;
}

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
void    EplDebugStoreParam()
{
    unsigned    int     uiNodeId        = 0;
    unsigned    int     uiParamSelect   = 0;
    unsigned    int     uiStoreSelect   = 0;

    unsigned    int     ParamSubId[]    = { 1, 2, 3, 4 };
    unsigned    int     StoreIndex[]      = { 0x1010, 0x1011  };

    tParamStoreCap      *pStoreCap;

    printf( "Store/Restore parameter\n" );

    uiNodeId    = EplDebugReadUint( "NodeID", 1, 255, 10 );

    uiStoreSelect   = EplDebugSelectMenu( ReStoreMenu,  tabentries( ReStoreMenu ), "Storetype" );
    uiParamSelect   = EplDebugSelectMenu( ParamMenu,    tabentries( ParamMenu ), "which parameters to (re)-store" );

    printf( "NodeId:\t%d\n",    uiNodeId        );
    printf( "Store:\t%d\n",     uiStoreSelect   );
    printf( "Param:\t%d\n",     uiParamSelect   );

    // Set up first transfer
    DebugInst.StoreCapsID  = 0;
    pStoreCap   = &ParamStoreCaps[DebugInst.StoreCapsID];

    DebugInst.StoreCapsID               = 0;
    DebugInst.ObtTransfer.SdoType       = kEplSdoTypeAsnd;
    DebugInst.ObtTransfer.uiNodeId      = uiNodeId;
    DebugInst.ObtTransfer.uiIndex       = pStoreCap->uiIndex;
    DebugInst.ObtTransfer.uiSubIndex    = pStoreCap->uiSubIndex;
    DebugInst.ObtTransfer.uiObdSize     = pStoreCap->uiSize;
    DebugInst.ObtTransfer.pvData        = &pStoreCap->WriteData;
    DebugInst.ObtTransfer.pName         = pStoreCap->Name;
    DebugInst.ObtTransfer.pUserArg      = pStoreCap;

    EplDebugWriteObj( &DebugInst.ObtTransfer );
}

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
void    EplDebugWriteStoreCapsCb( tEplSdoComFinished Sdo_p )
{
    tParamStoreCap      *pStoreCap;

    printf( "%s\n", __func__ );

/*
    pStoreCap   = (tParamStoreCap *) Sdo_p.m_pUserArg;

    printf( "Node: %d\tIndex: %.4X\tSubIndex: %d (%s):\t0x%.4X\n",
            Sdo_p.m_uiNodeId,
            pStoreCap->uiIndex,
            pStoreCap->uiSubIndex,
            pStoreCap->Name,
            pStoreCap->ReadData );

    DebugInst.StoreCapsID ++;

    // Set up next transfer
    if( tabentries(ParamStoreCaps) > DebugInst.StoreCapsID )
    {
        pStoreCap   = &ParamStoreCaps[DebugInst.StoreCapsID];

        DebugInst.ObtTransfer.uiIndex       = pStoreCap->uiIndex;
        DebugInst.ObtTransfer.uiSubIndex    = pStoreCap->uiSubIndex;
        DebugInst.ObtTransfer.uiObdSize     = pStoreCap->uiSize;
        DebugInst.ObtTransfer.pvData        = &pStoreCap->ReadData;
        DebugInst.ObtTransfer.pName         = pStoreCap->Name;
        DebugInst.ObtTransfer.pUserArg      = pStoreCap;

        EplDebugReadObj( &DebugInst.ObtTransfer );
    }
    */
}

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
DWORD   uiEplDebugEditObdData;
char    EplDebugEditObdName[]   = "EplDebugEditObdName";

void    EplDebugEditObd()
{
    unsigned    int     uiNodeId        = 0;
    unsigned    int     uiIndex         = 0;
    unsigned    int     uiSubIndex      = 0;
    unsigned    int     uiSize          = 0;
    unsigned    int     uiTransferType  = 0;
    unsigned    int     uiSdoType       = 0;
    tObdTransfer        *pObdTrans;

    tMenuEntry   EditObdMenu[]  =
    {
        {   'w',    "Write Object-Dict entry",    },
        {   'r',    "Read Object-Dict entry",     },
    };

    tMenuEntry   EditSdoTypeMenu[]  =
    {
        {   'a',    "ASnd",    },
        {   'u',    "UDP",     },
        {   'p',    "PDO",     },
    };

    printf( "============================================\n" );
    printf( "   Edit the Object-Dictionary entries\n" );
    printf( "============================================\n" );

    // Get user input
    uiNodeId        = EplDebugReadUint( "NodeId",   1,  255,    10  );
    uiIndex         = EplDebugReadUint( "Index",    1,  0x1FFF, 16  );
    uiSubIndex      = EplDebugReadUint( "SubIndex", 0,  255,    10  );
    uiSize          = EplDebugReadUint( "Size",     1,  4,      10  );
    uiSdoType       = EplDebugSelectMenu( EditSdoTypeMenu,  tabentries( EditSdoTypeMenu ), "Use SDO over ..." );
    uiTransferType  = EplDebugSelectMenu( EditObdMenu,  tabentries( EditObdMenu ), "R/W" );

    // Get transfer object
    pObdTrans   = (tObdTransfer *) EPL_MALLOC( sizeof( *pObdTrans ) );
    if( NULL == pObdTrans )
    {
        printf( "Malloc failed\n" );
        return;
    }

    // Set up SDO type
    switch( uiSdoType )
    {
        default:
        case 0:     pObdTrans->SdoType       = kEplSdoTypeAsnd;
                    break;

        case 1:     pObdTrans->SdoType       = kEplSdoTypeUdp;
                    break;

        case 2:     pObdTrans->SdoType       = kEplSdoTypePdo;
                    break;
    }

    pObdTrans->uiNodeId      = uiNodeId;
    pObdTrans->uiIndex       = uiIndex;
    pObdTrans->uiSubIndex    = uiSubIndex;
    pObdTrans->uiObdSize     = uiSize;
    pObdTrans->pName         = EplDebugEditObdName;
    pObdTrans->pUserArg      = pObdTrans;

    pObdTrans->pvData        = &uiEplDebugEditObdData;

    switch( uiTransferType )
    {
        case 0: // Write

            // Read data
            uiEplDebugEditObdData   = EplDebugReadUint( "Data", 0, (unsigned int) 0xFFFFFFFF, 16 );

            EplDebugWriteObj( pObdTrans );
            break;

        case 1: // Read

            EplDebugReadObj( pObdTrans );
            break;

        default:
            break;
    }
}

void    EplDebugCbEditObd( tObdTransfer *pObdTrans )
{
    BYTE    *pByte;
    WORD    *pWord;
    DWORD   *pDWord;
    DWORD   Data;

    switch( pObdTrans->uiObdSize )
    {
        case 1: pByte   = (BYTE *)  pObdTrans->pvData;
                Data    = *pByte;
                break;

        case 2: pWord   = (WORD *)  pObdTrans->pvData;
                Data    = *pWord;
                break;

        case 4: pDWord  = (DWORD *) pObdTrans->pvData;
                Data    = *pDWord;
                break;

        default:    printf( "Unsupported size.\n" );
                    return;
    }

    printf( "Data: %.8lX", Data );
}

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
tEplKernel  EplDebugTriggerPresForward(unsigned int uiNodeId_p)
{
    return  EplApiTriggerPresForward( uiNodeId_p );
}

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
tEplKernel  EplDebugGetIdentResponse(unsigned int uiNodeId_p)
{
    tEplIdentResponse*  pIdentResponse;

    if( kEplInvalidOperation == EplApiGetIdentResponse( uiNodeId_p, &pIdentResponse ) )
    {
        printf( "EplApiGetIdentResponse failed\n" );
    }
    else
    {
        printf( "EplApiGetIdentResponse successful\n" );

        printf( "Date: %ld\n", pIdentResponse->m_le_dwVerifyConfigurationDate );
        printf( "Time: %ld\n", pIdentResponse->m_le_dwApplicationSwTime );
    }

    return  kEplSuccessful;
}

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
tEplKernel  EplDebugSdoUdp( void )
{
/*    EplApiReadObject( )

        pSdoComConHdl_p         = INOUT: pointer to SDO connection handle (may be NULL in case of local OD access)
        //              uiNodeId_p              = IN: node ID (0 = itself)
        //              uiIndex_p               = IN: index of object in OD
        //              uiSubindex_p            = IN: sub-index of object in OD
        //              pDstData_le_p           = OUT: pointer to data in little endian
        //              puiSize_p               = INOUT: pointer to size of data
        //              SdoType_p               = IN: type of SDO transfer
        //              pUserArg_p

*/
    return  kEplSuccessful;
}

//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:
//---------------------------------------------------------------------------
void    EplDebugMenu( char cKey, tEplApiInitParam *pEplApiInitParam )
{
    tEplKernel  EplRet;

    switch (cKey)
    {
        case 'r':
        {
            EplRet = EplApiExecNmtCommand(kEplNmtEventSwReset);
            if (EplRet != kEplSuccessful)
            {
                goto ExitShutdown;
            }
            break;
        }

        case 'c':
        {
            EplRet = EplApiExecNmtCommand(kEplNmtEventNmtCycleError);
            if (EplRet != kEplSuccessful)
            {
                goto ExitShutdown;
            }
            break;
        }

        case 'p':
        {
            EplRet = EplDebugTriggerPresForward(1);
            if (EplRet != kEplSuccessful)
            {
                printf( "EplDebugTriggerPresForward failed.\n");
            }
            else
            {
                printf( "EplDebugTriggerPresForward worked.\n");
            }
            break;
        }

        case 'e':
        {
            EplRet = EplDebugSendNmtCmdEx();
            if (EplRet != kEplSuccessful)
            {
                goto ExitShutdown;
            }
            break;
        }

        case 's':
        {
            EplRet = EplDebugSendNmtCmd();
            if (EplRet != kEplSuccessful)
            {
                goto ExitShutdown;
            }
            break;
        }

        case 'm':
        {
            EplDebugPrintMnNmtState();
            break;
        }

        case 'd':
        {
            EplDebugTestSdoComu();
            break;
        }

        case 'a':
        {
            EplDebugTestSdoComuDelCon();
            break;
        }

        case 'f':
        {
            EplDebugTestSdoSequ();
            break;
        }

        case 'w':
        {
            EplDebugSwitchSdoStack( pEplApiInitParam );
            break;
        }

        case 't':
        {
            EplDebugTriggerReadStoreCaps( );
            break;
        }

        case 'g':
        {
            EplDebugStoreParam( );
            break;
        }

        case 'o':
        {
            EplDebugEditObd( );
            break;
        }

        case 'j':
        {
            EplDebugGetIdentResponse( 1 );
            break;
        }

        case 'h':
        {
            printf( "Supported commands are:\n\n"       );
            printf( "\tr ... Reset\n"                   );
            printf( "\tc ... Cycle error\n"             );
            printf( "\tp ... Pres Forward\n"            );
            printf( "\te ... Extended Nmt Command\n"    );
            printf( "\ts ... Simple Nmt Command\n"      );
            printf( "\tm ... Print MN state\n"          );
            printf( "\td ... SDO command test\n"        );
            printf( "\ta ... SDO command close\n"       );
            printf( "\tf ... SDO sequence test\n"       );
            printf( "\tw ... Switch SDO stack\n"        );
            printf( "\tt ... Read store Caps\n"         );
            printf( "\to ... Edit Object Dictionary\n"  );
            printf( "\tj ... Get Ident Response\n"      );
            printf( "\th ... Surprise :)\n"             );

            break;
        }

        default:
        {
            break;
        }
    }

ExitShutdown:
    {

    }
}

//=========================================================================//
//                                                                         //
//          P R I V A T E   F U N C T I O N S                              //
//                                                                         //
//=========================================================================//


//---------------------------------------------------------------------------
//
// Function:
//
// Description:
//
// Parameters:
//
// Returns:     tEplKernel      = error code,
//
// State:
//
//---------------------------------------------------------------------------


// EOF



