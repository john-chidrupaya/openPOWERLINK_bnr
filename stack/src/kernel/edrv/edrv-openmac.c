/**
********************************************************************************
\file   edrv-openmac.c

\brief  Implementation of openMAC Ethernet driver

This file contains the implementation of the openMAC Ethernet driver.

\ingroup module_edrv
*******************************************************************************/

/*------------------------------------------------------------------------------
Copyright (c) 2013, SYSTEC electronic GmbH
Copyright (c) 2013, Bernecker+Rainer Industrie-Elektronik Ges.m.b.H. (B&R)
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
#include <global.h>
#include <Epl.h>
#include <kernel/dllkfilter.h>

#include <common/edrv.h>
#include <target/openmac.h>
#include <omethlib.h>
#include <common/target.h>

#include <Benchmark.h>
#include <Debug.h>

//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

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

//comment the following lines to disable feature
//#define EDRV_2NDTXQUEUE    //use additional TX queue for MN

#if (EDRV_AUTO_RESPONSE == FALSE)
    #undef EDRV_MAX_AUTO_RESPONSES
    #define EDRV_MAX_AUTO_RESPONSES 0 //no auto-response used
#endif

#ifndef EDRV_TIME_TRIG_TX
#define EDRV_TIME_TRIG_TX FALSE
#endif

#if (EDRV_AUTO_RESPONSE == FALSE && EDRV_TIME_TRIG_TX == FALSE)
    #error "Please enable EDRV_AUTO_RESPONSE in EplCfg.h to use openMAC for CN!"
#endif
#if (EDRV_AUTO_RESPONSE != FALSE && EDRV_TIME_TRIG_TX != FALSE)
#error "Please disable EDRV_AUTO_RESPONSE in EplCfg.h to use openMAC for MN!"
#endif

#if (defined(EDRV_2NDTXQUEUE) && EDRV_TIME_TRIG_TX == FALSE)
    #undef EDRV_2NDTXQUEUE //2nd TX queue makes no sense here..
    #undef EDRV_MAX_TX_BUF2
#endif

//------------------------------------------------------------------------------
// local types
//------------------------------------------------------------------------------
#ifdef EDRV_2NDTXQUEUE
typedef struct
{
    tEdrvTxBuffer*  pBuffer;
    UINT32          timeOffsetAbs;
}tEdrv2ndTxQueue;
#endif

typedef struct
{
    tEdrvInitParam      initParam;
    ometh_config_typ    macConf;
    OMETH_H             pMacInst;
    OMETH_HOOK_H        pRxHookInst;
    OMETH_FILTER_H      apRxFilterInst[EDRV_MAX_FILTERS];
    phy_reg_typ*        apPhyInst[OPENMAC_PHYCNT];
    UINT8               phyInstCount;
    UINT32              txPacketFreed;
    UINT32              txPacketSent;
#if EDRV_MAX_AUTO_RESPONSES != 0
    // auto-response Tx buffers
    tEdrvTxBuffer*      apTxBuffer[EDRV_MAX_AUTO_RESPONSES];
#endif
#if OPENMAC_DMAOBSERV != 0
    BOOL                fDmaError;
#endif
#if OPENMAC_PKTLOCTX == OPENMAC_PKTBUF_LOCAL
    void*               pTxBufferBase;
    void*               pNextBufferBase;
    UINT8               txBufferCount;
    UINT                usedMemorySpace;
#endif
#if OPENMAC_PKTLOCRX == OPENMAC_PKTBUF_LOCAL
    void*               pRxBufferBase;
#endif
#ifdef EDRV_2NDTXQUEUE
    //additional tx queue
    tEdrv2ndTxQueue     txQueue[EDRV_MAX_TX_BUF2];
    INT                 txQueueWriteIndex;
    INT                 txQueueReadIndex;
#endif
#if DLL_DEFERRED_RXFRAME_RELEASE_ASYNCHRONOUS != FALSE
    OMETH_HOOK_H        pRxAsndHookInst;
#endif
} tEdrvInstance;

//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------
static tEdrvInstance edrvInstance_l;

//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------
static ometh_config_typ getMacConfig(UINT adapter_p);
static tEplKernel initRxFilters(void);
#if (OPENMAC_PKTLOCTX == OPENMAC_PKTBUF_LOCAL)
static ometh_packet_typ* allocTxMsgBufferIntern(tEdrvTxBuffer* pBuffer_p);
static void freeTxMsgBufferIntern(tEdrvTxBuffer* pBuffer_p);
#endif

// RX Hook function
static INT rxHook(void* pArg_p, ometh_packet_typ* pPacket_p, OMETH_BUF_FREE_FCT* pfnFree_p) SECTION_EDRVOPENMAC_RX_HOOK;
static void txAckCb(ometh_packet_typ* pPacket_p, void* pArg_p, ULONG time_p);
static void irqHandler(void* pArg_p) SECTION_EDRVOPENMAC_IRQ_HDL;

//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//

//------------------------------------------------------------------------------
/**
\brief  Ethernet driver initialization

This function initializes the Ethernet driver.

\param  pEdrvInitParam_p    Edrv initialization parameters

\return The function returns a tEplKernel error code.

\ingroup module_edrv
*/
//------------------------------------------------------------------------------
tEplKernel edrv_init(tEdrvInitParam* pEdrvInitParam_p)
{
    tEplKernel  ret = kEplSuccessful;
    INT         i;

    EPL_DBGLVL_EDRV_TRACE("*** %s ***\n", __func__);
    EPL_DBGLVL_EDRV_TRACE(" PHY_NUM = %d\n", OPENMAC_PHYCNT);
    EPL_DBGLVL_EDRV_TRACE(" MAX_RX_BUFFERS = %d\n", EDRV_MAX_RX_BUFFERS);
    EPL_DBGLVL_EDRV_TRACE(" PKTLOCTX = %d\n", OPENMAC_PKTLOCTX);
    EPL_DBGLVL_EDRV_TRACE(" PKTLOCRX = %d\n", OPENMAC_PKTLOCRX);
    EPL_DBGLVL_EDRV_TRACE(" PKTBUFSIZE = %d byte\n", OPENMAC_PKTBUFSIZE);

    EPL_MEMSET(&edrvInstance_l, 0, sizeof(edrvInstance_l));

    edrvInstance_l.initParam = *pEdrvInitParam_p;

    // The phys are reset by the initial hw state
    //  If sw comes here again, no phy reset is done!
    target_msleep(EDRV_PHY_RST_PULSE_MS);

    // activate phys and wait until ready
    omethMiiControl((void*)OPENMAC_PHY_BASE, MII_CTRL_ACTIVE);
    target_msleep(EDRV_PHY_RST_READY_MS);

    omethInit();

    edrvInstance_l.macConf = getMacConfig(0);

    edrvInstance_l.pMacInst = omethCreate(&edrvInstance_l.macConf);

    if (edrvInstance_l.pMacInst == NULL)
    {
        ret = kEplNoResource;
        DEBUG_LVL_ERROR_TRACE("%s() omethCreate failed\n", __func__);
        goto Exit;
    }

    //verify phy management
    for(i=0; i<OPENMAC_PHYCNT; i++)
    {
        edrvInstance_l.apPhyInst[i] = omethPhyInfo(edrvInstance_l.pMacInst, i);
        if(edrvInstance_l.apPhyInst[i] != 0)
        {
            edrvInstance_l.phyInstCount++;
        }
    }

    if(edrvInstance_l.phyInstCount != OPENMAC_PHYCNT)
    {
        DEBUG_LVL_ERROR_TRACE("%s() Not all phy are found as configured (%d)!\n", __func__, OPENMAC_PHYCNT);
        ret = kEplNoResource;
        goto Exit;
    }

    // initialize RX hook
    edrvInstance_l.pRxHookInst = omethHookCreate(edrvInstance_l.pMacInst, rxHook, 0); //last argument max. pending
    if (edrvInstance_l.pRxHookInst == NULL)
    {
        ret = kEplNoResource;
        goto Exit;
    }

#if DLL_DEFERRED_RXFRAME_RELEASE_ASYNCHRONOUS != FALSE
    // initialize Rx hook for Asnd frames with pending allowed
    edrvInstance_l.pRxAsndHookInst = omethHookCreate(edrvInstance_l.pMacInst, rxHook, EDRV_ASND_DEFFERRED_RX_BUFFERS);
    if(edrvInstance_l.pRxAsndHookInst == NULL)
    {
        DEBUG_LVL_ERROR_TRACE("%s() Rx hook creation for Asnd frames failed!\n", __func__);
        ret = kEplNoResource;
        goto Exit;
    }
#endif

    ret = initRxFilters();
    if(ret != kEplSuccessful)
        goto Exit;

    //moved following lines here, since omethHookCreate may change tx buffer base!
#if (OPENMAC_PKTLOCRX == OPENMAC_PKTBUF_LOCAL && OPENMAC_PKTLOCTX == OPENMAC_PKTBUF_LOCAL)
    //get rx/tx buffer base
    edrvInstance_l.pRxBufferBase = omethGetRxBufBase(edrvInstance_l.pMacInst);
    edrvInstance_l.pTxBufferBase = omethGetTxBufBase(edrvInstance_l.pMacInst);
#elif OPENMAC_PKTLOCTX == OPENMAC_PKTBUF_LOCAL
    //get tx buffer base
    edrvInstance_l.pTxBufferBase = openmac_memUncached((void*)OPENMAC_PKT_BASE, OPENMAC_PKT_SPAN);
#endif

    omethStart(edrvInstance_l.pMacInst, TRUE);
    EPL_DBGLVL_EDRV_TRACE(" OPENMAC started\n");

    ret = openmac_isrReg(kOpenmacIrqTxRx, irqHandler, (void*)edrvInstance_l.pMacInst);

    if(ret != kEplSuccessful)
        goto Exit;

    //wait some time (phy may not be ready...)
    target_msleep(1000);

Exit:
    return ret;
}

