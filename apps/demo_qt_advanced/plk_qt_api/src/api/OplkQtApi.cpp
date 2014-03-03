/**
********************************************************************************
\file   OplkQtApi.cpp

\brief  Contains the implementions to wrap the openPOWERLINK APIs.

/*------------------------------------------------------------------------------
Copyright (c) 2014, Kalycito Infotech Private Limited
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

/*******************************************************************************
* INCLUDES
*******************************************************************************/
#include <QMetaType>
#include <QtDebug>
#include "api/OplkQtApi.h"
#include "api/OplkEventHandler.h"

/*******************************************************************************
* Module global variables
*******************************************************************************/
static const ULONG kIpAddress = 0xc0a864F0;   ///< MN by default (192.168.100.240)
static const ULONG kSubnetMask = 0xFFFFFF00;  ///< 255.255.255.0
static const std::string kHostName = "openPOWERLINK Stack"; ///< max 32 chars
static const ULONG kCycleLen = 5000;  ///< Cycle Length (0x1006: NMT_CycleLen_U32) in [us]
static const BYTE abMacAddr[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  ///<Default MAC Address
static const std::string defaultCDCFilename = CONFIG_OBD_DEF_CONCISEDCF_FILENAME;  ///< Default CDC file name


/*******************************************************************************
* Static member variables
*******************************************************************************/
tEplApiInitParam OplkQtApi::initParam; ///< initparam.
bool OplkQtApi::cdcSet = false;  ///< Flag to detect CDC has been set or not.

/*******************************************************************************
* Private functions
*******************************************************************************/

void OplkQtApi::SetInitParam()
{
	EPL_MEMSET(&initParam, 0, sizeof (initParam));
	initParam.m_uiSizeOfStruct = sizeof (initParam);

	initParam.m_fAsyncOnly = FALSE;

	initParam.m_dwFeatureFlags = UINT_MAX;      // 0x1F82: NMT_FeatureFlags_U32
	initParam.m_dwCycleLen = kCycleLen;         // required for error detection
	initParam.m_uiIsochrTxMaxPayload = 256;     // const
	initParam.m_uiIsochrRxMaxPayload = 256;     // const
	initParam.m_dwPresMaxLatency = 50000;       // const; only required for IdentRes
	initParam.m_uiPreqActPayloadLimit = 36;     // required for initialisation (+28 bytes)
	initParam.m_uiPresActPayloadLimit = 36;     // required for initialisation of Pres frame (+28 bytes)
	initParam.m_dwAsndMaxLatency = 150000;      // const; only required for IdentRes
	initParam.m_uiMultiplCycleCnt = 0;          // required for error detection
	initParam.m_uiAsyncMtu = 1500;              // required to set up max frame size
	initParam.m_uiPrescaler = 2;                // required for sync
	initParam.m_dwLossOfFrameTolerance = 500000;
	initParam.m_dwAsyncSlotTimeout = 3000000;
	initParam.m_dwWaitSocPreq = 150000;
	initParam.m_dwDeviceType = UINT_MAX;        // NMT_DeviceType_U32
	initParam.m_dwVendorId = UINT_MAX;          // NMT_IdentityObject_REC.VendorId_U32
	initParam.m_dwProductCode = UINT_MAX;       // NMT_IdentityObject_REC.ProductCode_U32
	initParam.m_dwRevisionNumber = UINT_MAX;    // NMT_IdentityObject_REC.RevisionNo_U32
	initParam.m_dwSerialNumber = UINT_MAX;      // NMT_IdentityObject_REC.SerialNo_U32

	initParam.m_dwSubnetMask = kSubnetMask;
	initParam.m_dwDefaultGateway = 0;
	EPL_MEMCPY(initParam.m_sHostname, kHostName.c_str(), sizeof(initParam.m_sHostname));
	initParam.m_uiSyncNodeId = EPL_C_ADR_SYNC_ON_SOA;
	initParam.m_fSyncOnPrcNode = FALSE;

	// write 00:00:00:00:00:00 to MAC address, so that the driver uses the real hardware address
	EPL_MEMCPY(initParam.m_abMacAddress, abMacAddr, sizeof(initParam.m_abMacAddress));

	// set callback functions
	initParam.m_pfnCbEvent = OplkEventHandler::GetInstance().GetEventCbFunc();
	if (initParam.m_pfnCbEvent == NULL)
	{
		qDebug("Null Call back");
		// Never expecting to happen
		// If needed Throw std err or return kErrorInvalidInstanceParam
	}
}

/*******************************************************************************
* Public functions
*******************************************************************************/
tOplkError OplkQtApi::InitStack(const UINT nodeId,
						const std::string& networkInterface)
{
	OplkQtApi::SetInitParam();

	initParam.m_uiNodeId = nodeId;
	initParam.m_dwIpAddress = (kIpAddress & kSubnetMask) | initParam.m_uiNodeId;

	initParam.m_HwParam.m_pszDevName = networkInterface.c_str();

	return (oplk_init(&initParam));
}

tOplkError OplkQtApi::StartStack()
{
	tOplkError oplkRet = kErrorGeneralError;

	if (!OplkQtApi::cdcSet)
	{
		qDebug("No custom CDC set, setting default CDC path");
		oplkRet = oplk_setCdcFilename((char*) defaultCDCFilename.c_str());
		if (oplkRet != kErrorOk)
		{
			qDebug("Set default CDC File. Ret: %d",oplkRet);
			return oplkRet;
		}
	}

	// Setup processImage according to CiA302_4 profile.
	// TODO: If this is a CN setupPI is not needed.
	oplkRet = oplk_setupProcessImage();
	if (oplkRet != kErrorOk)
	{
		qDebug("SetupProcessImage retCode %x", oplkRet);
		return oplkRet;
	}

	// Starting process thread
	OplkEventHandler::GetInstance().start();

	// Start the OPLK stack by sending NMT s/w reset
	oplkRet = oplk_execNmtCommand(kNmtEventSwReset);
	if (oplkRet != kErrorOk)
		qDebug("kNmtEventSwReset Ret: %d", oplkRet);

	return oplkRet;
}

tOplkError OplkQtApi::StopStack()
{
	tOplkError oplkRet = kErrorGeneralError;

	// cdcSet should be set to false.
	OplkQtApi::cdcSet = false;

	oplkRet = oplk_execNmtCommand(kNmtEventSwitchOff);
	if (oplkRet != kErrorOk)
	{
		qDebug("kNmtEventSwitchOff Ret: %d", oplkRet);
		return oplkRet;
	}

	OplkEventHandler::GetInstance().AwaitNmtGsOff();

	// TODO Set ProcessImage::data to NULL;
	oplkRet = oplk_freeProcessImage();
	if (oplkRet != kErrorOk)
	{
		qDebug("freeProcessImage Ret: %d", oplkRet);
		return oplkRet;
	}

	oplkRet = oplk_shutdown();
	if (oplkRet != kErrorOk)
	{
		qDebug("shutdown Ret: %d", oplkRet);
	}

	return oplkRet;
}

bool OplkQtApi::RegisterNodeFoundEventHandler(const QObject& receiver,
					const QMetaMethod& receiverFunction)
{
	return QObject::connect(&OplkEventHandler::GetInstance(),
		QMetaMethod::fromSignal(&OplkEventHandler::SignalNodeFound),
		&receiver,
		receiverFunction,
		(Qt::ConnectionType) (Qt::QueuedConnection | Qt::UniqueConnection));
}

bool OplkQtApi::UnregisterNodeFoundEventHandler(const QObject& receiver,
					const QMetaMethod& receiverFunction)
{
	return QObject::disconnect(&OplkEventHandler::GetInstance(),
		QMetaMethod::fromSignal(&OplkEventHandler::SignalNodeFound),
		&receiver,
		receiverFunction);
}

bool OplkQtApi::RegisterNodeStateChangedEventHandler(const QObject& receiver,
					const QMetaMethod& receiverFunction)
{
	/* qRegisterMetaType<T>() is only required for sending the object
	 * through queued signal/slot connections.
	 */
	qRegisterMetaType<tNmtState>("tNmtState");
	return QObject::connect(&OplkEventHandler::GetInstance(),
		QMetaMethod::fromSignal(&OplkEventHandler::SignalNodeStateChanged),
		&receiver,
		receiverFunction,
		(Qt::ConnectionType) (Qt::QueuedConnection | Qt::UniqueConnection));
}

bool OplkQtApi::UnregisterNodeStateChangedEventHandler(const QObject& receiver,
					const QMetaMethod& receiverFunction)
{
	return QObject::disconnect(&OplkEventHandler::GetInstance(),
		QMetaMethod::fromSignal(&OplkEventHandler::SignalNodeStateChanged),
		&receiver,
		receiverFunction);
}

bool OplkQtApi::RegisterLocalNodeStateChangedEventHandler(
					const QObject& receiver,
					const QMetaMethod& receiverFunction)
{
	qRegisterMetaType<tNmtState>("tNmtState");
	return QObject::connect(&OplkEventHandler::GetInstance(),
		QMetaMethod::fromSignal(&OplkEventHandler::SignalLocalNodeStateChanged),
		&receiver,
		receiverFunction,
		(Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
}

bool OplkQtApi::UnregisterLocalNodeStateChangedEventHandler(
					const QObject& receiver,
					const QMetaMethod& receiverFunction)
{
	return QObject::disconnect(&OplkEventHandler::GetInstance(),
		QMetaMethod::fromSignal(&OplkEventHandler::SignalLocalNodeStateChanged),
		&receiver,
		receiverFunction);
}

bool OplkQtApi::RegisterEventLogger(const QObject& receiver,
					const QMetaMethod& receiverFunction)
{
	return QObject::connect(&OplkEventHandler::GetInstance(),
		QMetaMethod::fromSignal(&OplkEventHandler::SignalPrintLog),
		&receiver,
		receiverFunction,
		(Qt::ConnectionType) (Qt::QueuedConnection | Qt::UniqueConnection));
}

bool OplkQtApi::UnregisterEventLogger(const QObject& receiver,
					const QMetaMethod& receiverFunction)
{
	return QObject::disconnect(&OplkEventHandler::GetInstance(),
		QMetaMethod::fromSignal(&OplkEventHandler::SignalPrintLog),
		&receiver,
		receiverFunction);
}

tOplkError OplkQtApi::ExecuteNmtCommand(const UINT nodeId,
						tNmtCommand nmtCommand)
{
	return oplk_execRemoteNmtCommand(nodeId, nmtCommand);
}

tOplkError OplkQtApi::TransferObject(const SdoTransferJob& sdoTransferJob,
						const QObject& receiver,
						const QMetaMethod& receiverFunction)
{
	tSdoComConHdl *sdoComConHdl = new tSdoComConHdl;
	ReceiverContext *receiverContext = NULL;

	/* qRegisterMetaType<T>() is only required for sending the object
	 * through queued signal/slot connections.
	 */
	qRegisterMetaType<SdoTransferResult>("SdoTransferResult");

	if ( ((sdoTransferJob.GetNodeId() != initParam.m_uiNodeId)
		 && (sdoTransferJob.GetNodeId() != 0)) )
	{
		receiverContext = new ReceiverContext(&receiver, &receiverFunction);
		bool conSuccess = false;
		// Non-Local OD access
		qDebug("Remote OD access, connecting signal.");
		conSuccess = QObject::connect(&OplkEventHandler::GetInstance(),
					QMetaMethod::fromSignal(&OplkEventHandler::SignalSdoTransferFinished),
					&receiver,
					receiverFunction,
					(Qt::ConnectionType) (Qt::QueuedConnection | Qt::UniqueConnection));
		if (!conSuccess)
		{
			qDebug("Connection failed !");
			return kErrorApiInvalidParam;
		}
		qDebug("Connection success");
	}
	else
	{
		//Local OD access
		qDebug("Local OD access, not connecting signal.");
	}

	tOplkError oplkRet = kErrorGeneralError;
	UINT dataSize =  sdoTransferJob.GetDataSize();
	switch (sdoTransferJob.GetSdoAccessType())
	{
		case kSdoAccessTypeRead:
		{
			oplkRet = oplk_readObject(sdoComConHdl,
						sdoTransferJob.GetNodeId(),
						sdoTransferJob.GetIndex(),
						sdoTransferJob.GetSubIndex(),
						sdoTransferJob.GetData(),
						&dataSize,
						sdoTransferJob.GetSdoType(),
						(void*) receiverContext);
			break;
		}
		case kSdoAccessTypeWrite:
		{
			qDebug("Write Val %lu", ((ULONG)sdoTransferJob.GetData()));
			oplkRet = oplk_writeObject(sdoComConHdl,
						sdoTransferJob.GetNodeId(),
						sdoTransferJob.GetIndex(),
						sdoTransferJob.GetSubIndex(),
						sdoTransferJob.GetData(),
						dataSize,
						sdoTransferJob.GetSdoType(),
						(void*) receiverContext);
			break;
		}
		default:
			qDebug("Error Case: API: TransferObject ");
			break;
	}

	if ((oplkRet != kErrorApiTaskDeferred)
		&& ((sdoTransferJob.GetNodeId() != initParam.m_uiNodeId)
		&& (sdoTransferJob.GetNodeId() != 0)))
	{
		// Non-Local OD access: error case

		// Delete the receiver context.
		if (!receiverContext)
			delete[] receiverContext;

		bool disconnected = false;
		qDebug("Remote OD access, disconnecting signal.");
		disconnected = QObject::disconnect(&OplkEventHandler::GetInstance(),
					QMetaMethod::fromSignal(&OplkEventHandler::SignalSdoTransferFinished),
					&receiver,
					receiverFunction
		);

		if (!disconnected)
		{
			// This is not supposed to happen.
			qDebug("Disconnect failed !");
			return kErrorApiInvalidParam;
		}
		qDebug("Disconnected success");
	}

	return oplkRet;
}

tOplkError OplkQtApi::AllocateProcessImage(ProcessImageIn& in,
						ProcessImageOut& out)
{
	tOplkError oplkRet = kErrorGeneralError;

	/* Allocates the memory for the ProcessImage inside the stack */
	oplkRet = oplk_allocProcessImage(in.GetSize(), out.GetSize());
	if (oplkRet != kErrorOk)
	{
		qDebug("allocProcessImage retCode %x", oplkRet);
		return oplkRet;
	}

	in.SetProcessImageDataPtr((BYTE*)oplk_getProcessImageIn());
	out.SetProcessImageDataPtr((BYTE*)oplk_getProcessImageOut());

	return oplkRet;
}

void OplkQtApi::SetCdc(const BYTE* cdcBuffer, const UINT size)
{
	tOplkError oplkRet = oplk_setCdcBuffer((BYTE*) cdcBuffer, size);
	OplkQtApi::cdcSet = (oplkRet == kErrorOk);
}

void OplkQtApi::SetCdc(const char* cdcFileName)
{
	tOplkError oplkRet = oplk_setCdcFilename((char*) cdcFileName);
	OplkQtApi::cdcSet = (oplkRet == kErrorOk);
}

void OplkQtApi::SetCycleTime(const ULONG cycleTime)
{
	oplk_writeLocalObject(0x1006, 0x00, (void*)&cycleTime, 4);
	// If this is a CN. It has to do remote SDO write?.
}