//------------------------------------------------------------------------------
/**
\brief  Ethernet driver shutdown

This function shuts down the Ethernet driver.

\return The function returns a tEplKernel error code.

\ingroup module_edrv
*/
//------------------------------------------------------------------------------
tEplKernel edrv_shutdown(void)
{
    omethStop(edrvInstance_l.pMacInst);

    openmac_isrReg(kOpenmacIrqTxRx, NULL, NULL);

    //okay, before we destroy openMAC, observe its statistics!
    DEBUG_LVL_ERROR_TRACE("%s() \n", __func__);
    {
        ometh_stat_typ *pMacStat = NULL;

        pMacStat = omethStatistics(edrvInstance_l.pMacInst);

        if( pMacStat == NULL )
        {
            DEBUG_LVL_ERROR_TRACE(" Serious error occurred!? Can't find the statistics!\n");
        }
        else
        {
            EPL_DBGLVL_EDRV_TRACE(" --- omethStatistics ---\n");
            EPL_DBGLVL_EDRV_TRACE(" ----  RX           ----\n");
            EPL_DBGLVL_EDRV_TRACE("  CRC ERROR = %i\n", (INT)pMacStat->rxCrcError);
            EPL_DBGLVL_EDRV_TRACE("  HOOK DISABLED = %i\n", (INT)pMacStat->rxHookDisabled);
            EPL_DBGLVL_EDRV_TRACE("  HOOK OVERFLOW = %i\n", (INT)pMacStat->rxHookOverflow);
            EPL_DBGLVL_EDRV_TRACE("  LOST = %i\n", (INT)pMacStat->rxLost);
            EPL_DBGLVL_EDRV_TRACE("  OK = %i\n", (INT)pMacStat->rxOk);
            EPL_DBGLVL_EDRV_TRACE("  OVERSIZE = %i\n", (INT)pMacStat->rxOversize);
            EPL_DBGLVL_EDRV_TRACE(" ----  TX           ----\n");
            EPL_DBGLVL_EDRV_TRACE("  COLLISION = %i\n", (INT)pMacStat->txCollision);
            EPL_DBGLVL_EDRV_TRACE("  DONE = %i\n", (INT)pMacStat->txDone[0]);
            EPL_DBGLVL_EDRV_TRACE("  SPURIOUS IRQ = %i\n", (INT)pMacStat->txSpuriousInt);
        }
        EPL_DBGLVL_EDRV_TRACE("\n");
    }

#if OPENMAC_DMAOBSERV != 0
    if( edrvInstance_l.fDmaError == TRUE )
    {
        //if you see this openMAC DMA is connected to slow memory!
        // -> use embedded memory or 10 nsec SRAM!!!
        DEBUG_LVL_ERROR_TRACE("%s() OPENMAC DMA TRANSFER ERROR\n", __func__);
    }
#endif

    if (omethDestroy(edrvInstance_l.pMacInst) != 0) {
        DEBUG_LVL_ERROR_TRACE("%s() Edrv Shutdown failed\n", __func__);
        return kEplNoResource;
    }
    EPL_DBGLVL_EDRV_TRACE("Edrv Shutdown done\n");

    return kEplSuccessful;
}

//------------------------------------------------------------------------------
/**
\brief  Allocate Tx buffer

This function allocates a Tx buffer.

\param  pBuffer_p           Tx buffer descriptor

\return The function returns a tEplKernel error code.

\ingroup module_edrv
*/
//------------------------------------------------------------------------------
tEplKernel edrv_allocTxBuffer(tEdrvTxBuffer* pBuffer_p)
{
    tEplKernel          ret = kEplSuccessful;
    ometh_packet_typ*   pPacket = NULL;

    if (pBuffer_p->maxBufferSize > EDRV_MAX_BUFFER_SIZE)
    {
        ret = kEplEdrvNoFreeBufEntry;
        goto Exit;
    }

    //openMAC does no padding, use memory for padding
    if( pBuffer_p->maxBufferSize < EDRV_MIN_ETH_SIZE)
    {
        pBuffer_p->maxBufferSize = EDRV_MIN_ETH_SIZE;
    }

#if (OPENMAC_PKTLOCTX == OPENMAC_PKTBUF_LOCAL)
    pPacket = allocTxMsgBufferIntern(pBuffer_p);
#else
    EPL_DBGLVL_EDRV_TRACE("%s() allocate %i bytes\n", __func__, (INT)(pBuffer_p->maxBufferSize + sizeof(pPacket->length)));
    pPacket = (ometh_packet_typ*)openmac_uncachedMalloc(pBuffer_p->maxBufferSize + sizeof(pPacket->length));
#endif

    if (pPacket == NULL)
    {
        DEBUG_LVL_ERROR_TRACE("%s() Memory allocation error\n", __func__);
        ret = kEplEdrvNoFreeBufEntry;
        goto Exit;
    }

    pPacket->length = pBuffer_p->maxBufferSize;

    pBuffer_p->txBufferNumber.value = EDRV_MAX_FILTERS;

    pBuffer_p->pBuffer = (UINT8*) &pPacket->data;

Exit:
    return ret;
}

//------------------------------------------------------------------------------
/**
\brief  Free Tx buffer

This function releases the Tx buffer.

\param  pBuffer_p           Tx buffer descriptor

\return The function returns a tEplKernel error code.

\ingroup module_edrv
*/
//------------------------------------------------------------------------------
tEplKernel edrv_freeTxBuffer(tEdrvTxBuffer* pBuffer_p)
{
    tEplKernel          ret = kEplSuccessful;
#if (OPENMAC_PKTLOCTX != OPENMAC_PKTBUF_LOCAL)
    ometh_packet_typ*   pPacket = NULL;
#endif

    if (pBuffer_p->txBufferNumber.value < EDRV_MAX_AUTO_RESPONSES)
    {
        // disable auto-response
        omethResponseDisable(edrvInstance_l.apRxFilterInst[pBuffer_p->txBufferNumber.value]);
    }

    if (pBuffer_p->pBuffer == NULL)
    {
        ret = kEplEdrvInvalidParam;
        goto Exit;
    }

#if (OPENMAC_PKTLOCTX == OPENMAC_PKTBUF_LOCAL)
    freeTxMsgBufferIntern(pBuffer_p);
#else
    pPacket = GET_TYPE_BASE(ometh_packet_typ, data, pBuffer_p->pBuffer);
    openmac_uncachedFree((UINT8*)pPacket);
#endif

    // mark buffer as free
    pBuffer_p->pBuffer = NULL;

Exit:
    return ret;
}

//------------------------------------------------------------------------------
/**
\brief  Update Tx buffer

This function updates the Tx buffer for use with auto-response filter.

\param  pBuffer_p           Tx buffer descriptor

\return The function returns a tEplKernel error code.

\ingroup module_edrv
*/
//------------------------------------------------------------------------------
tEplKernel edrv_updateTxBuffer(tEdrvTxBuffer* pBuffer_p)
{
    tEplKernel          ret = kEplSuccessful;
#if EDRV_MAX_AUTO_RESPONSES > 0
    ometh_packet_typ*   pPacket = NULL;

    if (pBuffer_p->txBufferNumber.value >= EDRV_MAX_AUTO_RESPONSES)
    {
        ret = kEplEdrvInvalidParam;
        goto Exit;
    }

    pPacket = GET_TYPE_BASE(ometh_packet_typ, data, pBuffer_p->pBuffer);

    pPacket->length = pBuffer_p->txFrameSize;

    // Flush data cache before handing over the packet buffer to openMAC.
    openmac_flushDataCache((UINT8*)pPacket, pPacket->length);

    // Update autoresponse buffer
    edrvInstance_l.apTxBuffer[pBuffer_p->txBufferNumber.value] = pBuffer_p;

    pPacket = omethResponseSet(edrvInstance_l.apRxFilterInst[pBuffer_p->txBufferNumber.value], pPacket);
    if (pPacket == OMETH_INVALID_PACKET)
    {
        ret = kEplNoResource;
        goto Exit;
    }

Exit:
#else
    //invalid call, since auto-resp is deactivated for MN support
    ret = kEplEdrvInvalidParam;
#endif
    return ret;
}

//------------------------------------------------------------------------------
/**
\brief  Send Tx buffer

This function sends the Tx buffer.

\param  pBuffer_p           Tx buffer descriptor

\return The function returns a tEplKernel error code.

\ingroup module_edrv
*/
//------------------------------------------------------------------------------
tEplKernel edrv_sendTxBuffer(tEdrvTxBuffer* pBuffer_p)
{
    tEplKernel          ret = kEplSuccessful;
    ometh_packet_typ*   pPacket = NULL;
    ULONG               txLength;

    pPacket = GET_TYPE_BASE(ometh_packet_typ, data, pBuffer_p->pBuffer);

    pPacket->length = pBuffer_p->txFrameSize;
#if EDRV_TIME_TRIG_TX != FALSE
    if( (pBuffer_p->timeOffsetAbs & 1) == 1)
    {
        //free tx descriptors available
        txLength = omethTransmitTime(edrvInstance_l.pMacInst, pPacket,
                        txAckCb, pBuffer_p, pBuffer_p->timeOffsetAbs);

        if( txLength == 0 )
        {
#ifdef EDRV_2NDTXQUEUE
            //time triggered sent failed => move to 2nd tx queue
            if( (edrvInstance_l.txQueueWriteIndex - edrvInstance_l.txQueueReadIndex) >= EDRV_MAX_TX_BUF2)
            {
                DEBUG_LVL_ERROR_TRACE("%s() Edrv 2nd TX queue is full\n", __func__);
                ret = kEplEdrvNoFreeBufEntry;
                goto Exit;
            }
            else
            {
                tEdrv2ndTxQueue *pTxqueue = &edrvInstance_l.txQueue[edrvInstance_l.txQueueWriteIndex & (EDRV_MAX_TX_BUF2-1)];
                pTxqueue->pBuffer = pBuffer_p;
                pTxqueue->timeOffsetAbs = pBuffer_p->timeOffsetAbs;

                edrvInstance_l.txQueueWriteIndex++;
                ret = kEplSuccessful;
                goto Exit; //packet will be sent!
            }
#else
            DEBUG_LVL_ERROR_TRACE("%s() No TX descriptor available\n", __func__);
            ret = kEplEdrvNoFreeBufEntry;
            goto Exit;
#endif
        }
    }
    else
    {
#endif
        txLength = omethTransmitArg(edrvInstance_l.pMacInst, pPacket,
                            txAckCb, pBuffer_p);
#if EDRV_TIME_TRIG_TX != FALSE
    }
#endif

    if (txLength > 0)
    {
        edrvInstance_l.txPacketSent++;
        ret = kEplSuccessful;
    }
    else
    {
        ret = kEplEdrvNoFreeBufEntry;
    }

#if EDRV_TIME_TRIG_TX != FALSE
Exit:
#endif
    if( ret != kEplSuccessful )
    {
        BENCHMARK_MOD_01_TOGGLE(7);
    }

    return ret;
}

//------------------------------------------------------------------------------
/**
\brief  Change Rx filter setup

This function changes the Rx filter setup. The parameter entryChanged_p
selects the Rx filter entry that shall be changed and \p changeFlags_p determines
the property.
If entryChanged_p is equal or larger \p count_p all Rx filters shall be changed.

\param  pFilter_p           Base pointer of Rx filter array
\param  count_p             Number of Rx filter array entries
\param  entryChanged_p      Index of Rx filter entry that shall be changed
\param  changeFlags_p       Bit mask that selects the changing Rx filter property

\return The function returns a tEplKernel error code.

\ingroup module_edrv
*/
//------------------------------------------------------------------------------
tEplKernel edrv_changeRxFilter(tEdrvFilter* pFilter_p, UINT count_p,
        UINT entryChanged_p, UINT changeFlags_p)
{
    tEplKernel  ret = kEplSuccessful;
    UINT        index;
    UINT        entry;

    if (((count_p != 0) && (pFilter_p == NULL)) || (count_p >= EDRV_MAX_FILTERS))
    {
        ret = kEplEdrvInvalidParam;
        goto Exit;
    }

    if (entryChanged_p >= count_p)
    {   // no specific entry changed
        // -> all entries changed

        // at first, disable all filters in openMAC
        for (entry = 0; entry < EDRV_MAX_FILTERS; entry++)
        {
            omethFilterDisable(edrvInstance_l.apRxFilterInst[entry]);
            omethResponseDisable(edrvInstance_l.apRxFilterInst[entry]);
        }

        for (entry = 0; entry < count_p; entry++)
        {
            // set filter value and mask
            for (index = 0; index < sizeof (pFilter_p->aFilterValue); index++)
            {
                omethFilterSetByteValue(edrvInstance_l.apRxFilterInst[entry],
                                        index,
                                        pFilter_p[entry].aFilterValue[index]);

                omethFilterSetByteMask(edrvInstance_l.apRxFilterInst[entry],
                                       index,
                                       pFilter_p[entry].aFilterMask[index]);
            }
#if EDRV_MAX_AUTO_RESPONSES > 0
            // set auto response
            if (pFilter_p[entry].pTxBuffer != NULL)
            {
                edrvInstance_l.apTxBuffer[entry] = pFilter_p[entry].pTxBuffer;

                // set buffer number of TxBuffer to filter entry
                pFilter_p[entry].pTxBuffer[0].txBufferNumber.value = entry;
                pFilter_p[entry].pTxBuffer[1].txBufferNumber.value = entry;
                edrv_updateTxBuffer(pFilter_p[entry].pTxBuffer);
                omethResponseEnable(edrvInstance_l.apRxFilterInst[entry]);

#if EDRV_AUTO_RESPONSE_DELAY != FALSE
                {
                    UINT32 delayNs;

                    // set auto-response delay
                    delayNs = pFilter_p[entry].pTxBuffer->timeOffsetNs;
                    if (delayNs == 0)
                    {   // no auto-response delay is set
                        // send frame immediately after IFG
                        omethResponseTime(edrvInstance_l.apRxFilterInst[entry], 0);
                    }
                    else
                    {   // auto-response delay is set
                        UINT32 delayAfterIfgNs;

                        if (delayNs < EPL_C_DLL_T_IFG)
                        {   // set delay to a minimum of IFG
                            delayNs = EPL_C_DLL_T_IFG;
                        }
                        delayAfterIfgNs = delayNs - EPL_C_DLL_T_IFG;
                        omethResponseTime(edrvInstance_l.apRxFilterInst[entry],
                                          OMETH_NS_2_TICKS(delayAfterIfgNs));
                    }
                }
#endif
            }
#endif

            if (pFilter_p[entry].fEnable != FALSE)
            {   // enable the filter
                omethFilterEnable(edrvInstance_l.apRxFilterInst[entry]);
            }
        }
    }
    else
    {   // specific entry should be changed

        if (((changeFlags_p & (EDRV_FILTER_CHANGE_VALUE
                                 | EDRV_FILTER_CHANGE_MASK
#if EDRV_AUTO_RESPONSE_DELAY != FALSE
                                 | EDRV_FILTER_CHANGE_AUTO_RESPONSE_DELAY
#endif
                                 | EDRV_FILTER_CHANGE_AUTO_RESPONSE)) != 0)
            || (pFilter_p[entryChanged_p].fEnable == FALSE))
        {
            // disable this filter entry
            omethFilterDisable(edrvInstance_l.apRxFilterInst[entryChanged_p]);

            if ((changeFlags_p & EDRV_FILTER_CHANGE_VALUE) != 0)
            {   // filter value has changed
                for (index = 0; index < sizeof (pFilter_p->aFilterValue); index++)
                {
                    omethFilterSetByteValue(edrvInstance_l.apRxFilterInst[entryChanged_p],
                                            index,
                                            pFilter_p[entryChanged_p].aFilterValue[index]);
                }
            }

            if ((changeFlags_p & EDRV_FILTER_CHANGE_MASK) != 0)
            {   // filter mask has changed
                for (index = 0; index < sizeof (pFilter_p->aFilterMask); index++)
                {
                    omethFilterSetByteMask(edrvInstance_l.apRxFilterInst[entryChanged_p],
                                           index,
                                           pFilter_p[entryChanged_p].aFilterMask[index]);
                }
            }

            if ((changeFlags_p & EDRV_FILTER_CHANGE_AUTO_RESPONSE) != 0)
            {   // filter auto-response state or frame has changed
                if (pFilter_p[entryChanged_p].pTxBuffer != NULL)
                {   // auto-response enable
                    // set buffer number of TxBuffer to filter entry
                    pFilter_p[entryChanged_p].pTxBuffer[0].txBufferNumber.value = entryChanged_p;
                    pFilter_p[entryChanged_p].pTxBuffer[1].txBufferNumber.value = entryChanged_p;
                    edrv_updateTxBuffer(pFilter_p[entryChanged_p].pTxBuffer);
                    omethResponseEnable(edrvInstance_l.apRxFilterInst[entryChanged_p]);
                }
                else
                {   // auto-response disable
                    omethResponseDisable(edrvInstance_l.apRxFilterInst[entryChanged_p]);
                }
            }

#if EDRV_AUTO_RESPONSE_DELAY != FALSE
            if ((changeFlags_p & EDRV_FILTER_CHANGE_AUTO_RESPONSE_DELAY) != 0)
            {   // filter auto-response delay has changed
                UINT32 delayNs;

                if (pFilter_p[entryChanged_p].pTxBuffer == NULL)
                {
                    ret = kEplEdrvInvalidParam;
                    goto Exit;
                }
                delayNs = pFilter_p[entryChanged_p].pTxBuffer->timeOffsetNs;

                if (delayNs == 0)
                {   // no auto-response delay is set
                    // send frame immediately after IFG
                    omethResponseTime(edrvInstance_l.apRxFilterInst[entryChanged_p], 0);
                }
                else
                {   // auto-response delay is set
                    UINT32 delayAfterIfgNs;

                    if (delayNs < EPL_C_DLL_T_IFG)
                    {   // set delay to a minimum of IFG
                        delayNs = EPL_C_DLL_T_IFG;
                    }
                    delayAfterIfgNs = delayNs - EPL_C_DLL_T_IFG;
                    omethResponseTime(edrvInstance_l.apRxFilterInst[entryChanged_p],
                                      OMETH_NS_2_TICKS(delayAfterIfgNs));
                }
            }
#endif
        }

        if (pFilter_p[entryChanged_p].fEnable != FALSE)
        {   // enable the filter
            omethFilterEnable(edrvInstance_l.apRxFilterInst[entryChanged_p]);
        }
    }

Exit:
    return ret;
}

//------------------------------------------------------------------------------
/**
\brief  Set multicast address entry

This function sets a multicast entry into the Ethernet controller.

\param  pMacAddr_p  Multicast address

\return The function returns a tEplKernel error code.

\ingroup module_edrv
*/
//------------------------------------------------------------------------------
tEplKernel edrv_setRxMulticastMacAddr(UINT8* pMacAddr_p)
{
    UNUSED_PARAMETER(pMacAddr_p);

    return kEplSuccessful;
}

//------------------------------------------------------------------------------
/**
\brief  Clear multicast address entry

This function removes the multicast entry from the Ethernet controller.

\param  pMacAddr_p  Multicast address

\return The function returns a tEplKernel error code.

\ingroup module_edrv
*/
//------------------------------------------------------------------------------
tEplKernel edrv_clearRxMulticastMacAddr(UINT8* pMacAddr_p)
{
    UNUSED_PARAMETER(pMacAddr_p);

    return kEplSuccessful;
}

//------------------------------------------------------------------------------
/**
\brief  Set Tx buffer ready

This function sets the Tx buffer buffer ready for transmission.

\param  pBuffer_p   Tx buffer buffer descriptor

\return The function returns a tEplKernel error code.

\ingroup module_edrv
*/
//------------------------------------------------------------------------------
tEplKernel edrv_setTxBufferReady(tEdrvTxBuffer* pBuffer_p)
{
    UNUSED_PARAMETER(pBuffer_p);

    return kEplSuccessful;
}

//------------------------------------------------------------------------------
/**
\brief  Start ready Tx buffer

This function sends the Tx buffer marked as ready.

\param  pBuffer_p   Tx buffer buffer descriptor

\return The function returns a tEplKernel error code.

\ingroup module_edrv
*/
//------------------------------------------------------------------------------
tEplKernel edrv_startTxBuffer(tEdrvTxBuffer* pBuffer_p)
{
    UNUSED_PARAMETER(pBuffer_p);

    return kEplSuccessful;
}

//------------------------------------------------------------------------------
/**
\brief  Release Rx buffer

This function releases a late release Rx buffer.

\param  pRxBuffer_p     Rx buffer to be released

\return The function returns a tEplKernel error code.

\ingroup module_edrv
*/
//------------------------------------------------------------------------------
tEplKernel  edrv_releaseRxBuffer (tEdrvRxBuffer* pRxBuffer_p)
{
    tEplKernel          ret = kEplSuccessful;
    ometh_packet_typ*   pPacket = NULL;

    pPacket = GET_TYPE_BASE(ometh_packet_typ, data, pRxBuffer_p->pBuffer);
    pPacket->length = pRxBuffer_p->rxFrameSize;

    if(pPacket->length != 0)
        omethPacketFree(pPacket);
    else
        ret = kEplEdrvInvalidRxBuf;

    return ret;
}

//============================================================================//
//            P R I V A T E   F U N C T I O N S                               //
//============================================================================//
/// \name Private Functions
/// \{

//------------------------------------------------------------------------------
/**
\brief  Get openMAC configuration

This function returns the openMAC configuration depending on the adapter.

\param  adapter_p   Adapter number

\return The function returns the adapters configuration.
*/
//------------------------------------------------------------------------------
static ometh_config_typ getMacConfig(UINT adapter_p)
{
    ometh_config_typ config;

    EPL_MEMSET(&config, 0, sizeof(config));

    config.adapter = adapter_p;
    config.macType = OMETH_MAC_TYPE_01;

    config.mode = 0
            | OMETH_MODE_HALFDUPLEX         // Half-duplex
            | OMETH_MODE_100MBIT            // 100 Mbps
            | OMETH_MODE_DIS_AUTO_NEG       // Disable Phy auto-negotiation
            ;

    config.rxBuffers = EDRV_MAX_RX_BUFFERS;
    config.rxMtu = EDRV_MAX_BUFFER_SIZE;

    config.pPhyBase = (void*)OPENMAC_PHY_BASE;
    config.pRamBase = (void*)OPENMAC_RAM_BASE;
    config.pRegBase = (void*)OPENMAC_REG_BASE;

#if OPENMAC_PKTLOCRX == OPENMAC_PKTBUF_LOCAL
    config.pBufBase = (void*)OPENMAC_PKT_BASE;
    config.pktLoc = OMETH_PKT_LOC_MACINT;
#else
    config.pBufBase = NULL;
    config.pktLoc = OMETH_PKT_LOC_HEAP;
#endif

    return config;
}

//------------------------------------------------------------------------------
/**
\brief  Initialize Rx filters

This function initializes all Rx filters and disables them.

\return The function returns a tEplKernel error code.
*/
//------------------------------------------------------------------------------
static tEplKernel initRxFilters(void)
{
    tEplKernel      ret = kEplSuccessful;
    INT             i;
    UINT8           aMask[31];
    UINT8           aValue[31];
    OMETH_HOOK_H    pHook;

    // initialize the filters, so that they won't match any normal Ethernet frame
    EPL_MEMSET(aMask, 0, sizeof(aMask));
    EPL_MEMSET(aMask, 0xFF, 6);
    EPL_MEMSET(aValue, 0, sizeof(aValue));

    for (i = 0; i < EDRV_MAX_FILTERS; i++)
    {
        // Assign filters to corresponding hooks
        switch(i)
        {
#if DLL_DEFERRED_RXFRAME_RELEASE_ASYNCHRONOUS != FALSE
            case DLLK_FILTER_ASND:
                pHook = edrvInstance_l.pRxAsndHookInst;
                break;
#endif
            default:
                pHook = edrvInstance_l.pRxHookInst;
                break;
        }

        edrvInstance_l.apRxFilterInst[i] = omethFilterCreate(pHook, (void*) i, aMask, aValue);
        if (edrvInstance_l.apRxFilterInst[i] == 0)
        {
            DEBUG_LVL_ERROR_TRACE("%s() Creating filter %d failed\n", __func__, i);
            ret = kEplNoResource;
            goto Exit;
        }

        omethFilterDisable(edrvInstance_l.apRxFilterInst[i]);

        if (i < EDRV_MAX_AUTO_RESPONSES)
        {
            // initialize the auto response for each filter ...
            if (omethResponseInit(edrvInstance_l.apRxFilterInst[i]) != 0)
            {
                ret = kEplNoResource;
                goto Exit;
            }

            // ... but disable it
            omethResponseDisable(edrvInstance_l.apRxFilterInst[i]);
        }
    }

Exit:
    return ret;
}

#if (OPENMAC_PKTLOCTX == OPENMAC_PKTBUF_LOCAL)
//------------------------------------------------------------------------------
/**
\brief  Allocate Tx buffer locally

This function allocates local memory for the Tx buffer descriptor pBuffer_p.

\param  pBuffer_p       Tx buffer descriptor

\return The function returns the allocated packet buffer's descriptor.
*/
//------------------------------------------------------------------------------
static ometh_packet_typ* allocTxMsgBufferIntern(tEdrvTxBuffer* pBuffer_p)
{
    ometh_packet_typ*   pPacket;
    UINT                bufferSize;
    void*               pBufferBase = openmac_memUncached((void*)OPENMAC_PKT_BASE, OPENMAC_PKT_SPAN);

    // Initialize if no buffer is allocated
    if(edrvInstance_l.txBufferCount == 0)
    {
        edrvInstance_l.pNextBufferBase = edrvInstance_l.pTxBufferBase;
        edrvInstance_l.usedMemorySpace = 0;
    }

    // Get buffer size from descriptor and add packet length
    bufferSize = pBuffer_p->maxBufferSize + sizeof(((ometh_packet_typ*)0)->length);

    // Align the buffer size to 4 byte alignment
    bufferSize += 0x3U;
    bufferSize &= 0xFFFFFFFCU;

    // Check for enough memory space
    if(bufferSize > OPENMAC_PKTBUFSIZE - edrvInstance_l.usedMemorySpace)
    {
        DEBUG_LVL_ERROR_TRACE("%s() Out of local memory\n", __func__);
        return NULL;
    }

    pPacket = (ometh_packet_typ*)edrvInstance_l.pNextBufferBase;

    // Return if the requested buffer is not within the memory range
    if(!(edrvInstance_l.pTxBufferBase <= (void*)pPacket && (void*)pPacket < (void*)((UINT32)pBufferBase + OPENMAC_PKTBUFSIZE)))
    {
        DEBUG_LVL_ERROR_TRACE("%s() Out of local memory\n", __func__);
        return NULL;
    }

    // Set allocated buffer to zeros
    EPL_MEMSET((void*)pPacket, 0, bufferSize);

    // Calculate next buffer address for next allocation
    edrvInstance_l.pNextBufferBase = (void*)((UINT32)pPacket + bufferSize);

    // New buffer added
    edrvInstance_l.txBufferCount++;
    edrvInstance_l.usedMemorySpace += bufferSize;

    EPL_DBGLVL_EDRV_TRACE("%s() Add buffer @ 0x%08X with size %4d byte ", __func__, pPacket, bufferSize);
    EPL_DBGLVL_EDRV_TRACE("(Used memory %4d byte %2d buffers)\n", edrvInstance_l.usedMemorySpace, edrvInstance_l.txBufferCount);

    return pPacket;
}

//------------------------------------------------------------------------------
/**
\brief  Free Tx buffer locally

This function frees local memory for the Tx buffer descriptor pBuffer_p.

\param  pBuffer_p   Packet buffer descriptor

\return The function returns the allocated packet buffer's descriptor.
*/
//------------------------------------------------------------------------------
static void freeTxMsgBufferIntern(tEdrvTxBuffer* pBuffer_p)
{
    INT bufferSize = pBuffer_p->maxBufferSize + sizeof(((ometh_packet_typ*)0)->length);

    // Align the buffer size to 4 byte alignment
    bufferSize += 0x3U;
    bufferSize &= 0xFFFFFFFCU;

    // Free the buffer from local memory
    edrvInstance_l.txBufferCount--;
    edrvInstance_l.usedMemorySpace -= bufferSize;

    EPL_DBGLVL_EDRV_TRACE("%s() Remove buffer with size %4d byte ", __func__, bufferSize);
    EPL_DBGLVL_EDRV_TRACE("(Used memory %4d byte %2d buffers)\n", edrvInstance_l.usedMemorySpace, edrvInstance_l.txBufferCount);
}
#endif /* OPENMAC_PKTLOCTX == OPENMAC_PKTBUF_LOCAL */

//------------------------------------------------------------------------------
/**
\brief  Ethernet driver interrupt handler

This function is invoked by the Ethernet controller interrupt.

\param  pArg_p  Interrupt service routine argument
*/
//------------------------------------------------------------------------------
static void irqHandler (void* pArg_p)
{
#if OPENMAC_DMAOBSERV != 0
    UINT16 dmaObservVal = openmac_getDmaObserver(0);
    //read DMA observer feature
    if( dmaObservVal != 0 )
    {
        edrvInstance_l.fDmaError = TRUE;
        BENCHMARK_MOD_01_TOGGLE(7);
        DEBUG_LVL_ERROR_TRACE("%s() DMA observer recognized overflow! (%X)\n", __func__, dmaObservVal);

        omethStop(pArg_p); //since openMAC was naughty, stop it!
    }
#endif

#if (defined(EDRV_2NDTXQUEUE) && (EDRV_TIME_TRIG_TX != FALSE))
    //observe additional TX queue and send packet if necessary
    while( (edrvInstance_l.txQueueWriteIndex - edrvInstance_l.txQueueReadIndex) &&
        (omethTransmitPending(edrvInstance_l.pMacInst) < 16U) )
    {
        tEdrvTxBuffer*      pBuffer_p;
        ometh_packet_typ*   pPacket;
        ULONG               txLength = 0U;
        tEdrv2ndTxQueue*    pTxqueue = &edrvInstance_l.txQueue[edrvInstance_l.txQueueReadIndex & (EDRV_MAX_TX_BUF2-1)];
        pBuffer_p = pTxqueue->pBuffer;

        pPacket = GET_TYPE_BASE(ometh_packet_typ, data, pBuffer_p->pBuffer);

        pPacket->length = pBuffer_p->txFrameSize;

        //offset is the openMAC time tick (no conversion needed)
        txLength = omethTransmitTime(edrvInstance_l.pMacInst, pPacket,
                        txAckCb, pBuffer_p, pTxqueue->timeOffsetAbs);

        if( txLength > 0 )
        {
            edrvInstance_l.txQueueReadIndex++;
            edrvInstance_l.txPacketSent++;
        }
        else
        {
            //no tx descriptor is free
        }
    }
#endif

    omethRxTxIrqHandlerMux();
}

//------------------------------------------------------------------------------
/**
\brief  Tx buffer sent call back

This function is called by omethlib in Tx interrupt context.

\param  pPacket_p   Sent packet
\param  pArg_p      User specific argument holding the Tx buffer descriptor
\param  time_p      Tx time stamp
*/
//------------------------------------------------------------------------------
static void txAckCb(ometh_packet_typ *pPacket_p, void *pArg_p, ULONG time_p)
{
    tEdrvTxBuffer* pTxBuffer = pArg_p;

    UNUSED_PARAMETER(pPacket_p);
    UNUSED_PARAMETER(time_p);

    edrvInstance_l.txPacketFreed++;

    if(pArg_p != NULL && pTxBuffer->pfnTxHandler != NULL)
        pTxBuffer->pfnTxHandler(pTxBuffer);
}

//------------------------------------------------------------------------------
/**
\brief  Rx buffer hook call back

This function is called by omethlib in Rx interrupt context.

\param  pArg_p      User specific argument holding the Tx response index
\param  pPacket_p   Received packet
\param  pfnFree_p   Function pointer to free function

\return The function returns an Rx buffer release command.
\retval 0           Packet buffer \p pPacket_p is deferred
\retval -1          Packet buffer \p pPacket_p can be freed immediately
*/
//------------------------------------------------------------------------------
static INT rxHook(void* pArg_p, ometh_packet_typ* pPacket_p, OMETH_BUF_FREE_FCT* pfnFree_p)
{
    INT                     ret;
    tEdrvRxBuffer           rxBuffer;
    tEdrvReleaseRxBuffer    releaseRxBuffer;
    tEplTgtTimeStamp        timeStamp;
#if EDRV_MAX_AUTO_RESPONSES > 0
    UINT                    txRespIndex = (UINT)pArg_p;
#endif
    UNUSED_PARAMETER(pfnFree_p);

    rxBuffer.bufferInFrame = kEdrvBufferLastInFrame;
    rxBuffer.pBuffer = (UINT8*) &pPacket_p->data;
    rxBuffer.rxFrameSize = pPacket_p->length;
    timeStamp.timeStamp = omethGetTimestamp(pPacket_p);
    rxBuffer.rxTimeStamp = &timeStamp;

    // Before handing over the Rx packet to the stack invalidate the packet's
    // memory range.
    openmac_invalidateDataCache((UINT8*)pPacket_p, pPacket_p->length);

    releaseRxBuffer = edrvInstance_l.initParam.pfnRxHandler(&rxBuffer); //pass frame to Powerlink Stack

    if(releaseRxBuffer == kEdrvReleaseRxBufferLater)
        ret = 0; // Packet is deferred, openMAC may not use this buffer!
    else
        ret = -1; // Packet processing is done, returns to openMAC again

#if EDRV_MAX_AUTO_RESPONSES > 0
    if (edrvInstance_l.apTxBuffer[txRespIndex] != NULL)
    {   // filter with auto-response frame triggered
        BENCHMARK_MOD_01_SET(5);
        // call Tx handler function from DLL
        if (edrvInstance_l.apTxBuffer[txRespIndex]->pfnTxHandler != NULL)
        {
            edrvInstance_l.apTxBuffer[txRespIndex]->pfnTxHandler(edrvInstance_l.apTxBuffer[txRespIndex]);
        }
        BENCHMARK_MOD_01_RESET(5);
    }
#endif

    return ret;
}

///\}
