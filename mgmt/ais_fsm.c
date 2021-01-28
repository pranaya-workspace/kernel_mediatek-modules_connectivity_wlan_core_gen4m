/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/mgmt/ais_fsm.c#4
*/

/*! \file   "aa_fsm.c"
*    \brief  This file defines the FSM for SAA and AAA MODULE.
*
*    This file defines the FSM for SAA and AAA MODULE.
*/


/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "precomp.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#define AIS_ROAMING_CONNECTION_TRIAL_LIMIT  2
#define AIS_JOIN_TIMEOUT                    7

#define AIS_FSM_STATE_SEARCH_ACTION_PHASE_0	0
#define AIS_FSM_STATE_SEARCH_ACTION_PHASE_1	1
#define AIS_FSM_STATE_SEARCH_ACTION_PHASE_2	2

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
static uint8_t *apucDebugAisState[AIS_STATE_NUM] = {
	(uint8_t *) DISP_STRING("IDLE"),
	(uint8_t *) DISP_STRING("SEARCH"),
	(uint8_t *) DISP_STRING("SCAN"),
	(uint8_t *) DISP_STRING("ONLINE_SCAN"),
	(uint8_t *) DISP_STRING("LOOKING_FOR"),
	(uint8_t *) DISP_STRING("WAIT_FOR_NEXT_SCAN"),
	(uint8_t *) DISP_STRING("REQ_CHANNEL_JOIN"),
	(uint8_t *) DISP_STRING("JOIN"),
	(uint8_t *) DISP_STRING("JOIN_FAILURE"),
	(uint8_t *) DISP_STRING("IBSS_ALONE"),
	(uint8_t *) DISP_STRING("IBSS_MERGE"),
	(uint8_t *) DISP_STRING("NORMAL_TR"),
	(uint8_t *) DISP_STRING("DISCONNECTING"),
	(uint8_t *) DISP_STRING("REQ_REMAIN_ON_CHANNEL"),
	(uint8_t *) DISP_STRING("REMAIN_ON_CHANNEL")
};

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static void aisFsmRunEventScanDoneTimeOut(IN struct ADAPTER *prAdapter, unsigned long ulParam);
static void aisFsmSetOkcTimeout(IN struct ADAPTER *prAdapter, unsigned long ulParam);
/* Support AP Selection*/
static void aisRemoveDisappearedBlacklist(struct ADAPTER *prAdapter);

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
/*----------------------------------------------------------------------------*/
/*!
* @brief the function is used to initialize the value of the connection settings for
*        AIS network
*
* @param (none)
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisInitializeConnectionSettings(IN struct ADAPTER *prAdapter, IN struct REG_INFO *prRegInfo)
{
	struct CONNECTION_SETTINGS *prConnSettings;
	uint8_t aucAnyBSSID[] = BC_BSSID;
	uint8_t aucZeroMacAddr[] = NULL_MAC_ADDR;
	int i = 0;

	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	/* Setup default values for operation */
	COPY_MAC_ADDR(prConnSettings->aucMacAddress, aucZeroMacAddr);

	prConnSettings->ucDelayTimeOfDisconnectEvent = AIS_DELAY_TIME_OF_DISCONNECT_SEC;

	COPY_MAC_ADDR(prConnSettings->aucBSSID, aucAnyBSSID);
	prConnSettings->fgIsConnByBssidIssued = FALSE;

	prConnSettings->eReConnectLevel = RECONNECT_LEVEL_MIN;
	prConnSettings->fgIsConnReqIssued = FALSE;
	prConnSettings->fgIsDisconnectedByNonRequest = FALSE;

	prConnSettings->ucSSIDLen = 0;

	prConnSettings->eOPMode = NET_TYPE_INFRA;

	prConnSettings->eConnectionPolicy = CONNECT_BY_SSID_BEST_RSSI;

	if (prRegInfo) {
		prConnSettings->ucAdHocChannelNum = (uint8_t) nicFreq2ChannelNum(prRegInfo->u4StartFreq);
		prConnSettings->eAdHocBand = prRegInfo->u4StartFreq < 5000000 ? BAND_2G4 : BAND_5G;
		prConnSettings->eAdHocMode = (enum ENUM_PARAM_AD_HOC_MODE) (prRegInfo->u4AdhocMode);
	}

	prConnSettings->eAuthMode = AUTH_MODE_OPEN;

	prConnSettings->eEncStatus = ENUM_ENCRYPTION_DISABLED;

	prConnSettings->fgIsScanReqIssued = FALSE;

	/* MIB attributes */
	prConnSettings->u2BeaconPeriod = DOT11_BEACON_PERIOD_DEFAULT;

	prConnSettings->u2RTSThreshold = DOT11_RTS_THRESHOLD_DEFAULT;

	prConnSettings->u2DesiredNonHTRateSet = RATE_SET_ALL_ABG;

	/* prConnSettings->u4FreqInKHz; *//* Center frequency */

	/* Set U-APSD AC */
	prConnSettings->bmfgApsdEnAc = PM_UAPSD_NONE;

	secInit(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/* Features */
	prConnSettings->fgIsEnableRoaming = FALSE;

	prConnSettings->fgSecModeChangeStartTimer = FALSE;

#if CFG_SUPPORT_ROAMING
#if 0
	if (prRegInfo)
		prConnSettings->fgIsEnableRoaming = ((prRegInfo->fgDisRoaming > 0) ? (FALSE) : (TRUE));
#else
	if (prAdapter->rWifiVar.fgDisRoaming)
		prConnSettings->fgIsEnableRoaming = FALSE;
	else
		prConnSettings->fgIsEnableRoaming = TRUE;
#endif
#endif /* CFG_SUPPORT_ROAMING */

	prConnSettings->fgIsAdHocQoSEnable = FALSE;

#if CFG_SUPPORT_802_11AC
	prConnSettings->eDesiredPhyConfig = PHY_CONFIG_802_11ABGNAC;
#else
	prConnSettings->eDesiredPhyConfig = PHY_CONFIG_802_11ABGN;
#endif

	/* Set default bandwidth modes */
	prConnSettings->uc2G4BandwidthMode = CONFIG_BW_20M;
	prConnSettings->uc5GBandwidthMode = CONFIG_BW_20_40M;

	prConnSettings->rRsnInfo.ucElemId = 0x30;
	prConnSettings->rRsnInfo.u2Version = 0x0001;
	prConnSettings->rRsnInfo.u4GroupKeyCipherSuite = 0;
	prConnSettings->rRsnInfo.u4PairwiseKeyCipherSuiteCount = 0;
	for (i = 0; i < MAX_NUM_SUPPORTED_CIPHER_SUITES; i++)
		prConnSettings->rRsnInfo.au4PairwiseKeyCipherSuite[i] = 0;
	prConnSettings->rRsnInfo.u4AuthKeyMgtSuiteCount = 0;
	for (i = 0; i < MAX_NUM_SUPPORTED_AKM_SUITES; i++)
		prConnSettings->rRsnInfo.au4AuthKeyMgtSuite[i] = 0;
	prConnSettings->rRsnInfo.u2RsnCap = 0;
	prConnSettings->rRsnInfo.fgRsnCapPresent = FALSE;
}				/* end of aisFsmInitializeConnectionSettings() */

/*----------------------------------------------------------------------------*/
/*!
* @brief the function is used to initialize the value in AIS_FSM_INFO_T for
*        AIS FSM operation
*
* @param (none)
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmInit(IN struct ADAPTER *prAdapter)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo;
	uint8_t i;

	DEBUGFUNC("aisFsmInit()");
	DBGLOG(SW1, INFO, "->aisFsmInit()\n");

	/* avoid that the prAisBssInfo is realloc */
	if (prAdapter->prAisBssInfo != NULL)
		return;

	prAdapter->prAisBssInfo = prAisBssInfo = cnmGetBssInfoAndInit(prAdapter, NETWORK_TYPE_AIS, FALSE);
	ASSERT(prAisBssInfo);

	/* update MAC address */
	COPY_MAC_ADDR(prAdapter->prAisBssInfo->aucOwnMacAddr, prAdapter->rMyMacAddr);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisSpecificBssInfo = &(prAdapter->rWifiVar.rAisSpecificBssInfo);

	/* 4 <1> Initiate FSM */
	prAisFsmInfo->ePreviousState = AIS_STATE_IDLE;
	prAisFsmInfo->eCurrentState = AIS_STATE_IDLE;

	prAisFsmInfo->ucAvailableAuthTypes = 0;

	prAisFsmInfo->prTargetBssDesc = (struct BSS_DESC *) NULL;

	prAisFsmInfo->ucSeqNumOfReqMsg = 0;
	prAisFsmInfo->ucSeqNumOfChReq = 0;
	prAisFsmInfo->ucSeqNumOfScanReq = 0;
	prAisFsmInfo->u2SeqNumOfScanReport = AIS_SCN_REPORT_SEQ_NOT_SET;

	prAisFsmInfo->fgIsInfraChannelFinished = TRUE;
#if CFG_SUPPORT_ROAMING
	prAisFsmInfo->fgIsRoamingScanPending = FALSE;
#endif /* CFG_SUPPORT_ROAMING */
	prAisFsmInfo->fgIsChannelRequested = FALSE;
	prAisFsmInfo->fgIsChannelGranted = FALSE;
	prAisFsmInfo->u4PostponeIndStartTime = 0;
	/* Support AP Selection */
	prAisFsmInfo->ucJoinFailCntAfterScan = 0;

	/* 4 <1.1> Initiate FSM - Timer INIT */
	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rBGScanTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventBGSleepTimeOut, (unsigned long) NULL);

	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rIbssAloneTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventIbssAloneTimeOut, (unsigned long) NULL);

	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rScanDoneTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventScanDoneTimeOut, (unsigned long) NULL);

	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rJoinTimeoutTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventJoinTimeout, (unsigned long) NULL);

	cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rDeauthDoneTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventDeauthTimeout, (unsigned long) NULL);

	cnmTimerInitTimer(prAdapter,
				  &prAisFsmInfo->rWaitOkcPMKTimer,
				  (PFN_MGMT_TIMEOUT_FUNC)aisFsmSetOkcTimeout, (unsigned long) NULL);

		cnmTimerInitTimer(prAdapter,
			  &prAisFsmInfo->rSecModeChangeTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) aisFsmRunEventSecModeChangeTimeout, (unsigned long) NULL);

	/* 4 <1.2> Initiate PWR STATE */
	SET_NET_PWR_STATE_IDLE(prAdapter, prAisBssInfo->ucBssIndex);

	/* 4 <2> Initiate BSS_INFO_T - common part */
	BSS_INFO_INIT(prAdapter, prAisBssInfo);
	COPY_MAC_ADDR(prAisBssInfo->aucOwnMacAddr, prAdapter->rWifiVar.aucMacAddress);

	/* 4 <3> Initiate BSS_INFO_T - private part */
	/* TODO */
	prAisBssInfo->eBand = BAND_2G4;
	prAisBssInfo->ucPrimaryChannel = 1;
	prAisBssInfo->prStaRecOfAP = (struct STA_RECORD *) NULL;
	prAisBssInfo->ucNss = wlanGetSupportNss(prAdapter, prAisBssInfo->ucBssIndex);
	prAisBssInfo->ucWmmQueSet = (prAdapter->rWifiVar.eDbdcMode ==
		ENUM_DBDC_MODE_DISABLED) ? DBDC_5G_WMM_INDEX : DBDC_2G_WMM_INDEX;

	/* 4 <4> Allocate MSDU_INFO_T for Beacon */
	prAisBssInfo->prBeacon = cnmMgtPktAlloc(prAdapter,
						OFFSET_OF(struct WLAN_BEACON_FRAME, aucInfoElem[0]) + MAX_IE_LENGTH);

	if (prAisBssInfo->prBeacon) {
		prAisBssInfo->prBeacon->eSrc = TX_PACKET_MGMT;
		prAisBssInfo->prBeacon->ucStaRecIndex = 0xFF;	/* NULL STA_REC */
	} else {
		ASSERT(0);
	}

	prAisBssInfo->ucBMCWlanIndex = WTBL_RESERVED_ENTRY;

	for (i = 0; i < MAX_KEY_NUM; i++) {
		prAisBssInfo->ucBMCWlanIndexS[i] = WTBL_RESERVED_ENTRY;
		prAisBssInfo->ucBMCWlanIndexSUsed[i] = FALSE;
		prAisBssInfo->wepkeyUsed[i] = FALSE;
	}
#if 0
	prAisBssInfo->rPmProfSetupInfo.ucBmpDeliveryAC = PM_UAPSD_ALL;
	prAisBssInfo->rPmProfSetupInfo.ucBmpTriggerAC = PM_UAPSD_ALL;
	prAisBssInfo->rPmProfSetupInfo.ucUapsdSp = WMM_MAX_SP_LENGTH_2;
#else
	if (prAdapter->u4UapsdAcBmp == 0) {
		prAdapter->u4UapsdAcBmp = CFG_INIT_UAPSD_AC_BMP;
		/* ASSERT(prAdapter->u4UapsdAcBmp); */
	}
	prAisBssInfo->rPmProfSetupInfo.ucBmpDeliveryAC = (uint8_t) prAdapter->u4UapsdAcBmp;
	prAisBssInfo->rPmProfSetupInfo.ucBmpTriggerAC = (uint8_t) prAdapter->u4UapsdAcBmp;
	prAisBssInfo->rPmProfSetupInfo.ucUapsdSp = (uint8_t) prAdapter->u4MaxSpLen;
#endif

	/* request list initialization */
	LINK_INITIALIZE(&prAisFsmInfo->rPendingReqList);

	/* Support AP Selection */
	LINK_MGMT_INIT(&prAdapter->rWifiVar.rConnSettings.rBlackList);
		kalMemZero(&prAisSpecificBssInfo->arCurEssChnlInfo[0],
		sizeof(prAisSpecificBssInfo->arCurEssChnlInfo));
	LINK_INITIALIZE(&prAisSpecificBssInfo->rCurEssLink);
	/* end Support AP Selection */
	/* DBGPRINTF("[2] ucBmpDeliveryAC:0x%x, ucBmpTriggerAC:0x%x, ucUapsdSp:0x%x", */
	/* prAisBssInfo->rPmProfSetupInfo.ucBmpDeliveryAC, */
	/* prAisBssInfo->rPmProfSetupInfo.ucBmpTriggerAC, */
	/* prAisBssInfo->rPmProfSetupInfo.ucUapsdSp); */

	/* Bind NetDev & BssInfo */
	/* wlanBindBssIdxToNetInterface(prAdapter->prGlueInfo, NET_DEV_WLAN_IDX, prAisBssInfo->ucBssIndex); */
}				/* end of aisFsmInit() */

/*----------------------------------------------------------------------------*/
/*!
* @brief the function is used to uninitialize the value in AIS_FSM_INFO_T for
*        AIS FSM operation
*
* @param (none)
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmUninit(IN struct ADAPTER *prAdapter)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;

	DEBUGFUNC("aisFsmUninit()");
	DBGLOG(SW1, INFO, "->aisFsmUninit()\n");

	/* avoid that the prAisBssInfo is double freed */
	if (prAdapter->prAisBssInfo == NULL)
		return;

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;

	/* 4 <1> Stop all timers */
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rBGScanTimer);
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rIbssAloneTimer);
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rScanDoneTimer);
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rWaitOkcPMKTimer);
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rSecModeChangeTimer);

	/* 4 <2> flush pending request */
	aisFsmFlushRequest(prAdapter);

	/* 4 <3> Reset driver-domain BSS-INFO */
	if (prAisBssInfo) {

		if (prAisBssInfo->prBeacon) {
			cnmMgtPktFree(prAdapter, prAisBssInfo->prBeacon);
			prAisBssInfo->prBeacon = NULL;
		}

		cnmFreeBssInfo(prAdapter, prAisBssInfo);
		prAdapter->prAisBssInfo = NULL;
	}

#if CFG_SUPPORT_802_11W
	rsnStopSaQuery(prAdapter);
#endif
		/* Support AP Selection */
		LINK_MGMT_UNINIT(&prAdapter->rWifiVar.rConnSettings.rBlackList,
			struct AIS_BLACKLIST_ITEM, VIR_MEM_TYPE);
		/* end Support AP Selection */
}				/* end of aisFsmUninit() */

/*----------------------------------------------------------------------------*/
/*!
* @brief Initialization of JOIN STATE
*
* @param[in] prBssDesc  The pointer of BSS_DESC_T which is the BSS we will try to join with.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmStateInit_JOIN(IN struct ADAPTER *prAdapter, struct BSS_DESC *prBssDesc)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct AIS_SPECIFIC_BSS_INFO *prAisSpecificBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct STA_RECORD *prStaRec;
	struct MSG_SAA_FSM_START *prJoinReqMsg;

	DEBUGFUNC("aisFsmStateInit_JOIN()");

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;
	prAisSpecificBssInfo = &(prAdapter->rWifiVar.rAisSpecificBssInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	ASSERT(prBssDesc);

	/* 4 <1> We are going to connect to this BSS. */
	prBssDesc->fgIsConnecting = TRUE;

	/* 4 <2> Setup corresponding STA_RECORD_T */
	prStaRec = bssCreateStaRecFromBssDesc(prAdapter,
					      STA_TYPE_LEGACY_AP, prAdapter->prAisBssInfo->ucBssIndex, prBssDesc);

	prAisFsmInfo->prTargetStaRec = prStaRec;

	/* 4 <2.1> sync. to firmware domain */
	if (prStaRec->ucStaState == STA_STATE_1)
		cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

	/* 4 <3> Update ucAvailableAuthTypes which we can choice during SAA */
	if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_DISCONNECTED) {

		prStaRec->fgIsReAssoc = FALSE;

		switch (prConnSettings->eAuthMode) {
		case AUTH_MODE_OPEN:	/* Note: Omit break here. */
		case AUTH_MODE_WPA:
		case AUTH_MODE_WPA_PSK:
		case AUTH_MODE_WPA2:
		case AUTH_MODE_WPA2_PSK:
		case AUTH_MODE_WPA_OSEN:
			prAisFsmInfo->ucAvailableAuthTypes = (uint8_t) AUTH_TYPE_OPEN_SYSTEM;
			break;

		case AUTH_MODE_SHARED:
			prAisFsmInfo->ucAvailableAuthTypes = (uint8_t) AUTH_TYPE_SHARED_KEY;
			break;

		case AUTH_MODE_AUTO_SWITCH:
			DBGLOG(AIS, LOUD, "JOIN INIT: eAuthMode == AUTH_MODE_AUTO_SWITCH\n");
			prAisFsmInfo->ucAvailableAuthTypes = (uint8_t) (AUTH_TYPE_OPEN_SYSTEM | AUTH_TYPE_SHARED_KEY);
			break;

		default:
			ASSERT(!(prConnSettings->eAuthMode == AUTH_MODE_WPA_NONE));
			DBGLOG(AIS, ERROR,
			       "JOIN INIT: Auth Algorithm : %d was not supported by JOIN\n", prConnSettings->eAuthMode);
			/* TODO(Kevin): error handling ? */
			return;
		}

		/* TODO(tyhsu): Assume that Roaming Auth Type is equal to ConnSettings eAuthMode */
		prAisSpecificBssInfo->ucRoamingAuthTypes = prAisFsmInfo->ucAvailableAuthTypes;

		prStaRec->ucTxAuthAssocRetryLimit = TX_AUTH_ASSOCI_RETRY_LIMIT;

		/* Update Bss info before join */
		prAisBssInfo->eBand = prBssDesc->eBand;
		prAisBssInfo->ucPrimaryChannel = prBssDesc->ucChannelNum;

	} else {
		ASSERT(prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE);

		DBGLOG(AIS, LOUD, "JOIN INIT: AUTH TYPE = %d for Roaming\n", prAisSpecificBssInfo->ucRoamingAuthTypes);

		prStaRec->fgIsReAssoc = TRUE;	/* We do roaming while the medium is connected */

		/* TODO(Kevin): We may call a sub function to acquire the Roaming Auth Type */
		prAisFsmInfo->ucAvailableAuthTypes = prAisSpecificBssInfo->ucRoamingAuthTypes;

		prStaRec->ucTxAuthAssocRetryLimit = TX_AUTH_ASSOCI_RETRY_LIMIT_FOR_ROAMING;
	}

	/* 4 <4> Use an appropriate Authentication Algorithm Number among the ucAvailableAuthTypes */
	if (prAisFsmInfo->ucAvailableAuthTypes & (uint8_t) AUTH_TYPE_SHARED_KEY) {

		DBGLOG(AIS, LOUD, "JOIN INIT: Try to do Authentication with AuthType == SHARED_KEY.\n");

		prAisFsmInfo->ucAvailableAuthTypes &= ~(uint8_t) AUTH_TYPE_SHARED_KEY;

		prStaRec->ucAuthAlgNum = (uint8_t) AUTH_ALGORITHM_NUM_SHARED_KEY;
	} else if (prAisFsmInfo->ucAvailableAuthTypes & (uint8_t) AUTH_TYPE_OPEN_SYSTEM) {

		DBGLOG(AIS, LOUD, "JOIN INIT: Try to do Authentication with AuthType == OPEN_SYSTEM.\n");
		prAisFsmInfo->ucAvailableAuthTypes &= ~(uint8_t) AUTH_TYPE_OPEN_SYSTEM;

		prStaRec->ucAuthAlgNum = (uint8_t) AUTH_ALGORITHM_NUM_OPEN_SYSTEM;
	} else if (prAisFsmInfo->ucAvailableAuthTypes & (uint8_t) AUTH_TYPE_FAST_BSS_TRANSITION) {

		DBGLOG(AIS, LOUD, "JOIN INIT: Try to do Authentication with AuthType == FAST_BSS_TRANSITION.\n");

		prAisFsmInfo->ucAvailableAuthTypes &= ~(uint8_t) AUTH_TYPE_FAST_BSS_TRANSITION;

		prStaRec->ucAuthAlgNum = (uint8_t) AUTH_ALGORITHM_NUM_FAST_BSS_TRANSITION;
	} else {
		ASSERT(0);
	}

	/* 4 <5> Overwrite Connection Setting for eConnectionPolicy == ANY (Used by Assoc Req) */
	if (prBssDesc->ucSSIDLen)
		COPY_SSID(prConnSettings->aucSSID, prConnSettings->ucSSIDLen, prBssDesc->aucSSID, prBssDesc->ucSSIDLen);
	/* 4 <6> Send a Msg to trigger SAA to start JOIN process. */
	prJoinReqMsg = (struct MSG_SAA_FSM_START *) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(struct MSG_SAA_FSM_START));
	if (!prJoinReqMsg) {

		ASSERT(0);	/* Can't trigger SAA FSM */
		return;
	}

	prJoinReqMsg->rMsgHdr.eMsgId = MID_AIS_SAA_FSM_START;
	prJoinReqMsg->ucSeqNum = ++prAisFsmInfo->ucSeqNumOfReqMsg;
	prJoinReqMsg->prStaRec = prStaRec;

	if (1) {
		int j;
		struct FRAG_INFO *prFragInfo;

		for (j = 0; j < MAX_NUM_CONCURRENT_FRAGMENTED_MSDUS; j++) {
			prFragInfo = &prStaRec->rFragInfo[j];

			if (prFragInfo->pr1stFrag) {
				/* nicRxReturnRFB(prAdapter, prFragInfo->pr1stFrag); */
				prFragInfo->pr1stFrag = (struct SW_RFB *) NULL;
			}
		}
	}

	mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *) prJoinReqMsg, MSG_SEND_METHOD_BUF);
}				/* end of aisFsmInit_JOIN() */

/*----------------------------------------------------------------------------*/
/*!
* @brief Retry JOIN for AUTH_MODE_AUTO_SWITCH
*
* @param[in] prStaRec       Pointer to the STA_RECORD_T
*
* @retval TRUE      We will retry JOIN
* @retval FALSE     We will not retry JOIN
*/
/*----------------------------------------------------------------------------*/
u_int8_t aisFsmStateInit_RetryJOIN(IN struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct MSG_SAA_FSM_START *prJoinReqMsg;

	DEBUGFUNC("aisFsmStateInit_RetryJOIN()");

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	/* Retry other AuthType if possible */
	if (!prAisFsmInfo->ucAvailableAuthTypes)
		return FALSE;

	if (prAisFsmInfo->ucAvailableAuthTypes & (uint8_t) AUTH_TYPE_OPEN_SYSTEM) {

		DBGLOG(AIS, INFO, "RETRY JOIN INIT: Retry Authentication with AuthType == OPEN_SYSTEM.\n");

		prAisFsmInfo->ucAvailableAuthTypes &= ~(uint8_t) AUTH_TYPE_OPEN_SYSTEM;

		prStaRec->ucAuthAlgNum = (uint8_t) AUTH_ALGORITHM_NUM_OPEN_SYSTEM;
	} else {
		DBGLOG(AIS, ERROR, "RETRY JOIN INIT: Retry Authentication with Unexpected AuthType.\n");
		ASSERT(0);
	}

	prAisFsmInfo->ucAvailableAuthTypes = 0;	/* No more available Auth Types */

	/* Trigger SAA to start JOIN process. */
	prJoinReqMsg = (struct MSG_SAA_FSM_START *) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(struct MSG_SAA_FSM_START));
	if (!prJoinReqMsg) {

		ASSERT(0);	/* Can't trigger SAA FSM */
		return FALSE;
	}

	prJoinReqMsg->rMsgHdr.eMsgId = MID_AIS_SAA_FSM_START;
	prJoinReqMsg->ucSeqNum = ++prAisFsmInfo->ucSeqNumOfReqMsg;
	prJoinReqMsg->prStaRec = prStaRec;

	mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *) prJoinReqMsg, MSG_SEND_METHOD_BUF);

	return TRUE;

}				/* end of aisFsmRetryJOIN() */

#if CFG_SUPPORT_ADHOC
/*----------------------------------------------------------------------------*/
/*!
* @brief State Initialization of AIS_STATE_IBSS_ALONE
*
* @param (none)
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmStateInit_IBSS_ALONE(IN struct ADAPTER *prAdapter)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_INFO *prAisBssInfo;

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	prAisBssInfo = prAdapter->prAisBssInfo;

	/* 4 <1> Check if IBSS was created before ? */
	if (prAisBssInfo->fgIsBeaconActivated) {

		/* 4 <2> Start IBSS Alone Timer for periodic SCAN and then SEARCH */
#if !CFG_SLT_SUPPORT
		cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rIbssAloneTimer, SEC_TO_MSEC(AIS_IBSS_ALONE_TIMEOUT_SEC));
#endif
	}

	aisFsmCreateIBSS(prAdapter);
}				/* end of aisFsmStateInit_IBSS_ALONE() */

/*----------------------------------------------------------------------------*/
/*!
* @brief State Initialization of AIS_STATE_IBSS_MERGE
*
* @param[in] prBssDesc  The pointer of BSS_DESC_T which is the IBSS we will try to merge with.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmStateInit_IBSS_MERGE(IN struct ADAPTER *prAdapter, struct BSS_DESC *prBssDesc)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_INFO *prAisBssInfo;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;

	ASSERT(prBssDesc);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	prAisBssInfo = prAdapter->prAisBssInfo;

	/* 4 <1> We will merge with to this BSS immediately. */
	prBssDesc->fgIsConnecting = FALSE;
	prBssDesc->fgIsConnected = TRUE;

	/* 4 <2> Setup corresponding STA_RECORD_T */
	prStaRec = bssCreateStaRecFromBssDesc(prAdapter,
					      STA_TYPE_ADHOC_PEER, prAdapter->prAisBssInfo->ucBssIndex, prBssDesc);

	prStaRec->fgIsMerging = TRUE;

	prAisFsmInfo->prTargetStaRec = prStaRec;

	/* 4 <2.1> sync. to firmware domain */
	cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_1);

	/* 4 <3> IBSS-Merge */
	aisFsmMergeIBSS(prAdapter, prStaRec);
}				/* end of aisFsmStateInit_IBSS_MERGE() */

#endif /* CFG_SUPPORT_ADHOC */

/*----------------------------------------------------------------------------*/
/*!
* @brief Process of JOIN Abort
*
* @param (none)
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmStateAbort_JOIN(IN struct ADAPTER *prAdapter)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct MSG_SAA_FSM_ABORT *prJoinAbortMsg;

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	/* 1. Abort JOIN process */
	prJoinAbortMsg = (struct MSG_SAA_FSM_ABORT *) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(struct MSG_SAA_FSM_ABORT));
	if (!prJoinAbortMsg) {

		ASSERT(0);	/* Can't abort SAA FSM */
		return;
	}

	prJoinAbortMsg->rMsgHdr.eMsgId = MID_AIS_SAA_FSM_ABORT;
	prJoinAbortMsg->ucSeqNum = prAisFsmInfo->ucSeqNumOfReqMsg;
	prJoinAbortMsg->prStaRec = prAisFsmInfo->prTargetStaRec;

	prAisFsmInfo->prTargetBssDesc->fgIsConnected = FALSE;
	prAisFsmInfo->prTargetBssDesc->fgIsConnecting = FALSE;

	mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *) prJoinAbortMsg, MSG_SEND_METHOD_BUF);

	/* 2. Return channel privilege */
	aisFsmReleaseCh(prAdapter);

	/* 3.1 stop join timeout timer */
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);

	/* 3.2 reset local variable */
	prAisFsmInfo->fgIsInfraChannelFinished = TRUE;
}				/* end of aisFsmAbortJOIN() */

/*----------------------------------------------------------------------------*/
/*!
* @brief Process of SCAN Abort
*
* @param (none)
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmStateAbort_SCAN(IN struct ADAPTER *prAdapter)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct MSG_SCN_SCAN_CANCEL *prScanCancelMsg;

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	DBGLOG(AIS, STATE, "aisFsmStateAbort_SCAN\n");

	/* Abort JOIN process. */
	prScanCancelMsg = (struct MSG_SCN_SCAN_CANCEL *) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(struct MSG_SCN_SCAN_CANCEL));
	if (!prScanCancelMsg) {

		ASSERT(0);	/* Can't abort SCN FSM */
		return;
	}

	prScanCancelMsg->rMsgHdr.eMsgId = MID_AIS_SCN_SCAN_CANCEL;
	prScanCancelMsg->ucSeqNum = prAisFsmInfo->ucSeqNumOfScanReq;
	prScanCancelMsg->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
	prScanCancelMsg->fgIsChannelExt = FALSE;

	/* unbuffered message to guarantee scan is cancelled in sequence */
	mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *) prScanCancelMsg, MSG_SEND_METHOD_UNBUF);
}				/* end of aisFsmAbortSCAN() */

/*----------------------------------------------------------------------------*/
/*!
* @brief Process of NORMAL_TR Abort
*
* @param (none)
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmStateAbort_NORMAL_TR(IN struct ADAPTER *prAdapter)
{
	struct AIS_FSM_INFO *prAisFsmInfo;

	ASSERT(prAdapter);
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	/* TODO(Kevin): Do abort other MGMT func */

	/* 1. Release channel to CNM */
	aisFsmReleaseCh(prAdapter);

	/* 2.1 stop join timeout timer */
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);

	/* 2.2 reset local variable */
	prAisFsmInfo->fgIsInfraChannelFinished = TRUE;
}				/* end of aisFsmAbortNORMAL_TR() */

#if CFG_SUPPORT_ADHOC
/*----------------------------------------------------------------------------*/
/*!
* @brief Process of NORMAL_TR Abort
*
* @param (none)
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmStateAbort_IBSS(IN struct ADAPTER *prAdapter)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_DESC *prBssDesc;

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	/* reset BSS-DESC */
	if (prAisFsmInfo->prTargetStaRec) {
		prBssDesc = scanSearchBssDescByTA(prAdapter, prAisFsmInfo->prTargetStaRec->aucMacAddr);

		if (prBssDesc) {
			prBssDesc->fgIsConnected = FALSE;
			prBssDesc->fgIsConnecting = FALSE;
		}
	}
	/* release channel privilege */
	aisFsmReleaseCh(prAdapter);
}
#endif /* CFG_SUPPORT_ADHOC */

/*----------------------------------------------------------------------------*/
/*!
* @brief The Core FSM engine of AIS(Ad-hoc, Infra STA)
*
* @param[in] eNextState Enum value of next AIS STATE
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmSteps(IN struct ADAPTER *prAdapter, enum ENUM_AIS_STATE eNextState)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_DESC *prBssDesc;
	struct MSG_CH_REQ *prMsgChReq;
	struct MSG_SCN_SCAN_REQ_V2 *prScanReqMsg;
	struct AIS_REQ_HDR *prAisReq;
	enum ENUM_BAND eBand;
	uint8_t ucChannel;
	uint16_t u2ScanIELen;
	u_int8_t fgIsTransition = (u_int8_t) FALSE;
#if CFG_SUPPORT_DBDC
	struct CNM_DBDC_CAP rDbdcCap;
#endif /*CFG_SUPPORT_DBDC*/
	uint8_t ucRfBw;

	DEBUGFUNC("aisFsmSteps()");

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	do {

		/* Do entering Next State */
		prAisFsmInfo->ePreviousState = prAisFsmInfo->eCurrentState;

		DBGLOG(AIS, STATE, "[AIS]TRANSITION: [%s] -> [%s]\n",
			apucDebugAisState[prAisFsmInfo->eCurrentState], apucDebugAisState[eNextState]);

		/* NOTE(Kevin): This is the only place to change the eCurrentState(except initial) */
		prAisFsmInfo->eCurrentState = eNextState;

		fgIsTransition = (u_int8_t) FALSE;

		aisPostponedEventOfDisconnTimeout(prAdapter, prAisFsmInfo);

		/* Do tasks of the State that we just entered */
		switch (prAisFsmInfo->eCurrentState) {
			/* NOTE(Kevin): we don't have to rearrange the sequence of following
			 * switch case. Instead I would like to use a common lookup table of array
			 * of function pointer to speed up state search.
			 */
		case AIS_STATE_IDLE:

			prAisReq = aisFsmGetNextRequest(prAdapter);
			cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rScanDoneTimer);
			cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rWaitOkcPMKTimer);

			if (prAisReq)
				DBGLOG(AIS, TRACE, "eReqType=%d, fgIsConnReqIssued=%d, DisByNonRequest=%d\n",
					prAisReq->eReqType, prConnSettings->fgIsConnReqIssued,
					prConnSettings->fgIsDisconnectedByNonRequest);
			if (prAisReq == NULL || prAisReq->eReqType == AIS_REQUEST_RECONNECT) {
				if (prConnSettings->fgIsConnReqIssued == TRUE &&
				    prConnSettings->fgIsDisconnectedByNonRequest == FALSE) {

					prAisFsmInfo->fgTryScan = TRUE;

					if (!IS_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex)) {
						SET_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
						/* sync with firmware */
						nicActivateNetwork(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
					}

					SET_NET_PWR_STATE_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
#if CFG_SUPPORT_PNO
					prAisBssInfo->fgIsNetRequestInActive = FALSE;
#endif
					/* reset trial count */
					prAisFsmInfo->ucConnTrialCount = 0;

					eNextState = AIS_STATE_SEARCH;
					fgIsTransition = TRUE;
				} else {
					SET_NET_PWR_STATE_IDLE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

					/* sync with firmware */
#if CFG_SUPPORT_PNO
					prAisBssInfo->fgIsNetRequestInActive = TRUE;
					if (prAisBssInfo->fgIsPNOEnable) {
						DBGLOG(BSS, INFO,
						       "[BSSidx][Network]=%d PNOEnable&&OP_MODE_INFRASTRUCTURE,KEEP ACTIVE\n",
						       prAisBssInfo->ucBssIndex);
					} else
#endif
					{
						UNSET_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
						nicDeactivateNetwork(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
					}

					/* check for other pending request */
					if (prAisReq && (aisFsmIsRequestPending
							 (prAdapter, AIS_REQUEST_SCAN, TRUE) == TRUE)) {
						wlanClearScanningResult(prAdapter);
						eNextState = AIS_STATE_SCAN;

						fgIsTransition = TRUE;
					}
				}

				if (prAisReq) {
					/* free the message */
					cnmMemFree(prAdapter, prAisReq);
				}
			} else if (prAisReq->eReqType == AIS_REQUEST_SCAN) {
#if CFG_SUPPORT_ROAMING
				prAisFsmInfo->fgIsRoamingScanPending = FALSE;
#endif /* CFG_SUPPORT_ROAMING */
				wlanClearScanningResult(prAdapter);

				eNextState = AIS_STATE_SCAN;
				fgIsTransition = TRUE;

				/* free the message */
				cnmMemFree(prAdapter, prAisReq);
			} else if (prAisReq->eReqType == AIS_REQUEST_ROAMING_CONNECT
				   || prAisReq->eReqType == AIS_REQUEST_ROAMING_SEARCH) {
				/* ignore */
				/* free the message */
				cnmMemFree(prAdapter, prAisReq);
			} else if (prAisReq->eReqType == AIS_REQUEST_REMAIN_ON_CHANNEL) {
				eNextState = AIS_STATE_REQ_REMAIN_ON_CHANNEL;
				fgIsTransition = TRUE;

				/* free the message */
				cnmMemFree(prAdapter, prAisReq);
			}

			prAisFsmInfo->u4SleepInterval = AIS_BG_SCAN_INTERVAL_MIN_SEC;

			break;

		case AIS_STATE_SEARCH:
			/* 4 <1> Search for a matched candidate and save it to prTargetBssDesc. */
#if CFG_SLT_SUPPORT
			prBssDesc = prAdapter->rWifiVar.rSltInfo.prPseudoBssDesc;
#else
			/* Support AP Selection */
			if (prAisFsmInfo->ucJoinFailCntAfterScan >= SCN_BSS_JOIN_FAIL_THRESOLD) {
				prBssDesc = NULL;
				DBGLOG(AIS, STATE,
					"Failed to connect %s more than 4 times after last scan, scan again\n",
					prConnSettings->aucSSID);
			} else {
#if CFG_SELECT_BSS_BASE_ON_MULTI_PARAM
				prBssDesc = scanSearchBssDescByScoreForAis(prAdapter);
#else
				prBssDesc = scanSearchBssDescByPolicy(prAdapter, NETWORK_TYPE_AIS_INDEX);
#endif
			}
#endif
			/* we are under Roaming Condition. */
			if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
				if (prAisFsmInfo->ucConnTrialCount > AIS_ROAMING_CONNECTION_TRIAL_LIMIT) {
#if CFG_SUPPORT_ROAMING
					DBGLOG(AIS, STATE,
						"Roaming retry count :%d fail!\n", prAisFsmInfo->ucConnTrialCount);
					roamingFsmRunEventFail(prAdapter, ROAMING_FAIL_REASON_CONNLIMIT);
#endif /* CFG_SUPPORT_ROAMING */
					/* reset retry count */
					prAisFsmInfo->ucConnTrialCount = 0;

					/* abort connection trial */
					if (prConnSettings->eReConnectLevel < RECONNECT_LEVEL_BEACON_TIMEOUT) {
						prConnSettings->eReConnectLevel = RECONNECT_LEVEL_ROAMING_FAIL;
						prConnSettings->fgIsConnReqIssued = FALSE;
					} else {
						DBGLOG(AIS, INFO,
						       "Do not set fgIsConnReqIssued, level is %d\n",
						       prConnSettings->eReConnectLevel);
					}

					eNextState = AIS_STATE_NORMAL_TR;
					fgIsTransition = TRUE;

					break;
				}
			}
			/* 4 <2> We are not under Roaming Condition. */
			if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_DISCONNECTED) {

				/* 4 <2.a> If we have the matched one */
				if (prBssDesc) {
					/* 4 <A> Stored the Selected BSS security cipher. */
					/* or later asoc req compose IE */
					prAisBssInfo->u4RsnSelectedGroupCipher = prBssDesc->u4RsnSelectedGroupCipher;
					prAisBssInfo->u4RsnSelectedPairwiseCipher =
					    prBssDesc->u4RsnSelectedPairwiseCipher;
					prAisBssInfo->u4RsnSelectedAKMSuite = prBssDesc->u4RsnSelectedAKMSuite;
#if CFG_SUPPORT_DBDC
					cnmDbdcEnableDecision(prAdapter, prAisBssInfo->ucBssIndex, prBssDesc->eBand);
					cnmGetDbdcCapability(prAdapter, prAisBssInfo->ucBssIndex,
						prBssDesc->eBand, prBssDesc->ucChannelNum,
						wlanGetSupportNss(prAdapter, prAisBssInfo->ucBssIndex), &rDbdcCap);

					prAisBssInfo->ucNss = rDbdcCap.ucNss;
					prAisBssInfo->ucWmmQueSet = rDbdcCap.ucWmmSetIndex;
#endif /*CFG_SUPPORT_DBDC*/
					/* 4 <B> Do STATE transition and update current Operation Mode. */
					if (prBssDesc->eBSSType == BSS_TYPE_INFRASTRUCTURE) {

						prAisBssInfo->eCurrentOPMode = OP_MODE_INFRASTRUCTURE;

						/* Record the target BSS_DESC_T for next STATE. */
						prAisFsmInfo->prTargetBssDesc = prBssDesc;

						/* Transit to channel acquire */
						eNextState = AIS_STATE_REQ_CHANNEL_JOIN;
						fgIsTransition = TRUE;

						/* increase connection trial count */
						prAisFsmInfo->ucConnTrialCount++;
					}
#if CFG_SUPPORT_ADHOC
					else if (prBssDesc->eBSSType == BSS_TYPE_IBSS) {

						prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;

						/* Record the target BSS_DESC_T for next STATE. */
						prAisFsmInfo->prTargetBssDesc = prBssDesc;

						eNextState = AIS_STATE_IBSS_MERGE;
						fgIsTransition = TRUE;
					}
#endif /* CFG_SUPPORT_ADHOC */
					else {
						ASSERT(0);
						eNextState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
						fgIsTransition = TRUE;
					}
				}
				/* 4 <2.b> If we don't have the matched one */
				else {
					if (prAisFsmInfo->rJoinReqTime != 0 &&
						CHECK_FOR_TIMEOUT(kalGetTimeTick(),
								  prAisFsmInfo->rJoinReqTime,
								  SEC_TO_SYSTIME(AIS_JOIN_TIMEOUT))) {
						eNextState = AIS_STATE_JOIN_FAILURE;
						fgIsTransition = TRUE;
						break;
					}
					/* increase connection trial count for infrastructure connection */
					if (prConnSettings->eOPMode == NET_TYPE_INFRA)
						prAisFsmInfo->ucConnTrialCount++;

					/* 4 <A> Try to SCAN */
					if (prAisFsmInfo->fgTryScan) {
						eNextState = AIS_STATE_LOOKING_FOR;

						fgIsTransition = TRUE;
					}
					/* 4 <B> We've do SCAN already, now wait in some STATE. */
					else {
						eNextState = aisFsmStateSearchAction(prAdapter,
									AIS_FSM_STATE_SEARCH_ACTION_PHASE_0);
						fgIsTransition = TRUE;
					}
				}
			}
			/* 4 <3> We are under Roaming Condition. */
			else {	/* prAdapter->eConnectionState == MEDIA_STATE_CONNECTED. */

				/* 4 <3.a> This BSS_DESC_T is our AP. */
				/* NOTE(Kevin 2008/05/16): Following cases will go back to NORMAL_TR.
				 * CASE I: During Roaming, APP(WZC/NDISTEST) change the connection
				 *         settings. That make we can NOT match the original AP, so the
				 *         prBssDesc is NULL.
				 * CASE II: The same reason as CASE I. Because APP change the
				 *          eOPMode to other network type in connection setting
				 *          (e.g. NET_TYPE_IBSS), so the BssDesc become the IBSS node.
				 * (For CASE I/II, before WZC/NDISTEST set the OID_SSID, it will change
				 * other parameters in connection setting first. So if we do roaming
				 * at the same time, it will hit these cases.)
				 *
				 * CASE III: Normal case, we can't find other candidate to roam
				 * out, so only the current AP will be matched.
				 *
				 * CASE VI: Timestamp of the current AP might be reset
				 */
				if (prAisBssInfo->ucReasonOfDisconnect != DISCONNECT_REASON_CODE_REASSOCIATION &&
					((!prBssDesc) ||    /* CASE I */
					(prBssDesc->eBSSType != BSS_TYPE_INFRASTRUCTURE) ||	/* CASE II */
					(prBssDesc->fgIsConnected) ||	/* CASE III */
					(EQUAL_MAC_ADDR(prBssDesc->aucBSSID, prAisBssInfo->aucBSSID))) /* CASE VI */) {
					if (prBssDesc) {
						DBGLOG(ROAMING, INFO,
						"fgIsConnected=%d, prBssDesc->BSSID %pM, prAisBssInfo->BSSID %pM\n",
						prBssDesc->fgIsConnected, prBssDesc->aucBSSID, prAisBssInfo->aucBSSID);
					}
#if DBG
					if ((prBssDesc) && (prBssDesc->fgIsConnected))
						ASSERT(EQUAL_MAC_ADDR(prBssDesc->aucBSSID, prAisBssInfo->aucBSSID));
#endif /* DBG */
					/* We already associated with it, go back to NORMAL_TR */
					/* TODO(Kevin): Roaming Fail */
#if CFG_SUPPORT_ROAMING
					roamingFsmRunEventFail(prAdapter, ROAMING_FAIL_REASON_NOCANDIDATE);
#endif /* CFG_SUPPORT_ROAMING */

					/* Retreat to NORMAL_TR state */
					eNextState = AIS_STATE_NORMAL_TR;
					fgIsTransition = TRUE;
				}
				/* 4 <3.b> Try to roam out for JOIN this BSS_DESC_T. */
				else {
					if (prBssDesc == NULL) {
						fgIsTransition = TRUE;
						eNextState = aisFsmStateSearchAction(prAdapter,
									AIS_FSM_STATE_SEARCH_ACTION_PHASE_1);
					} else {
						aisFsmStateSearchAction(prAdapter, AIS_FSM_STATE_SEARCH_ACTION_PHASE_2);
						/* 4 <A> Record the target BSS_DESC_T for next STATE. */
						prAisFsmInfo->prTargetBssDesc = prBssDesc;

						/* tyhsu: increase connection trial count */
						prAisFsmInfo->ucConnTrialCount++;

						/* Transit to channel acquire */
						eNextState = AIS_STATE_REQ_CHANNEL_JOIN;
						fgIsTransition = TRUE;
					}
				}
			}
			if (prBssDesc && prConnSettings->fgOkcEnabled) {
				uint8_t aucBuf[sizeof(struct PARAM_PMKID_CANDIDATE_LIST) + sizeof(struct PARAM_STATUS_INDICATION)];
				struct PARAM_STATUS_INDICATION *prStatusEvent = (struct PARAM_STATUS_INDICATION *)aucBuf;
				struct PARAM_PMKID_CANDIDATE_LIST *prPmkidCandicate =
					(struct PARAM_PMKID_CANDIDATE_LIST *)(prStatusEvent+1);
				uint32_t u4Entry = 0;

				if (rsnSearchPmkidEntry(prAdapter, prBssDesc->aucBSSID, &u4Entry) &&
					prAdapter->rWifiVar.rAisSpecificBssInfo.arPmkidCache[u4Entry].fgPmkidExist)
					break;
				DBGLOG(AIS, INFO, "No PMK for %pM, try to generate a OKC PMK\n", prBssDesc->aucBSSID);
				prStatusEvent->eStatusType = ENUM_STATUS_TYPE_CANDIDATE_LIST;
				prPmkidCandicate->u4Version = 1;
				prPmkidCandicate->u4NumCandidates = 1;
				prPmkidCandicate->arCandidateList[0].u4Flags = 0; /* don't request preauth */
				COPY_MAC_ADDR(prPmkidCandicate->arCandidateList[0].arBSSID, prBssDesc->aucBSSID);
				kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
					 WLAN_STATUS_MEDIA_SPECIFIC_INDICATION, (void *)aucBuf, sizeof(aucBuf));
				cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rWaitOkcPMKTimer, AIS_WAIT_OKC_PMKID_SEC);
			}

			break;

		case AIS_STATE_WAIT_FOR_NEXT_SCAN:

			DBGLOG(AIS, LOUD, "SCAN: Idle Begin - Current Time = %u\n", kalGetTimeTick());

			cnmTimerStartTimer(prAdapter,
					   &prAisFsmInfo->rBGScanTimer, SEC_TO_MSEC(prAisFsmInfo->u4SleepInterval));

			SET_NET_PWR_STATE_IDLE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

			if (prAisFsmInfo->u4SleepInterval < AIS_BG_SCAN_INTERVAL_MAX_SEC)
				prAisFsmInfo->u4SleepInterval <<= 1;

			break;

		case AIS_STATE_SCAN:
		case AIS_STATE_ONLINE_SCAN:
		case AIS_STATE_LOOKING_FOR:

			if (!IS_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex)) {
				SET_NET_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

				/* sync with firmware */
				nicActivateNetwork(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
#if CFG_SUPPORT_PNO
				prAisBssInfo->fgIsNetRequestInActive = FALSE;
#endif
			}

			/* IE length decision */
			if (prAisFsmInfo->u4ScanIELength > 0) {
				u2ScanIELen = (uint16_t) prAisFsmInfo->u4ScanIELength;
			} else {
#if CFG_SUPPORT_WPS2
				u2ScanIELen = prAdapter->prGlueInfo->u2WSCIELen;
#else
				u2ScanIELen = 0;
#endif
			}

			prScanReqMsg = (struct MSG_SCN_SCAN_REQ_V2 *) cnmMemAlloc(prAdapter,
									   RAM_TYPE_MSG,
									   OFFSET_OF
									   (struct MSG_SCN_SCAN_REQ_V2, aucIE) + u2ScanIELen);
			if (!prScanReqMsg) {
				ASSERT(0);	/* Can't trigger SCAN FSM */
				return;
			}
			kalMemZero(prScanReqMsg, OFFSET_OF
				(struct MSG_SCN_SCAN_REQ_V2, aucIE) +
				u2ScanIELen);
			prScanReqMsg->rMsgHdr.eMsgId = MID_AIS_SCN_SCAN_REQ_V2;
			prScanReqMsg->ucSeqNum = ++prAisFsmInfo->ucSeqNumOfScanReq;
			if (prAisFsmInfo->u2SeqNumOfScanReport ==
					AIS_SCN_REPORT_SEQ_NOT_SET) {
				prAisFsmInfo->u2SeqNumOfScanReport =
					(uint16_t)prScanReqMsg->ucSeqNum;
			}
			prScanReqMsg->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;

#if CFG_SUPPORT_RDD_TEST_MODE
			prScanReqMsg->eScanType = SCAN_TYPE_PASSIVE_SCAN;
#else
			if (prAisFsmInfo->eCurrentState == AIS_STATE_SCAN
			    || prAisFsmInfo->eCurrentState == AIS_STATE_ONLINE_SCAN) {
				uint8_t ucScanSSIDNum;
				enum ENUM_SCAN_TYPE eScanType;

				ucScanSSIDNum = prAisFsmInfo->ucScanSSIDNum;
				eScanType = prAisFsmInfo->eScanType;

				if (eScanType == SCAN_TYPE_ACTIVE_SCAN
					&& ucScanSSIDNum == 0) {
					prScanReqMsg->eScanType = eScanType;

					prScanReqMsg->ucSSIDType
						= SCAN_REQ_SSID_WILDCARD;
					prScanReqMsg->ucSSIDNum = 0;
				} else if (eScanType == SCAN_TYPE_PASSIVE_SCAN
					&& ucScanSSIDNum == 0) {
					prScanReqMsg->eScanType = eScanType;

					prScanReqMsg->ucSSIDType = 0;
					prScanReqMsg->ucSSIDNum = 0;
				} else {
					prScanReqMsg->eScanType = SCAN_TYPE_ACTIVE_SCAN;

					prScanReqMsg->ucSSIDType = SCAN_REQ_SSID_SPECIFIED;
					prScanReqMsg->ucSSIDNum = ucScanSSIDNum;
					prScanReqMsg->prSsid = prAisFsmInfo->arScanSSID;
				}
			} else {
				prScanReqMsg->eScanType = SCAN_TYPE_ACTIVE_SCAN;

				COPY_SSID(prAisFsmInfo->rRoamingSSID.aucSsid,
					  prAisFsmInfo->rRoamingSSID.u4SsidLen,
					  prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

				/* Scan for determined SSID */
				prScanReqMsg->ucSSIDType = SCAN_REQ_SSID_SPECIFIED;
				prScanReqMsg->ucSSIDNum = 1;
				prScanReqMsg->prSsid = &(prAisFsmInfo->rRoamingSSID);
			}
#endif

			/* using default channel dwell time/timeout value */
			prScanReqMsg->u2ProbeDelay = 0;
			prScanReqMsg->u2ChannelDwellTime = 0;
			prScanReqMsg->u2TimeoutValue = 0;

			/* check if tethering is running and need to fix on specific channel */
			if (cnmAisInfraChannelFixed(prAdapter, &eBand, &ucChannel) == TRUE) {
				prScanReqMsg->eScanChannel = SCAN_CHANNEL_SPECIFIED;
				prScanReqMsg->ucChannelListNum = 1;
				prScanReqMsg->arChnlInfoList[0].eBand = eBand;
				prScanReqMsg->arChnlInfoList[0].ucChannelNum = ucChannel;
#if CFG_SUPPORT_NCHO
			} else if (prAdapter->rNchoInfo.fgECHOEnabled &&
				prAdapter->rNchoInfo.u4RoamScanControl == TRUE &&
				prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED &&
				prAdapter->rWifiVar.rRoamingInfo.eCurrentState == ROAMING_STATE_DISCOVERY) {
				/* handle NCHO scan channel info */
				uint32_t	u4size = 0;
				struct _CFG_NCHO_SCAN_CHNL_T *prRoamScnChnl = NULL;

				prRoamScnChnl = &prAdapter->rNchoInfo.rRoamScnChnl;
				/* set partial scan */
				prScanReqMsg->ucChannelListNum = prRoamScnChnl->ucChannelListNum;
				u4size = sizeof(prRoamScnChnl->arChnlInfoList);

				DBGLOG(AIS, TRACE,
					"NCHO SCAN channel num = %d, total size=%d\n",
					prScanReqMsg->ucChannelListNum, u4size);

				kalMemCopy(&(prScanReqMsg->arChnlInfoList), &(prRoamScnChnl->arChnlInfoList),
						u4size);

				/* set scan channel type for NCHO scan */
				prScanReqMsg->eScanChannel = SCAN_CHANNEL_SPECIFIED;
#endif
			} else if (prAdapter->aePreferBand[prAdapter->prAisBssInfo->ucBssIndex] == BAND_NULL) {
				if (prAdapter->fgEnable5GBand == TRUE)
					prScanReqMsg->eScanChannel = SCAN_CHANNEL_FULL;
				else
					prScanReqMsg->eScanChannel = SCAN_CHANNEL_2G4;

			} else if (prAdapter->aePreferBand[prAdapter->prAisBssInfo->ucBssIndex] == BAND_2G4) {
				prScanReqMsg->eScanChannel = SCAN_CHANNEL_2G4;
			} else if (prAdapter->aePreferBand[prAdapter->prAisBssInfo->ucBssIndex] == BAND_5G) {
				prScanReqMsg->eScanChannel = SCAN_CHANNEL_5G;
			} else {
				prScanReqMsg->eScanChannel = SCAN_CHANNEL_FULL;
				ASSERT(0);
			}

			switch (prScanReqMsg->eScanChannel) {
			case SCAN_CHANNEL_FULL:
			case SCAN_CHANNEL_2G4:
			case SCAN_CHANNEL_5G:
				scanSetRequestChannel(
					prAdapter,
					prAisFsmInfo->u4ScanChannelNum,
					prAisFsmInfo->arChannel,
					(prAisFsmInfo->eCurrentState ==
					AIS_STATE_ONLINE_SCAN),
					prScanReqMsg);
				break;
			default:
				break;
			}

			if (prAisFsmInfo->u4ScanIELength > 0) {
				kalMemCopy(prScanReqMsg->aucIE, prAisFsmInfo->aucScanIEBuf,
					   prAisFsmInfo->u4ScanIELength);
				kalMemZero(prAisFsmInfo->aucScanIEBuf,
					prAisFsmInfo->u4ScanIELength);
				prAisFsmInfo->u4ScanIELength = 0;
			} else {
#if CFG_SUPPORT_WPS2
				if (prAdapter->prGlueInfo->u2WSCIELen > 0) {
					kalMemCopy(prScanReqMsg->aucIE,
						   &prAdapter->prGlueInfo->aucWSCIE, prAdapter->prGlueInfo->u2WSCIELen);
				}
			}
#endif
			prScanReqMsg->u2IELen = u2ScanIELen;

			mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *) prScanReqMsg, MSG_SEND_METHOD_BUF);

			prAisFsmInfo->fgTryScan = FALSE;	/* Will enable background sleep for infrastructure */
			/* Support AP Selection */
			prAisFsmInfo->ucJoinFailCntAfterScan = 0;

			break;

		case AIS_STATE_REQ_CHANNEL_JOIN:
			/* send message to CNM for acquiring channel */
			prMsgChReq = (struct MSG_CH_REQ *) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(struct MSG_CH_REQ));
			if (!prMsgChReq) {
				ASSERT(0);	/* Can't indicate CNM for channel acquiring */
				return;
			}

			prMsgChReq->rMsgHdr.eMsgId = MID_MNY_CNM_CH_REQ;
			prMsgChReq->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
			prMsgChReq->ucTokenID = ++prAisFsmInfo->ucSeqNumOfChReq;
			prMsgChReq->eReqType = CH_REQ_TYPE_JOIN;
			prMsgChReq->u4MaxInterval = AIS_JOIN_CH_REQUEST_INTERVAL;
			prMsgChReq->ucPrimaryChannel = prAisFsmInfo->prTargetBssDesc->ucChannelNum;
			prMsgChReq->eRfSco = prAisFsmInfo->prTargetBssDesc->eSco;
			prMsgChReq->eRfBand = prAisFsmInfo->prTargetBssDesc->eBand;
#if CFG_SUPPORT_DBDC
			prMsgChReq->eDBDCBand = ENUM_BAND_AUTO;
#endif /*CFG_SUPPORT_DBDC*/
			/* To do: check if 80/160MHz bandwidth is needed here */
			/* Decide RF BW by own OP and Peer OP BW */
			ucRfBw = cnmGetDbdcBwCapability(prAdapter, prAisBssInfo->ucBssIndex);
			ucRfBw = rlmGetVhtOpBwByBssOpBw(ucRfBw); /* Revise to VHT OP BW */
			if (ucRfBw > prAisFsmInfo->prTargetBssDesc->eChannelWidth)
				ucRfBw = prAisFsmInfo->prTargetBssDesc->eChannelWidth;

			prMsgChReq->eRfChannelWidth = ucRfBw;
			/* TODO: BW80+80 support */
			prMsgChReq->ucRfCenterFreqSeg1 =
				nicGetVhtS1(prMsgChReq->ucPrimaryChannel, prMsgChReq->eRfChannelWidth);
			DBGLOG(RLM, INFO, "AIS req CH for CH:%d, Bw:%d, s1=%d\n",
				prAisBssInfo->ucPrimaryChannel, prMsgChReq->eRfChannelWidth,
				prMsgChReq->ucRfCenterFreqSeg1);
			prMsgChReq->ucRfCenterFreqSeg2 = 0;

			rlmReviseMaxBw(prAdapter, prAisBssInfo->ucBssIndex, &prMsgChReq->eRfSco,
					(enum ENUM_CHANNEL_WIDTH *)&prMsgChReq->eRfChannelWidth,
				&prMsgChReq->ucRfCenterFreqSeg1, &prMsgChReq->ucPrimaryChannel);

			mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *) prMsgChReq, MSG_SEND_METHOD_BUF);

			prAisFsmInfo->fgIsChannelRequested = TRUE;
			break;

		case AIS_STATE_JOIN:
			aisFsmStateInit_JOIN(prAdapter, prAisFsmInfo->prTargetBssDesc);
			break;

		case AIS_STATE_JOIN_FAILURE:
			prAdapter->rWifiVar.rConnSettings.eReConnectLevel = RECONNECT_LEVEL_MIN;
			prConnSettings->fgIsDisconnectedByNonRequest = TRUE;

			nicMediaJoinFailure(prAdapter, prAdapter->prAisBssInfo->ucBssIndex,
						WLAN_STATUS_JOIN_FAILURE);

			eNextState = AIS_STATE_IDLE;
			fgIsTransition = TRUE;

			break;

#if CFG_SUPPORT_ADHOC
		case AIS_STATE_IBSS_ALONE:
			aisFsmStateInit_IBSS_ALONE(prAdapter);
			break;

		case AIS_STATE_IBSS_MERGE:
			aisFsmStateInit_IBSS_MERGE(prAdapter, prAisFsmInfo->prTargetBssDesc);
			break;
#endif /* CFG_SUPPORT_ADHOC */

		case AIS_STATE_NORMAL_TR:
			if (prAisFsmInfo->fgIsInfraChannelFinished == FALSE) {
				/* Don't do anything when rJoinTimeoutTimer is still ticking */
			} else {
				/* 1. Process for pending scan */
				if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN, TRUE) == TRUE) {
					wlanClearScanningResult(prAdapter);
					eNextState = AIS_STATE_ONLINE_SCAN;
					fgIsTransition = TRUE;
				}
				/* 2. Process for pending roaming scan */
				else if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_SEARCH, TRUE) == TRUE) {
					eNextState = AIS_STATE_LOOKING_FOR;
					fgIsTransition = TRUE;
				}
				/* 3. Process for pending roaming scan */
				else if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_CONNECT, TRUE) == TRUE) {
					eNextState = AIS_STATE_SEARCH;
					fgIsTransition = TRUE;
				} else
				    if (aisFsmIsRequestPending
					(prAdapter, AIS_REQUEST_REMAIN_ON_CHANNEL, TRUE) == TRUE) {
					eNextState = AIS_STATE_REQ_REMAIN_ON_CHANNEL;
					fgIsTransition = TRUE;
				}
			}

			break;

		case AIS_STATE_DISCONNECTING:
			/* send for deauth frame for disconnection */
			authSendDeauthFrame(prAdapter,
					    prAisBssInfo,
					    prAisBssInfo->prStaRecOfAP,
					    (struct SW_RFB *) NULL, REASON_CODE_DEAUTH_LEAVING_BSS, aisDeauthXmitComplete);
			cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rDeauthDoneTimer, 100);
			break;

		case AIS_STATE_REQ_REMAIN_ON_CHANNEL:
			/* send message to CNM for acquiring channel */
			prMsgChReq = (struct MSG_CH_REQ *) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(struct MSG_CH_REQ));
			if (!prMsgChReq) {
				ASSERT(0);	/* Can't indicate CNM for channel acquiring */
				return;
			}

			/* release channel */
			aisFsmReleaseCh(prAdapter);

			/* zero-ize */
			kalMemZero(prMsgChReq, sizeof(struct MSG_CH_REQ));

			/* filling */
			prMsgChReq->rMsgHdr.eMsgId = MID_MNY_CNM_CH_REQ;
			prMsgChReq->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
			prMsgChReq->ucTokenID = ++prAisFsmInfo->ucSeqNumOfChReq;
			prMsgChReq->eReqType = CH_REQ_TYPE_JOIN;
			prMsgChReq->u4MaxInterval = prAisFsmInfo->rChReqInfo.u4DurationMs;
			prMsgChReq->ucPrimaryChannel = prAisFsmInfo->rChReqInfo.ucChannelNum;
			prMsgChReq->eRfSco = prAisFsmInfo->rChReqInfo.eSco;
			prMsgChReq->eRfBand = prAisFsmInfo->rChReqInfo.eBand;
#if CFG_SUPPORT_DBDC
			prMsgChReq->eDBDCBand = ENUM_BAND_AUTO;
#endif
			mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *) prMsgChReq, MSG_SEND_METHOD_BUF);

			prAisFsmInfo->fgIsChannelRequested = TRUE;

			break;

		case AIS_STATE_REMAIN_ON_CHANNEL:
			if (!IS_NET_ACTIVE(prAdapter,
				prAdapter->prAisBssInfo->ucBssIndex)) {
			SET_NET_ACTIVE(prAdapter,
				prAdapter->prAisBssInfo->ucBssIndex);

				/* sync with firmware */
				nicActivateNetwork(prAdapter,
					prAdapter->prAisBssInfo->ucBssIndex);
			}
#if CFG_SUPPORT_PNO
			prAisBssInfo->fgIsNetRequestInActive = FALSE;
#endif
			break;

		default:
			ASSERT(0);	/* Make sure we have handle all STATEs */
			break;

		}
	} while (fgIsTransition);

	return;

}				/* end of aisFsmSteps() */

enum ENUM_AIS_STATE aisFsmStateSearchAction(IN struct ADAPTER *prAdapter, uint8_t ucPhase)
{
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_DESC *prBssDesc;
	enum ENUM_AIS_STATE eState = AIS_STATE_IDLE;

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

#if CFG_SLT_SUPPORT
	prBssDesc = prAdapter->rWifiVar.rSltInfo.prPseudoBssDesc;
#else
	prBssDesc = scanSearchBssDescByPolicy(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
#endif

	if (ucPhase == AIS_FSM_STATE_SEARCH_ACTION_PHASE_0) {
		if (prConnSettings->eOPMode == NET_TYPE_INFRA) {

			/* issue reconnect request, */
			/*and retreat to idle state for scheduling */
			aisFsmInsertRequest(prAdapter, AIS_REQUEST_RECONNECT);
			eState = AIS_STATE_IDLE;
		}
#if CFG_SUPPORT_ADHOC
		else if ((prConnSettings->eOPMode == NET_TYPE_IBSS)
			 || (prConnSettings->eOPMode == NET_TYPE_AUTO_SWITCH)
			 || (prConnSettings->eOPMode == NET_TYPE_DEDICATED_IBSS)) {
			prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;
			prAisFsmInfo->prTargetBssDesc = NULL;
			eState = AIS_STATE_IBSS_ALONE;
		}
#endif /* CFG_SUPPORT_ADHOC */
		else {
			ASSERT(0);
			eState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
		}
	} else if (ucPhase == AIS_FSM_STATE_SEARCH_ACTION_PHASE_1) {
		/* increase connection trial count for infrastructure connection */
		if (prConnSettings->eOPMode == NET_TYPE_INFRA)
			prAisFsmInfo->ucConnTrialCount++;
		/* 4 <A> Try to SCAN */
		if (prAisFsmInfo->fgTryScan)
			eState = AIS_STATE_LOOKING_FOR;

		/* 4 <B> We've do SCAN already, now wait in some STATE. */
		else {
			if (prConnSettings->eOPMode == NET_TYPE_INFRA) {

				/* issue reconnect request, and */
				/* retreat to idle state for scheduling */
				aisFsmInsertRequest(prAdapter, AIS_REQUEST_RECONNECT);

				eState = AIS_STATE_IDLE;
			}
#if CFG_SUPPORT_ADHOC
			else if ((prConnSettings->eOPMode == NET_TYPE_IBSS)
				 || (prConnSettings->eOPMode == NET_TYPE_AUTO_SWITCH)
				 || (prConnSettings->eOPMode == NET_TYPE_DEDICATED_IBSS)) {

				prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;
				prAisFsmInfo->prTargetBssDesc = NULL;

				eState = AIS_STATE_IBSS_ALONE;
			}
#endif /* CFG_SUPPORT_ADHOC */
			else {
				ASSERT(0);
				eState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
			}
		}
	} else {
#if DBG
		if (prAisBssInfo->ucReasonOfDisconnect != DISCONNECT_REASON_CODE_REASSOCIATION)
			ASSERT(UNEQUAL_MAC_ADDR(prBssDesc->aucBSSID, prAisBssInfo->aucBSSID));
#endif /* DBG */
	}
	return eState;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
void aisFsmRunEventScanDone(IN struct ADAPTER *prAdapter, IN struct MSG_HDR *prMsgHdr)
{
	struct MSG_SCN_SCAN_DONE *prScanDoneMsg;
	struct AIS_FSM_INFO *prAisFsmInfo;
	enum ENUM_AIS_STATE eNextState;
	uint8_t ucSeqNumOfCompMsg;
	struct CONNECTION_SETTINGS *prConnSettings;
	enum ENUM_SCAN_STATUS eStatus = SCAN_STATUS_DONE;

	DEBUGFUNC("aisFsmRunEventScanDone()");

	ASSERT(prAdapter);
	ASSERT(prMsgHdr);

	DBGLOG(AIS, LOUD, "EVENT-SCAN DONE: Current Time = %u\n", kalGetTimeTick());

	if (prAdapter->prAisBssInfo == NULL) {
		/* This case occurs when the AIS isn't done, but the wlan0 */
		/* has changed to AP mode. And the prAisBssInfo is freed.  */
		DBGLOG(AIS, WARN, "prAisBssInfo is NULL, and then return\n");
		return;
	}

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	prScanDoneMsg = (struct MSG_SCN_SCAN_DONE *) prMsgHdr;
	ASSERT(prScanDoneMsg->ucBssIndex == prAdapter->prAisBssInfo->ucBssIndex);

	ucSeqNumOfCompMsg = prScanDoneMsg->ucSeqNum;
	eStatus = prScanDoneMsg->eScanStatus;
	cnmMemFree(prAdapter, prMsgHdr);

	DBGLOG(AIS, INFO, "ScanDone %u, status(%d) native req(%u)\n",
		ucSeqNumOfCompMsg, eStatus, prAisFsmInfo->u2SeqNumOfScanReport);

	eNextState = prAisFsmInfo->eCurrentState;

	if ((uint16_t)ucSeqNumOfCompMsg == prAisFsmInfo->u2SeqNumOfScanReport) {
		prAisFsmInfo->u2SeqNumOfScanReport = AIS_SCN_REPORT_SEQ_NOT_SET;
		prConnSettings->fgIsScanReqIssued = FALSE;
		kalScanDone(prAdapter->prGlueInfo, KAL_NETWORK_TYPE_AIS_INDEX,
			(eStatus == SCAN_STATUS_DONE) ?
			WLAN_STATUS_SUCCESS : WLAN_STATUS_FAILURE);
	}
	if (ucSeqNumOfCompMsg != prAisFsmInfo->ucSeqNumOfScanReq) {
		DBGLOG(AIS, WARN,
			"SEQ NO of AIS SCN DONE MSG is not matched %u %u\n",
			ucSeqNumOfCompMsg, prAisFsmInfo->ucSeqNumOfScanReq);
	} else {
		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rScanDoneTimer);
		switch (prAisFsmInfo->eCurrentState) {
		case AIS_STATE_SCAN:
			eNextState = AIS_STATE_IDLE;
#if CFG_SUPPORT_AGPS_ASSIST
			scanReportScanResultToAgps(prAdapter);
#endif
			break;

		case AIS_STATE_ONLINE_SCAN:
#if CFG_SUPPORT_ROAMING
			eNextState = aisFsmRoamingScanResultsUpdate(prAdapter);
#else
			eNextState = AIS_STATE_NORMAL_TR;
#endif /* CFG_SUPPORT_ROAMING */
#if CFG_SUPPORT_AGPS_ASSIST
			scanReportScanResultToAgps(prAdapter);
#endif
/* Support AP Selection */
#if CFG_SELECT_BSS_BASE_ON_MULTI_PARAM
			scanGetCurrentEssChnlList(prAdapter);
#endif
/* end Support AP Selection */
			break;

		case AIS_STATE_LOOKING_FOR:
#if CFG_SUPPORT_ROAMING
			eNextState = aisFsmRoamingScanResultsUpdate(prAdapter);
#else
			eNextState = AIS_STATE_SEARCH;
#endif /* CFG_SUPPORT_ROAMING */
/* Support AP Selection */
#if CFG_SELECT_BSS_BASE_ON_MULTI_PARAM
			scanGetCurrentEssChnlList(prAdapter);
#endif
/* ebd Support AP Selection */
			break;

		default:
			break;

		}
	}
	if (eNextState != prAisFsmInfo->eCurrentState)
		aisFsmSteps(prAdapter, eNextState);
}				/* end of aisFsmRunEventScanDone() */

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
void aisFsmRunEventAbort(IN struct ADAPTER *prAdapter, IN struct MSG_HDR *prMsgHdr)
{
	struct MSG_AIS_ABORT *prAisAbortMsg;
	struct AIS_FSM_INFO *prAisFsmInfo;
	uint8_t ucReasonOfDisconnect;
	u_int8_t fgDelayIndication;
	struct CONNECTION_SETTINGS *prConnSettings;

	DEBUGFUNC("aisFsmRunEventAbort()");

	ASSERT(prAdapter);
	ASSERT(prMsgHdr);
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	/* 4 <1> Extract information of Abort Message and then free memory. */
	prAisAbortMsg = (struct MSG_AIS_ABORT *) prMsgHdr;
	ucReasonOfDisconnect = prAisAbortMsg->ucReasonOfDisconnect;
	fgDelayIndication = prAisAbortMsg->fgDelayIndication;

	cnmMemFree(prAdapter, prMsgHdr);

	DBGLOG(AIS, STATE, "EVENT-ABORT: Current State %s, ucReasonOfDisconnect:%d\n",
		apucDebugAisState[prAisFsmInfo->eCurrentState], ucReasonOfDisconnect);

	/* record join request time */
	GET_CURRENT_SYSTIME(&(prAisFsmInfo->rJoinReqTime));

	/* 4 <2> clear previous pending connection request and insert new one */
	/* Support AP Selection */
	if (ucReasonOfDisconnect == DISCONNECT_REASON_CODE_DEAUTHENTICATED ||
		ucReasonOfDisconnect == DISCONNECT_REASON_CODE_DISASSOCIATED) {
		struct STA_RECORD *prSta = prAisFsmInfo->prTargetStaRec;
		struct BSS_DESC *prBss = prAisFsmInfo->prTargetBssDesc;

		if (prSta && prBss && prSta->u2ReasonCode ==
			REASON_CODE_DISASSOC_AP_OVERLOAD) {
			struct AIS_BLACKLIST_ITEM *prBlackList =
				aisAddBlacklist(prAdapter, prBss);

			if (prBlackList)
				prBlackList->u2DeauthReason = prSta->u2ReasonCode;
		}
		if (prAisFsmInfo->prTargetBssDesc)
			prAisFsmInfo->prTargetBssDesc->fgDeauthLastTime = TRUE;
		prConnSettings->fgIsDisconnectedByNonRequest = TRUE;
	/* end Support AP Selection */
	} else {
		prConnSettings->fgIsDisconnectedByNonRequest = FALSE;
	}

	/* to support user space triggered roaming */
	if (ucReasonOfDisconnect == DISCONNECT_REASON_CODE_ROAMING &&
	    prAisFsmInfo->eCurrentState != AIS_STATE_DISCONNECTING) {

		if (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR) {
			/* 1. release channel */
			aisFsmReleaseCh(prAdapter);
			/* 2.1 stop join timeout timer */
			cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);
			/* 2.2 reset local variable */
			prAisFsmInfo->fgIsInfraChannelFinished = TRUE;
			aisFsmSteps(prAdapter, AIS_STATE_SEARCH);
		} else {
			aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_SEARCH, TRUE);
			aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_CONNECT, TRUE);
			aisFsmInsertRequest(prAdapter, AIS_REQUEST_ROAMING_CONNECT);
		}
		return;
	}
	/* Support AP Selection */
#if CFG_SELECT_BSS_BASE_ON_MULTI_PARAM
	scanGetCurrentEssChnlList(prAdapter);
#endif
	/* end Support AP Selection */

	aisFsmIsRequestPending(prAdapter, AIS_REQUEST_RECONNECT, TRUE);
	aisFsmInsertRequest(prAdapter, AIS_REQUEST_RECONNECT);

	if (prAisFsmInfo->eCurrentState != AIS_STATE_DISCONNECTING) {
		if (ucReasonOfDisconnect != DISCONNECT_REASON_CODE_REASSOCIATION) {
			/* 4 <3> invoke abort handler */
			aisFsmStateAbort(prAdapter, ucReasonOfDisconnect, fgDelayIndication);
		} else {
			/* 1. release channel */
			aisFsmReleaseCh(prAdapter);
			/* 2.1 stop join timeout timer */
			cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);
			/* 2.2 reset local variable */
			prAisFsmInfo->fgIsInfraChannelFinished = TRUE;

			prAdapter->prAisBssInfo->ucReasonOfDisconnect = ucReasonOfDisconnect;
			aisFsmSteps(prAdapter, AIS_STATE_IDLE);
		}
	}
}				/* end of aisFsmRunEventAbort() */

/*----------------------------------------------------------------------------*/
/*!
* \brief        This function handles AIS-FSM abort event/command
*
* \param[in] prAdapter              Pointer of ADAPTER_T
*            ucReasonOfDisconnect   Reason for disonnection
*            fgDelayIndication      Option to delay disconnection indication
*
* \return none
*/
/*----------------------------------------------------------------------------*/
void aisFsmStateAbort(IN struct ADAPTER *prAdapter, uint8_t ucReasonOfDisconnect, u_int8_t fgDelayIndication)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	u_int8_t fgIsCheckConnected;

	ASSERT(prAdapter);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;

	/* XXX: The wlan0 may has been changed to AP mode. */
	if (prAisBssInfo == NULL)
		return;

	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	fgIsCheckConnected = FALSE;

	DBGLOG(AIS, STATE, "aisFsmStateAbort DiscReason[%d], CurState[%d]\n",
		ucReasonOfDisconnect, prAisFsmInfo->eCurrentState);

	/* 4 <1> Save information of Abort Message and then free memory. */
	prAisBssInfo->ucReasonOfDisconnect = ucReasonOfDisconnect;

	/* 4 <2> Abort current job. */
	switch (prAisFsmInfo->eCurrentState) {
	case AIS_STATE_IDLE:
	case AIS_STATE_SEARCH:
	case AIS_STATE_JOIN_FAILURE:
		break;

	case AIS_STATE_WAIT_FOR_NEXT_SCAN:
		/* Do cancel timer */
		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rBGScanTimer);

		/* in case roaming is triggered */
		fgIsCheckConnected = TRUE;
		break;

	case AIS_STATE_SCAN:
		/* Do abort SCAN */
		aisFsmStateAbort_SCAN(prAdapter);

		/* queue for later handling */
		if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN, FALSE) == FALSE)
			aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);

		break;

	case AIS_STATE_LOOKING_FOR:
		/* Do abort SCAN */
		aisFsmStateAbort_SCAN(prAdapter);

		/* in case roaming is triggered */
		fgIsCheckConnected = TRUE;
		break;

	case AIS_STATE_REQ_CHANNEL_JOIN:
		/* Release channel to CNM */
		aisFsmReleaseCh(prAdapter);

		/* in case roaming is triggered */
		fgIsCheckConnected = TRUE;

		/* stop okc timeout timer */
		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rWaitOkcPMKTimer);
		break;

	case AIS_STATE_JOIN:
		/* Do abort JOIN */
		aisFsmStateAbort_JOIN(prAdapter);

		/* in case roaming is triggered */
		fgIsCheckConnected = TRUE;
		break;

#if CFG_SUPPORT_ADHOC
	case AIS_STATE_IBSS_ALONE:
	case AIS_STATE_IBSS_MERGE:
		aisFsmStateAbort_IBSS(prAdapter);
		break;
#endif /* CFG_SUPPORT_ADHOC */

	case AIS_STATE_ONLINE_SCAN:
		/* Do abort SCAN */
		aisFsmStateAbort_SCAN(prAdapter);
		/* queue for later handling */
		if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN, FALSE) == FALSE)
			aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);

		fgIsCheckConnected = TRUE;
		break;

	case AIS_STATE_NORMAL_TR:
		fgIsCheckConnected = TRUE;
		break;

	case AIS_STATE_DISCONNECTING:
		/* Do abort NORMAL_TR */
		aisFsmStateAbort_NORMAL_TR(prAdapter);

		break;

	case AIS_STATE_REQ_REMAIN_ON_CHANNEL:
		/* release channel */
		aisFsmReleaseCh(prAdapter);
		break;

	case AIS_STATE_REMAIN_ON_CHANNEL:
		/* 1. release channel */
		aisFsmReleaseCh(prAdapter);

		/* 2. stop channel timeout timer */
		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rChannelTimeoutTimer);

		break;

	default:
		break;
	}

	if (fgIsCheckConnected && (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED)) {

		/* switch into DISCONNECTING state for sending DEAUTH if necessary */
		if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE &&
		    prAisBssInfo->ucReasonOfDisconnect == DISCONNECT_REASON_CODE_NEW_CONNECTION &&
		    prAisBssInfo->prStaRecOfAP && prAisBssInfo->prStaRecOfAP->fgIsInUse) {
			aisFsmSteps(prAdapter, AIS_STATE_DISCONNECTING);

			return;
		}
		/* Do abort NORMAL_TR */
		aisFsmStateAbort_NORMAL_TR(prAdapter);
	}

	aisFsmDisconnect(prAdapter, fgDelayIndication);

	return;

}				/* end of aisFsmStateAbort() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will handle the Join Complete Event from SAA FSM for AIS FSM
*
* @param[in] prMsgHdr   Message of Join Complete of SAA FSM.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmRunEventJoinComplete(IN struct ADAPTER *prAdapter, IN struct MSG_HDR *prMsgHdr)
{
	struct MSG_SAA_FSM_COMP *prJoinCompMsg;
	struct AIS_FSM_INFO *prAisFsmInfo;
	enum ENUM_AIS_STATE eNextState;
	struct SW_RFB *prAssocRspSwRfb;

	DEBUGFUNC("aisFsmRunEventJoinComplete()");
	ASSERT(prMsgHdr);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prJoinCompMsg = (struct MSG_SAA_FSM_COMP *)prMsgHdr;
	prAssocRspSwRfb = prJoinCompMsg->prSwRfb;

	eNextState = prAisFsmInfo->eCurrentState;

	/* Check State and SEQ NUM */
	if (prAisFsmInfo->eCurrentState == AIS_STATE_JOIN) {
		/* Check SEQ NUM */
		if (prJoinCompMsg->ucSeqNum == prAisFsmInfo->ucSeqNumOfReqMsg)
			eNextState = aisFsmJoinCompleteAction(prAdapter, prMsgHdr);
#if DBG
		else
			DBGLOG(AIS, WARN, "SEQ NO of AIS JOIN COMP MSG is not matched\n");
#endif /* DBG */
	}
	/* Support AP Selection*/
	/* try to remove timeout blacklist item */
		aisRemoveDisappearedBlacklist(prAdapter);
	/* end Support AP Selection*/
	if (eNextState != prAisFsmInfo->eCurrentState)
		aisFsmSteps(prAdapter, eNextState);

	if (prAssocRspSwRfb)
		nicRxReturnRFB(prAdapter, prAssocRspSwRfb);

	cnmMemFree(prAdapter, prMsgHdr);
}				/* end of aisFsmRunEventJoinComplete() */

enum ENUM_AIS_STATE aisFsmJoinCompleteAction(IN struct ADAPTER *prAdapter, IN struct MSG_HDR *prMsgHdr)
{
	struct MSG_SAA_FSM_COMP *prJoinCompMsg;
	struct AIS_FSM_INFO *prAisFsmInfo;
	enum ENUM_AIS_STATE eNextState;
	struct STA_RECORD *prStaRec;
	struct SW_RFB *prAssocRspSwRfb;
	struct BSS_INFO *prAisBssInfo;
	OS_SYSTIME rCurrentTime;
	struct CONNECTION_SETTINGS *prConnSettings;

	DEBUGFUNC("aisFsmJoinCompleteAction()");

	ASSERT(prMsgHdr);

	GET_CURRENT_SYSTIME(&rCurrentTime);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prJoinCompMsg = (struct MSG_SAA_FSM_COMP *)prMsgHdr;
	prStaRec = prJoinCompMsg->prStaRec;
	prAssocRspSwRfb = prJoinCompMsg->prSwRfb;
	prAisBssInfo = prAdapter->prAisBssInfo;
	eNextState = prAisFsmInfo->eCurrentState;
	prConnSettings = &prAdapter->rWifiVar.rConnSettings;

	do {
		/* 4 <1> JOIN was successful */
		if (prJoinCompMsg->rJoinStatus == WLAN_STATUS_SUCCESS) {
			prAdapter->rWifiVar.rConnSettings.fgSecModeChangeStartTimer = FALSE;

			/* 1. Reset retry count */
			prAisFsmInfo->ucConnTrialCount = 0;
			prAdapter->rWifiVar.rConnSettings.eReConnectLevel = RECONNECT_LEVEL_MIN;

			/* Completion of roaming */
			if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {

#if CFG_SUPPORT_ROAMING
				/* 2. Deactivate previous BSS */
				aisFsmRoamingDisconnectPrevAP(prAdapter, prStaRec);

				/* 3. Update bss based on roaming staRec */
				aisUpdateBssInfoForRoamingAP(prAdapter, prStaRec, prAssocRspSwRfb);
#endif /* CFG_SUPPORT_ROAMING */
			} else {
				kalResetStats(prAdapter->prGlueInfo->prDevHandler);

				/* 4 <1.1> Change FW's Media State immediately. */
				aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_CONNECTED);

				/* 4 <1.2> Deactivate previous AP's STA_RECORD_T in Driver if have. */
				if ((prAisBssInfo->prStaRecOfAP) &&
				    (prAisBssInfo->prStaRecOfAP != prStaRec) &&
				    (prAisBssInfo->prStaRecOfAP->fgIsInUse)) {

					cnmStaRecChangeState(prAdapter, prAisBssInfo->prStaRecOfAP, STA_STATE_1);
					cnmStaRecFree(prAdapter, prAisBssInfo->prStaRecOfAP);
				}

				/* For temp solution, need to refine */
				/* 4 <1.4> Update BSS_INFO_T */
				aisUpdateBssInfoForJOIN(prAdapter, prStaRec, prAssocRspSwRfb);

				/* 4 <1.3> Activate current AP's STA_RECORD_T in Driver. */
				cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);

				/* 4 <1.5> Update RSSI if necessary */
				nicUpdateRSSI(prAdapter,
					      prAdapter->prAisBssInfo->ucBssIndex,
					      (int8_t) (RCPI_TO_dBm(prStaRec->ucRCPI)), 0);

				/* 4 <1.6> Indicate Connected Event to Host immediately. */
				/* Require BSSID, Association ID, Beacon Interval */
				/* .. from AIS_BSS_INFO_T */
				aisIndicationOfMediaStateToHost(prAdapter, PARAM_MEDIA_STATE_CONNECTED, FALSE);

				if (prAdapter->rWifiVar.ucTpTestMode == ENUM_TP_TEST_MODE_THROUGHPUT)
					nicEnterTPTestMode(prAdapter, TEST_MODE_THROUGHPUT);
				else if (prAdapter->rWifiVar.ucTpTestMode == ENUM_TP_TEST_MODE_SIGMA_AC_N_PMF)
					nicEnterTPTestMode(prAdapter, TEST_MODE_SIGMA_AC_N_PMF);
				else if (prAdapter->rWifiVar.ucTpTestMode == ENUM_TP_TEST_MODE_SIGMA_WMM_PS)
					nicEnterTPTestMode(prAdapter, TEST_MODE_SIGMA_WMM_PS);
			}

#if CFG_SUPPORT_ROAMING
			/* if user space roaming is enabled, we should disable driver/fw roaming */
			if ((prAdapter->rWifiVar.rConnSettings.eConnectionPolicy != CONNECT_BY_BSSID)
				&& prAdapter->rWifiVar.rRoamingInfo.fgDrvRoamingAllow)
				roamingFsmRunEventStart(prAdapter);
#endif /* CFG_SUPPORT_ROAMING */
			if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_CONNECT, FALSE) == FALSE)
				prAisFsmInfo->rJoinReqTime = 0;

			/* Support AP Selection */
			prAisFsmInfo->prTargetBssDesc->fgDeauthLastTime = FALSE;
			prAisFsmInfo->ucJoinFailCntAfterScan = 0;
			/* end Support AP Selection */

			/* 4 <1.7> Set the Next State of AIS FSM */
			eNextState = AIS_STATE_NORMAL_TR;
		}
		/* 4 <2> JOIN was not successful */
		else {
			/* 4 <2.1> Redo JOIN process with other Auth Type if possible */
			if (aisFsmStateInit_RetryJOIN(prAdapter, prStaRec) == FALSE) {
				struct BSS_DESC *prBssDesc;
				struct PARAM_SSID rSsid;
				struct CONNECTION_SETTINGS *prConnSettings;

				prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
				prBssDesc = prAisFsmInfo->prTargetBssDesc;

				/* 1. Increase Failure Count */
				prStaRec->ucJoinFailureCount++;

				/* 2. release channel */
				aisFsmReleaseCh(prAdapter);

				/* 3.1 stop join timeout timer */
				cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rJoinTimeoutTimer);

				/* 3.2 reset local variable */
				prAisFsmInfo->fgIsInfraChannelFinished = TRUE;
				/* Support AP Selection */
				prAisFsmInfo->ucJoinFailCntAfterScan++;

				kalMemZero(&rSsid, sizeof(struct PARAM_SSID));
				if (prBssDesc)
					COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen,
						prBssDesc->aucSSID, prBssDesc->ucSSIDLen);
				else
					COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen,
						prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

				prBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter,
								prStaRec->aucMacAddr, TRUE, &rSsid);

				if (prBssDesc == NULL)
					break;

				DBGLOG(AIS, TRACE,
				"ucJoinFailureCount=%d %d, Status=%d Reason=%d, eConnectionState=%d",
					prStaRec->ucJoinFailureCount, prBssDesc->ucJoinFailureCount,
					prStaRec->u2StatusCode, prStaRec->u2ReasonCode,
					prAisBssInfo->eConnectionState);

				/* ASSERT(prBssDesc); */
				/* ASSERT(prBssDesc->fgIsConnecting); */
				prBssDesc->u2JoinStatus = prStaRec->u2StatusCode;
				prBssDesc->ucJoinFailureCount++;
				if (prBssDesc->ucJoinFailureCount >= SCN_BSS_JOIN_FAIL_THRESOLD) {
					/* Support AP Selection */
					aisAddBlacklist(prAdapter, prBssDesc);

					GET_CURRENT_SYSTIME(&prBssDesc->rJoinFailTime);
					DBGLOG(AIS, INFO,
					       "Bss " MACSTR " join fail %d times, temp disable it at time: %u\n",
					       MAC2STR(prBssDesc->aucBSSID), SCN_BSS_JOIN_FAIL_THRESOLD,
					       prBssDesc->rJoinFailTime);
				}

				/* Support AP Selection */
				if (prBssDesc->prBlack)
					prBssDesc->prBlack->u2AuthStatus = prStaRec->u2StatusCode;

				if (prBssDesc)
					prBssDesc->fgIsConnecting = FALSE;

				/* 3.3 Free STA-REC */
				if (prStaRec != prAisBssInfo->prStaRecOfAP)
					cnmStaRecFree(prAdapter, prStaRec);

				if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
					/* roaming fail count and time */
					prAdapter->prGlueInfo->u4RoamFailCnt++;
					prAdapter->prGlueInfo->u8RoamFailTime = sched_clock();
#if CFG_SUPPORT_ROAMING
					eNextState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
#endif /* CFG_SUPPORT_ROAMING */
					if (prConnSettings->eConnectionPolicy == CONNECT_BY_BSSID
						&& prBssDesc->u2JoinStatus)
						eNextState = AIS_STATE_JOIN_FAILURE;
				} else if (prAisFsmInfo->rJoinReqTime != 0 && CHECK_FOR_TIMEOUT
					(rCurrentTime, prAisFsmInfo->rJoinReqTime, SEC_TO_SYSTIME(AIS_JOIN_TIMEOUT))) {
					/* 4.a temrminate join operation */
					eNextState = AIS_STATE_JOIN_FAILURE;
				} else if (prAisFsmInfo->rJoinReqTime != 0
					&& prBssDesc->ucJoinFailureCount >= SCN_BSS_JOIN_FAIL_THRESOLD
					&& prBssDesc->u2JoinStatus) {
					/* AP reject STA for STATUS_CODE_ASSOC_DENIED_AP_OVERLOAD, or AP block STA */
					eNextState = AIS_STATE_JOIN_FAILURE;
				} else {
					/* 4.b send reconnect request */
					aisFsmInsertRequest(prAdapter, AIS_REQUEST_RECONNECT);

					eNextState = AIS_STATE_IDLE;
				}
			}
		}
	} while (0);
	return eNextState;
}

#if CFG_SUPPORT_ADHOC
/*----------------------------------------------------------------------------*/
/*!
* @brief This function will handle the Grant Msg of IBSS Create which was sent by
*        CNM to indicate that channel was changed for creating IBSS.
*
* @param[in] prAdapter  Pointer of ADAPTER_T
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmCreateIBSS(IN struct ADAPTER *prAdapter)
{
	struct AIS_FSM_INFO *prAisFsmInfo;

	ASSERT(prAdapter);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	do {
		/* Check State */
		if (prAisFsmInfo->eCurrentState == AIS_STATE_IBSS_ALONE)
			aisUpdateBssInfoForCreateIBSS(prAdapter);

	} while (FALSE);
}				/* end of aisFsmCreateIBSS() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will handle the Grant Msg of IBSS Merge which was sent by
*        CNM to indicate that channel was changed for merging IBSS.
*
* @param[in] prAdapter  Pointer of ADAPTER_T
* @param[in] prStaRec   Pointer of STA_RECORD_T for merge
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmMergeIBSS(IN struct ADAPTER *prAdapter, IN struct STA_RECORD *prStaRec)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	enum ENUM_AIS_STATE eNextState;
	struct BSS_INFO *prAisBssInfo;

	ASSERT(prAdapter);
	ASSERT(prStaRec);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;

	do {

		eNextState = prAisFsmInfo->eCurrentState;

		switch (prAisFsmInfo->eCurrentState) {
		case AIS_STATE_IBSS_MERGE:
			{
				struct BSS_DESC *prBssDesc;

				/* 4 <1.1> Change FW's Media State immediately. */
				aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_CONNECTED);

				/* 4 <1.2> Deactivate previous Peers' STA_RECORD_T in Driver if have. */
				bssInitializeClientList(prAdapter, prAisBssInfo);

				/* 4 <1.3> Unmark connection flag of previous BSS_DESC_T. */
				prBssDesc = scanSearchBssDescByBssid(prAdapter, prAisBssInfo->aucBSSID);
				if (prBssDesc != NULL) {
					prBssDesc->fgIsConnecting = FALSE;
					prBssDesc->fgIsConnected = FALSE;
				}
				/* 4 <1.4> Add Peers' STA_RECORD_T to Client List */
				bssAddClient(prAdapter, prAisBssInfo, prStaRec);

				/* 4 <1.5> Activate current Peer's STA_RECORD_T in Driver. */
				cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);
				prStaRec->fgIsMerging = FALSE;

				/* 4 <1.6> Update BSS_INFO_T */
				aisUpdateBssInfoForMergeIBSS(prAdapter, prStaRec);

				/* 4 <1.7> Enable other features */

				/* 4 <1.8> Indicate Connected Event to Host immediately. */
				aisIndicationOfMediaStateToHost(prAdapter, PARAM_MEDIA_STATE_CONNECTED, FALSE);

				/* 4 <1.9> Set the Next State of AIS FSM */
				eNextState = AIS_STATE_NORMAL_TR;

				/* 4 <1.10> Release channel privilege */
				aisFsmReleaseCh(prAdapter);

#if CFG_SLT_SUPPORT
				prAdapter->rWifiVar.rSltInfo.prPseudoStaRec = prStaRec;
#endif
			}
			break;

		default:
			break;
		}

		if (eNextState != prAisFsmInfo->eCurrentState)
			aisFsmSteps(prAdapter, eNextState);

	} while (FALSE);
}				/* end of aisFsmMergeIBSS() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will handle the Notification of existing IBSS was found
*        from SCN.
*
* @param[in] prMsgHdr   Message of Notification of an IBSS was present.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmRunEventFoundIBSSPeer(IN struct ADAPTER *prAdapter, IN struct MSG_HDR *prMsgHdr)
{
	struct MSG_AIS_IBSS_PEER_FOUND *prAisIbssPeerFoundMsg;
	struct AIS_FSM_INFO *prAisFsmInfo;
	enum ENUM_AIS_STATE eNextState;
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prAisBssInfo;
	struct BSS_DESC *prBssDesc;
	u_int8_t fgIsMergeIn;

	ASSERT(prMsgHdr);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;

	prAisIbssPeerFoundMsg = (struct MSG_AIS_IBSS_PEER_FOUND *) prMsgHdr;

	ASSERT(prAisIbssPeerFoundMsg->ucBssIndex == prAdapter->prAisBssInfo->ucBssIndex);

	prStaRec = prAisIbssPeerFoundMsg->prStaRec;
	ASSERT(prStaRec);

	fgIsMergeIn = prAisIbssPeerFoundMsg->fgIsMergeIn;

	cnmMemFree(prAdapter, prMsgHdr);

	eNextState = prAisFsmInfo->eCurrentState;
	switch (prAisFsmInfo->eCurrentState) {
	case AIS_STATE_IBSS_ALONE:
		{
			/* 4 <1> An IBSS Peer 'merged in'. */
			if (fgIsMergeIn) {

				/* 4 <1.1> Change FW's Media State immediately. */
				aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_CONNECTED);

				/* 4 <1.2> Add Peers' STA_RECORD_T to Client List */
				bssAddClient(prAdapter, prAisBssInfo, prStaRec);

#if CFG_SLT_SUPPORT
				/* 4 <1.3> Mark connection flag of BSS_DESC_T. */
				prBssDesc = scanSearchBssDescByTA(prAdapter, prStaRec->aucMacAddr);

				if (prBssDesc != NULL) {
					prBssDesc->fgIsConnecting = FALSE;
					prBssDesc->fgIsConnected = TRUE;
				} else {
					ASSERT(0);	/* Should be able to find a BSS_DESC_T here. */
				}

				/* 4 <1.4> Activate current Peer's STA_RECORD_T in Driver. */
				prStaRec->fgIsQoS = TRUE;	/* TODO(Kevin): TBD */
#else
				/* 4 <1.3> Mark connection flag of BSS_DESC_T. */
				prBssDesc = scanSearchBssDescByBssid(prAdapter, prAisBssInfo->aucBSSID);

				if (prBssDesc != NULL) {
					prBssDesc->fgIsConnecting = FALSE;
					prBssDesc->fgIsConnected = TRUE;
				} else {
					ASSERT(0);	/* Should be able to find a BSS_DESC_T here. */
				}

				/* 4 <1.4> Activate current Peer's STA_RECORD_T in Driver. */
				prStaRec->fgIsQoS = FALSE;	/* TODO(Kevin): TBD */

#endif

				cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);
				prStaRec->fgIsMerging = FALSE;

				/* 4 <1.6> sync. to firmware */
				nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

				/* 4 <1.7> Indicate Connected Event to Host immediately. */
				aisIndicationOfMediaStateToHost(prAdapter, PARAM_MEDIA_STATE_CONNECTED, FALSE);

				/* 4 <1.8> indicate PM for connected */
				nicPmIndicateBssConnected(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

				/* 4 <1.9> Set the Next State of AIS FSM */
				eNextState = AIS_STATE_NORMAL_TR;

				/* 4 <1.10> Release channel privilege */
				aisFsmReleaseCh(prAdapter);
			}
			/* 4 <2> We need 'merge out' to this IBSS */
			else {

				/* 4 <2.1> Get corresponding BSS_DESC_T */
				prBssDesc = scanSearchBssDescByTA(prAdapter, prStaRec->aucMacAddr);

				prAisFsmInfo->prTargetBssDesc = prBssDesc;

				/* 4 <2.2> Set the Next State of AIS FSM */
				eNextState = AIS_STATE_IBSS_MERGE;
			}
		}
		break;

	case AIS_STATE_NORMAL_TR:
		{

			/* 4 <3> An IBSS Peer 'merged in'. */
			if (fgIsMergeIn) {

				/* 4 <3.1> Add Peers' STA_RECORD_T to Client List */
				bssAddClient(prAdapter, prAisBssInfo, prStaRec);

#if CFG_SLT_SUPPORT
				/* 4 <3.2> Activate current Peer's STA_RECORD_T in Driver. */
				prStaRec->fgIsQoS = TRUE;	/* TODO(Kevin): TBD */
#else
				/* 4 <3.2> Activate current Peer's STA_RECORD_T in Driver. */
				prStaRec->fgIsQoS = FALSE;	/* TODO(Kevin): TBD */
#endif

				cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);
				prStaRec->fgIsMerging = FALSE;

			}
			/* 4 <4> We need 'merge out' to this IBSS */
			else {

				/* 4 <4.1> Get corresponding BSS_DESC_T */
				prBssDesc = scanSearchBssDescByTA(prAdapter, prStaRec->aucMacAddr);

				prAisFsmInfo->prTargetBssDesc = prBssDesc;

				/* 4 <4.2> Set the Next State of AIS FSM */
				eNextState = AIS_STATE_IBSS_MERGE;

			}
		}
		break;

	default:
		break;
	}

	if (eNextState != prAisFsmInfo->eCurrentState)
		aisFsmSteps(prAdapter, eNextState);
}				/* end of aisFsmRunEventFoundIBSSPeer() */
#endif /* CFG_SUPPORT_ADHOC */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will indicate the Media State to HOST
*
* @param[in] eConnectionState   Current Media State
* @param[in] fgDelayIndication  Set TRUE for postponing the Disconnect Indication.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void
aisIndicationOfMediaStateToHost(IN struct ADAPTER *prAdapter,
				enum ENUM_PARAM_MEDIA_STATE eConnectionState, u_int8_t fgDelayIndication)
{
	struct EVENT_CONNECTION_STATUS rEventConnStatus;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;

	DEBUGFUNC("aisIndicationOfMediaStateToHost()");

	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	prAisBssInfo = prAdapter->prAisBssInfo;
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	DBGLOG(AIS, LOUD, "AIS indicate Media State to Host Current State [%d]\n", prAisBssInfo->eConnectionState);

	/* NOTE(Kevin): Move following line to aisChangeMediaState() macro per CM's request. */
	/* prAisBssInfo->eConnectionState = eConnectionState; */

	/* For indicating the Disconnect Event only if current media state is
	 * disconnected and we didn't do indication yet.
	 */
	DBGLOG(AIS, INFO, "Current state: %d, connection state indicated: %d\n",
		prAisFsmInfo->eCurrentState, prAisBssInfo->eConnectionStateIndicated);

	if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_DISCONNECTED &&
		/* if receive DEAUTH in JOIN state, report disconnect*/
		!(prAisBssInfo->ucReasonOfDisconnect == DISCONNECT_REASON_CODE_DEAUTHENTICATED &&
		 prAisFsmInfo->eCurrentState == AIS_STATE_JOIN)) {
		if (prAisBssInfo->eConnectionStateIndicated == eConnectionState)
			return;
	}

	if (!fgDelayIndication) {
		/* 4 <0> Cancel Delay Timer */
		prAisFsmInfo->u4PostponeIndStartTime = 0;

		/* 4 <1> Fill EVENT_CONNECTION_STATUS */
		rEventConnStatus.ucMediaStatus = (uint8_t) eConnectionState;

		if (eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
			rEventConnStatus.ucReasonOfDisconnect = DISCONNECT_REASON_CODE_RESERVED;

			if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
				rEventConnStatus.ucInfraMode = (uint8_t) NET_TYPE_INFRA;
				rEventConnStatus.u2AID = prAisBssInfo->u2AssocId;
				rEventConnStatus.u2ATIMWindow = 0;
			} else if (prAisBssInfo->eCurrentOPMode == OP_MODE_IBSS) {
				rEventConnStatus.ucInfraMode = (uint8_t) NET_TYPE_IBSS;
				rEventConnStatus.u2AID = 0;
				rEventConnStatus.u2ATIMWindow = prAisBssInfo->u2ATIMWindow;
			} else {
				ASSERT(0);
			}

			COPY_SSID(rEventConnStatus.aucSsid,
				  rEventConnStatus.ucSsidLen, prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

			COPY_MAC_ADDR(rEventConnStatus.aucBssid, prAisBssInfo->aucBSSID);

			rEventConnStatus.u2BeaconPeriod = prAisBssInfo->u2BeaconInterval;
			rEventConnStatus.u4FreqInKHz = nicChannelNum2Freq(prAisBssInfo->ucPrimaryChannel);

			switch (prAisBssInfo->ucNonHTBasicPhyType) {
			case PHY_TYPE_HR_DSSS_INDEX:
				rEventConnStatus.ucNetworkType = (uint8_t) PARAM_NETWORK_TYPE_DS;
				break;

			case PHY_TYPE_ERP_INDEX:
				rEventConnStatus.ucNetworkType = (uint8_t) PARAM_NETWORK_TYPE_OFDM24;
				break;

			case PHY_TYPE_OFDM_INDEX:
				rEventConnStatus.ucNetworkType = (uint8_t) PARAM_NETWORK_TYPE_OFDM5;
				break;

			default:
				ASSERT(0);
				rEventConnStatus.ucNetworkType = (uint8_t) PARAM_NETWORK_TYPE_DS;
				break;
			}
		} else {
			/* Clear the pmkid cache while media disconnect */
			secClearPmkid(prAdapter);
			rEventConnStatus.ucReasonOfDisconnect = prAisBssInfo->ucReasonOfDisconnect;
		}

		/* 4 <2> Indication */
		nicMediaStateChange(prAdapter, prAdapter->prAisBssInfo->ucBssIndex, &rEventConnStatus);
		prAisBssInfo->eConnectionStateIndicated = eConnectionState;
	} else {
		/* NOTE: Only delay the Indication of Disconnect Event */
		ASSERT(eConnectionState == PARAM_MEDIA_STATE_DISCONNECTED);

		DBGLOG(AIS, INFO, "Postpone the indication of Disconnect for %d seconds\n",
		       prConnSettings->ucDelayTimeOfDisconnectEvent);

		prAisFsmInfo->u4PostponeIndStartTime = kalGetTimeTick();
	}
}				/* end of aisIndicationOfMediaStateToHost() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will indicate an Event of "Media Disconnect" to HOST
*
* @param[in] u4Param  Unused timer parameter
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisPostponedEventOfDisconnTimeout(IN struct ADAPTER *prAdapter,
				IN struct AIS_FSM_INFO *prAisFsmInfo)
{
	struct BSS_INFO *prAisBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	bool fgFound = TRUE;
	bool fgIsPostponeTimeout;
	bool fgIsBeaconTimeout;

	/* firstly, check if we have started postpone indication.
	** otherwise, give a chance to do join before indicate to host
	**/
	if (prAisFsmInfo->u4PostponeIndStartTime == 0)
		return;

	/* if we're in	req channel/join/search state, don't report disconnect. */
	if (prAisFsmInfo->eCurrentState == AIS_STATE_JOIN ||
		prAisFsmInfo->eCurrentState == AIS_STATE_SEARCH ||
		prAisFsmInfo->eCurrentState == AIS_STATE_REQ_CHANNEL_JOIN)
		return;

	prAisBssInfo = prAdapter->prAisBssInfo;
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	DBGLOG(AIS, EVENT, "aisPostponedEventOfDisconnTimeout\n");

	fgIsPostponeTimeout = CHECK_FOR_TIMEOUT(kalGetTimeTick(),
					prAisFsmInfo->u4PostponeIndStartTime,
		SEC_TO_MSEC(prConnSettings->ucDelayTimeOfDisconnectEvent));

	fgIsBeaconTimeout =
		prAisBssInfo->ucReasonOfDisconnect == DISCONNECT_REASON_CODE_RADIO_LOST;

	/* only retry connect once when beacon timeout */
	if (!fgIsPostponeTimeout && !(fgIsBeaconTimeout && prAisFsmInfo->ucConnTrialCount > 1)) {
		DBGLOG(AIS, INFO, "DelayTimeOfDisconnect, don't report disconnect\n");
		return;
	}

	/* 4 <1> Deactivate previous AP's STA_RECORD_T in Driver if have. */
	if (prAisBssInfo->prStaRecOfAP) {
		/* cnmStaRecChangeState(prAdapter, prAisBssInfo->prStaRecOfAP, STA_STATE_1); */

		prAisBssInfo->prStaRecOfAP = (struct STA_RECORD *) NULL;
	}
	/* 4 <2> Remove all connection request */
	while (fgFound)
		fgFound = aisFsmIsRequestPending(prAdapter, AIS_REQUEST_RECONNECT, TRUE);
	if (prAisFsmInfo->eCurrentState == AIS_STATE_LOOKING_FOR)
		prAisFsmInfo->eCurrentState = AIS_STATE_IDLE;
	prConnSettings->fgIsDisconnectedByNonRequest = TRUE;
	prAisBssInfo->u2DeauthReason = REASON_CODE_BEACON_TIMEOUT;
	/* 4 <3> Indicate Disconnected Event to Host immediately. */
	aisIndicationOfMediaStateToHost(prAdapter, PARAM_MEDIA_STATE_DISCONNECTED, FALSE);
}				/* end of aisPostponedEventOfDisconnTimeout() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will update the contain of BSS_INFO_T for AIS network once
*        the association was completed.
*
* @param[in] prStaRec               Pointer to the STA_RECORD_T
* @param[in] prAssocRspSwRfb        Pointer to SW RFB of ASSOC RESP FRAME.
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisUpdateBssInfoForJOIN(IN struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec, struct SW_RFB *prAssocRspSwRfb)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct WLAN_ASSOC_RSP_FRAME *prAssocRspFrame;
	struct BSS_DESC *prBssDesc;
	uint16_t u2IELength;
	uint8_t *pucIE;
	struct PARAM_SSID rSsid;

	DEBUGFUNC("aisUpdateBssInfoForJOIN()");

	ASSERT(prStaRec);
	ASSERT(prAssocRspSwRfb);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);
	prAssocRspFrame = (struct WLAN_ASSOC_RSP_FRAME *) prAssocRspSwRfb->pvHeader;

	DBGLOG(AIS, INFO, "Update AIS_BSS_INFO_T and apply settings to MAC\n");

	/* 3 <1> Update BSS_INFO_T from AIS_FSM_INFO_T or User Settings */
	/* 4 <1.1> Setup Operation Mode */
	prAisBssInfo->eCurrentOPMode = OP_MODE_INFRASTRUCTURE;

	/* 4 <1.2> Setup SSID */
	COPY_SSID(prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen, prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

	/* 4 <1.3> Setup Channel, Band */
	prAisBssInfo->ucPrimaryChannel = prAisFsmInfo->prTargetBssDesc->ucChannelNum;
	prAisBssInfo->eBand = prAisFsmInfo->prTargetBssDesc->eBand;

	/* 3 <2> Update BSS_INFO_T from STA_RECORD_T */
	/* 4 <2.1> Save current AP's STA_RECORD_T and current AID */
	prAisBssInfo->prStaRecOfAP = prStaRec;
	prAisBssInfo->u2AssocId = prStaRec->u2AssocId;

	/* 4 <2.2> Setup Capability */
	prAisBssInfo->u2CapInfo = prStaRec->u2CapInfo;	/* Use AP's Cap Info as BSS Cap Info */

	if (prAisBssInfo->u2CapInfo & CAP_INFO_SHORT_PREAMBLE)
		prAisBssInfo->fgIsShortPreambleAllowed = TRUE;
	else
		prAisBssInfo->fgIsShortPreambleAllowed = FALSE;

#if CFG_SUPPORT_TDLS
	prAisBssInfo->fgTdlsIsProhibited = prStaRec->fgTdlsIsProhibited;
	prAisBssInfo->fgTdlsIsChSwProhibited = prStaRec->fgTdlsIsChSwProhibited;
#endif /* CFG_SUPPORT_TDLS */

	/* 4 <2.3> Setup PHY Attributes and Basic Rate Set/Operational Rate Set */
	prAisBssInfo->ucPhyTypeSet = prStaRec->ucDesiredPhyTypeSet;

	prAisBssInfo->ucNonHTBasicPhyType = prStaRec->ucNonHTBasicPhyType;

	prAisBssInfo->u2OperationalRateSet = prStaRec->u2OperationalRateSet;
	prAisBssInfo->u2BSSBasicRateSet = prStaRec->u2BSSBasicRateSet;

	nicTxUpdateBssDefaultRate(prAisBssInfo);

	/* 3 <3> Update BSS_INFO_T from SW_RFB_T (Association Resp Frame) */
	/* 4 <3.1> Setup BSSID */
	COPY_MAC_ADDR(prAisBssInfo->aucBSSID, prAssocRspFrame->aucBSSID);

	u2IELength = (uint16_t) ((prAssocRspSwRfb->u2PacketLen - prAssocRspSwRfb->u2HeaderLen) -
				(OFFSET_OF(struct WLAN_ASSOC_RSP_FRAME, aucInfoElem[0]) - WLAN_MAC_MGMT_HEADER_LEN));
	pucIE = prAssocRspFrame->aucInfoElem;

	/* 4 <3.2> Parse WMM and setup QBSS flag */
	/* Parse WMM related IEs and configure HW CRs accordingly */
	mqmProcessAssocRsp(prAdapter, prAssocRspSwRfb, pucIE, u2IELength);

	prAisBssInfo->fgIsQBSS = prStaRec->fgIsQoS;

	/* 3 <4> Update BSS_INFO_T from BSS_DESC_T */
	prBssDesc = prAisFsmInfo->prTargetBssDesc;
	if (prBssDesc)
		COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen,
			prBssDesc->aucSSID, prBssDesc->ucSSIDLen);
	else
		COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen,
			prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

	prBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter, prAssocRspFrame->aucBSSID, TRUE, &rSsid);
	if (prBssDesc) {
		prBssDesc->fgIsConnecting = FALSE;
		prBssDesc->fgIsConnected = TRUE;
		prBssDesc->ucJoinFailureCount = 0;

		aisRemoveBlackList(prAdapter, prBssDesc);
		/* 4 <4.1> Setup MIB for current BSS */
		prAisBssInfo->u2BeaconInterval = prBssDesc->u2BeaconInterval;
	} else {
		/* should never happen */
		ASSERT(0);
	}

	/* NOTE: Defer ucDTIMPeriod updating to when beacon is received after connection */
	prAisBssInfo->ucDTIMPeriod = 0;
	prAisBssInfo->u2ATIMWindow = 0;

	prAisBssInfo->ucBeaconTimeoutCount = AIS_BEACON_TIMEOUT_COUNT_INFRA;
#if CFG_SUPPORT_ROAMING_SKIP_ONE_AP
	prAisBssInfo->ucRoamSkipTimes = ROAMING_ONE_AP_SKIP_TIMES;
	prAisBssInfo->fgGoodRcpiArea = FALSE;
	prAisBssInfo->fgPoorRcpiArea = FALSE;
#endif

	/* 4 <4.2> Update HT information and set channel */
	/* Record HT related parameters in rStaRec and rBssInfo
	 * Note: it shall be called before nicUpdateBss()
	 */
	rlmProcessAssocRsp(prAdapter, prAssocRspSwRfb, pucIE, u2IELength);

	secPostUpdateAddr(prAdapter, prAdapter->prAisBssInfo);

	/* 4 <4.3> Sync with firmware for BSS-INFO */
	nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/* 4 <4.4> *DEFER OPERATION* nicPmIndicateBssConnected() will be invoked */
	/* inside scanProcessBeaconAndProbeResp() after 1st beacon is received */
}				/* end of aisUpdateBssInfoForJOIN() */

#if CFG_SUPPORT_ADHOC
/*----------------------------------------------------------------------------*/
/*!
* @brief This function will create an Ad-Hoc network and start sending Beacon Frames.
*
* @param (none)
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisUpdateBssInfoForCreateIBSS(IN struct ADAPTER *prAdapter)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	if (prAisBssInfo->fgIsBeaconActivated)
		return;

	/* 3 <1> Update BSS_INFO_T per Network Basis */
	/* 4 <1.1> Setup Operation Mode */
	prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;

	/* 4 <1.2> Setup SSID */
	COPY_SSID(prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen, prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

	/* 4 <1.3> Clear current AP's STA_RECORD_T and current AID */
	prAisBssInfo->prStaRecOfAP = (struct STA_RECORD *) NULL;
	prAisBssInfo->u2AssocId = 0;

	/* 4 <1.4> Setup Channel, Band and Phy Attributes */
	prAisBssInfo->ucPrimaryChannel = prConnSettings->ucAdHocChannelNum;
	prAisBssInfo->eBand = prConnSettings->eAdHocBand;

	if (prAisBssInfo->eBand == BAND_2G4) {
		/* Depend on eBand */
		prAisBssInfo->ucPhyTypeSet = prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11BGN;
		/* Depend on eCurrentOPMode and ucPhyTypeSet */
		prAisBssInfo->ucConfigAdHocAPMode = AD_HOC_MODE_MIXED_11BG;
	} else {
		/* Depend on eBand */
		prAisBssInfo->ucPhyTypeSet = prAdapter->rWifiVar.ucAvailablePhyTypeSet & PHY_TYPE_SET_802_11ANAC;
		/* Depend on eCurrentOPMode and ucPhyTypeSet */
		prAisBssInfo->ucConfigAdHocAPMode = AD_HOC_MODE_11A;
	}

	/* 4 <1.5> Setup MIB for current BSS */
	prAisBssInfo->u2BeaconInterval = prConnSettings->u2BeaconPeriod;
	prAisBssInfo->ucDTIMPeriod = 0;
	prAisBssInfo->u2ATIMWindow = prConnSettings->u2AtimWindow;

	prAisBssInfo->ucBeaconTimeoutCount = AIS_BEACON_TIMEOUT_COUNT_ADHOC;

	if (prConnSettings->eEncStatus == ENUM_ENCRYPTION1_ENABLED ||
	    prConnSettings->eEncStatus == ENUM_ENCRYPTION2_ENABLED ||
	    prConnSettings->eEncStatus == ENUM_ENCRYPTION3_ENABLED) {
		prAisBssInfo->fgIsProtection = TRUE;
	} else {
		prAisBssInfo->fgIsProtection = FALSE;
	}

	/* 3 <2> Update BSS_INFO_T common part */
	ibssInitForAdHoc(prAdapter, prAisBssInfo);
	/* 4 <2.1> Initialize client list */
	bssInitializeClientList(prAdapter, prAisBssInfo);

	/* 3 <3> Set MAC HW */
	/* 4 <3.1> Setup channel and bandwidth */
	rlmBssInitForAPandIbss(prAdapter, prAisBssInfo);

	/* 4 <3.2> use command packets to inform firmware */
	nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/* 4 <3.3> enable beaconing */
	bssUpdateBeaconContent(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/* 4 <3.4> Update AdHoc PM parameter */
	nicPmIndicateBssCreated(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/* 3 <4> Set ACTIVE flag. */
	prAisBssInfo->fgIsBeaconActivated = TRUE;
	prAisBssInfo->fgHoldSameBssidForIBSS = TRUE;

	/* 3 <5> Start IBSS Alone Timer */
	cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rIbssAloneTimer, SEC_TO_MSEC(AIS_IBSS_ALONE_TIMEOUT_SEC));
}				/* end of aisCreateIBSS() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will update the contain of BSS_INFO_T for AIS network once
*        the existing IBSS was found.
*
* @param[in] prStaRec               Pointer to the STA_RECORD_T
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisUpdateBssInfoForMergeIBSS(IN struct ADAPTER *prAdapter, IN struct STA_RECORD *prStaRec)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_DESC *prBssDesc;
	/* UINT_16 u2IELength; */
	/* PUINT_8 pucIE; */

	ASSERT(prStaRec);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rIbssAloneTimer);

	if (!prAisBssInfo->fgIsBeaconActivated) {

		/* 3 <1> Update BSS_INFO_T per Network Basis */
		/* 4 <1.1> Setup Operation Mode */
		prAisBssInfo->eCurrentOPMode = OP_MODE_IBSS;

		/* 4 <1.2> Setup SSID */
		COPY_SSID(prAisBssInfo->aucSSID,
			  prAisBssInfo->ucSSIDLen, prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

		/* 4 <1.3> Clear current AP's STA_RECORD_T and current AID */
		prAisBssInfo->prStaRecOfAP = (struct STA_RECORD *) NULL;
		prAisBssInfo->u2AssocId = 0;
	}
	/* 3 <2> Update BSS_INFO_T from STA_RECORD_T */
	/* 4 <2.1> Setup Capability */
	prAisBssInfo->u2CapInfo = prStaRec->u2CapInfo;	/* Use Peer's Cap Info as IBSS Cap Info */

	if (prAisBssInfo->u2CapInfo & CAP_INFO_SHORT_PREAMBLE) {
		prAisBssInfo->fgIsShortPreambleAllowed = TRUE;
		prAisBssInfo->fgUseShortPreamble = TRUE;
	} else {
		prAisBssInfo->fgIsShortPreambleAllowed = FALSE;
		prAisBssInfo->fgUseShortPreamble = FALSE;
	}

	/* 7.3.1.4 For IBSS, the Short Slot Time subfield shall be set to 0. */
	prAisBssInfo->fgUseShortSlotTime = FALSE;	/* Set to FALSE for AdHoc */
	prAisBssInfo->u2CapInfo &= ~CAP_INFO_SHORT_SLOT_TIME;

	if (prAisBssInfo->u2CapInfo & CAP_INFO_PRIVACY)
		prAisBssInfo->fgIsProtection = TRUE;
	else
		prAisBssInfo->fgIsProtection = FALSE;

	/* 4 <2.2> Setup PHY Attributes and Basic Rate Set/Operational Rate Set */
	prAisBssInfo->ucPhyTypeSet = prStaRec->ucDesiredPhyTypeSet;

	prAisBssInfo->ucNonHTBasicPhyType = prStaRec->ucNonHTBasicPhyType;

	prAisBssInfo->u2OperationalRateSet = prStaRec->u2OperationalRateSet;
	prAisBssInfo->u2BSSBasicRateSet = prStaRec->u2BSSBasicRateSet;

	rateGetDataRatesFromRateSet(prAisBssInfo->u2OperationalRateSet,
				    prAisBssInfo->u2BSSBasicRateSet,
				    prAisBssInfo->aucAllSupportedRates, &prAisBssInfo->ucAllSupportedRatesLen);

	/* 3 <3> X Update BSS_INFO_T from SW_RFB_T (Association Resp Frame) */

	/* 3 <4> Update BSS_INFO_T from BSS_DESC_T */
	prBssDesc = scanSearchBssDescByTA(prAdapter, prStaRec->aucMacAddr);
	if (prBssDesc) {
		prBssDesc->fgIsConnecting = FALSE;
		prBssDesc->fgIsConnected = TRUE;
		/* Support AP Selection */
		aisRemoveBlackList(prAdapter, prBssDesc);

		/* 4 <4.1> Setup BSSID */
		COPY_MAC_ADDR(prAisBssInfo->aucBSSID, prBssDesc->aucBSSID);

		/* 4 <4.2> Setup Channel, Band */
		prAisBssInfo->ucPrimaryChannel = prBssDesc->ucChannelNum;
		prAisBssInfo->eBand = prBssDesc->eBand;

		/* 4 <4.3> Setup MIB for current BSS */
		prAisBssInfo->u2BeaconInterval = prBssDesc->u2BeaconInterval;
		prAisBssInfo->ucDTIMPeriod = 0;
		prAisBssInfo->u2ATIMWindow = 0;	/* TBD(Kevin) */

		prAisBssInfo->ucBeaconTimeoutCount = AIS_BEACON_TIMEOUT_COUNT_ADHOC;
	} else {
		/* should never happen */
		ASSERT(0);
	}

	/* 3 <5> Set MAC HW */
	/* 4 <5.1> Find Lowest Basic Rate Index for default TX Rate of MMPDU */
	nicTxUpdateBssDefaultRate(prAisBssInfo);

	/* 4 <5.2> Setup channel and bandwidth */
	rlmBssInitForAPandIbss(prAdapter, prAisBssInfo);

	/* 4 <5.3> use command packets to inform firmware */
	nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/* 4 <5.4> enable beaconing */
	bssUpdateBeaconContent(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/* 4 <5.5> Update AdHoc PM parameter */
	nicPmIndicateBssConnected(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/* 3 <6> Set ACTIVE flag. */
	prAisBssInfo->fgIsBeaconActivated = TRUE;
	prAisBssInfo->fgHoldSameBssidForIBSS = TRUE;
}				/* end of aisUpdateBssInfoForMergeIBSS() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will validate the Rx Probe Request Frame and then return
*        result to BSS to indicate if need to send the corresponding Probe Response
*        Frame if the specified conditions were matched.
*
* @param[in] prAdapter          Pointer to the Adapter structure.
* @param[in] prSwRfb            Pointer to SW RFB data structure.
* @param[out] pu4ControlFlags   Control flags for replying the Probe Response
*
* @retval TRUE      Reply the Probe Response
* @retval FALSE     Don't reply the Probe Response
*/
/*----------------------------------------------------------------------------*/
u_int8_t aisValidateProbeReq(IN struct ADAPTER *prAdapter, IN struct SW_RFB *prSwRfb, OUT uint32_t *pu4ControlFlags)
{
	struct WLAN_MAC_MGMT_HEADER *prMgtHdr;
	struct BSS_INFO *prBssInfo;
	struct IE_SSID *prIeSsid = (struct IE_SSID *) NULL;
	uint8_t *pucIE;
	uint16_t u2IELength;
	uint16_t u2Offset = 0;
	u_int8_t fgReplyProbeResp = FALSE;

	ASSERT(prSwRfb);
	ASSERT(pu4ControlFlags);

	prBssInfo = prAdapter->prAisBssInfo;

	/* 4 <1> Parse Probe Req IE and Get IE ptr (SSID, Supported Rate IE, ...) */
	prMgtHdr = (struct WLAN_MAC_MGMT_HEADER *) prSwRfb->pvHeader;

	u2IELength = prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen;
	pucIE = (uint8_t *) ((unsigned long) prSwRfb->pvHeader + prSwRfb->u2HeaderLen);

	IE_FOR_EACH(pucIE, u2IELength, u2Offset) {
		if (IE_ID(pucIE) == ELEM_ID_SSID) {
			if ((!prIeSsid) && (IE_LEN(pucIE) <= ELEM_MAX_LEN_SSID))
				prIeSsid = (struct IE_SSID *) pucIE;

			break;
		}
	}			/* end of IE_FOR_EACH */

	/* 4 <2> Check network conditions */

	if (prBssInfo->eCurrentOPMode == OP_MODE_IBSS) {

		if ((prIeSsid) && ((prIeSsid->ucLength == BC_SSID_LEN) ||	/* WILDCARD SSID */
				   EQUAL_SSID(prBssInfo->aucSSID, prBssInfo->ucSSIDLen,	/* CURRENT SSID */
					      prIeSsid->aucSSID, prIeSsid->ucLength))) {
			fgReplyProbeResp = TRUE;
		}
	}

	return fgReplyProbeResp;

}				/* end of aisValidateProbeReq() */

#endif /* CFG_SUPPORT_ADHOC */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will modify and update necessary information to firmware
*        for disconnection handling
*
* @param[in] prAdapter          Pointer to the Adapter structure.
*
* @retval None
*/
/*----------------------------------------------------------------------------*/
void aisFsmDisconnect(IN struct ADAPTER *prAdapter, IN u_int8_t fgDelayIndication)
{
	struct BSS_INFO *prAisBssInfo;

	ASSERT(prAdapter);

	prAisBssInfo = prAdapter->prAisBssInfo;

	nicPmIndicateBssAbort(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

#if CFG_SUPPORT_ADHOC
	if (prAisBssInfo->fgIsBeaconActivated) {
		nicUpdateBeaconIETemplate(prAdapter,
					  IE_UPD_METHOD_DELETE_ALL, prAdapter->prAisBssInfo->ucBssIndex, 0, NULL, 0);

		prAisBssInfo->fgIsBeaconActivated = FALSE;
	}
#endif

	rlmBssAborted(prAdapter, prAisBssInfo);

	/* 4 <3> Unset the fgIsConnected flag of BSS_DESC_T and send Deauth if needed. */
	if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {

		{
			if (prAdapter->rWifiVar.ucTpTestMode != ENUM_TP_TEST_MODE_NORMAL)
				nicEnterTPTestMode(prAdapter, TEST_MODE_NONE);

#if 0
			if (prAdapter->rWifiVar.ucSigmaTestMode)
				nicEnterTPTestMode(prAdapter, TEST_MODE_NONE);
#endif
		}

		if (prAisBssInfo->ucReasonOfDisconnect == DISCONNECT_REASON_CODE_RADIO_LOST) {
			scanRemoveBssDescByBssid(prAdapter, prAisBssInfo->aucBSSID);

			/* remove from scanning results as well */
			wlanClearBssInScanningResult(prAdapter, prAisBssInfo->aucBSSID);

			/* trials for re-association */
			if (fgDelayIndication) {
				aisFsmIsRequestPending(prAdapter, AIS_REQUEST_RECONNECT, TRUE);
				aisFsmInsertRequest(prAdapter, AIS_REQUEST_RECONNECT);
			}
		} else {
			scanRemoveConnFlagOfBssDescByBssid(prAdapter, prAisBssInfo->aucBSSID);
			prAdapter->rWifiVar.rAisFsmInfo.prTargetBssDesc->fgIsConnected = FALSE;
			prAdapter->rWifiVar.rAisFsmInfo.prTargetBssDesc->fgIsConnecting = FALSE;
		}

		if (fgDelayIndication) {
			if (prAisBssInfo->eCurrentOPMode != OP_MODE_IBSS)
				prAisBssInfo->fgHoldSameBssidForIBSS = FALSE;
		} else {
			prAisBssInfo->fgHoldSameBssidForIBSS = FALSE;
		}
	} else {
		prAisBssInfo->fgHoldSameBssidForIBSS = FALSE;
	}

	/* 4 <4> Change Media State immediately. */
	if (prAisBssInfo->ucReasonOfDisconnect != DISCONNECT_REASON_CODE_REASSOCIATION) {
		aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_DISCONNECTED);

		/* 4 <4.1> sync. with firmware */
		nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);
	}

	if (!fgDelayIndication) {
		/* 4 <5> Deactivate previous AP's STA_RECORD_T or all Clients in Driver if have. */
		if (prAisBssInfo->prStaRecOfAP) {
			/* cnmStaRecChangeState(prAdapter, prAisBssInfo->prStaRecOfAP, STA_STATE_1); */
			prAisBssInfo->prStaRecOfAP = (struct STA_RECORD *) NULL;
		}
	}
#if CFG_SUPPORT_ROAMING
	roamingFsmRunEventAbort(prAdapter);

	/* clear pending roaming connection request */
	aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_SEARCH, TRUE);
	aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_CONNECT, TRUE);
#endif /* CFG_SUPPORT_ROAMING */

	/* 4 <6> Indicate Disconnected Event to Host */
	aisIndicationOfMediaStateToHost(prAdapter, PARAM_MEDIA_STATE_DISCONNECTED, fgDelayIndication);

	/* 4 <7> Trigger AIS FSM */
	aisFsmSteps(prAdapter, AIS_STATE_IDLE);
}				/* end of aisFsmDisconnect() */

static void aisFsmRunEventScanDoneTimeOut(IN struct ADAPTER *prAdapter, unsigned long ulParam)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct CONNECTION_SETTINGS *prConnSettings;

	DEBUGFUNC("aisFsmRunEventScanDoneTimeOut()");

	ASSERT(prAdapter);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	DBGLOG(AIS, STATE, "aisFsmRunEventScanDoneTimeOut Current[%d] Seq=%u\n",
		prAisFsmInfo->eCurrentState, prAisFsmInfo->ucSeqNumOfScanReq);

	/* try to stop scan in CONNSYS */
	aisFsmStateAbort_SCAN(prAdapter);
}				/* end of aisFsmBGSleepTimeout() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will indicate an Event of "Background Scan Time-Out" to AIS FSM.
*
* @param[in] u4Param  Unused timer parameter
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmRunEventBGSleepTimeOut(IN struct ADAPTER *prAdapter, unsigned long ulParamPtr)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	enum ENUM_AIS_STATE eNextState;

	DEBUGFUNC("aisFsmRunEventBGSleepTimeOut()");

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	eNextState = prAisFsmInfo->eCurrentState;

	switch (prAisFsmInfo->eCurrentState) {
	case AIS_STATE_WAIT_FOR_NEXT_SCAN:
		DBGLOG(AIS, LOUD, "EVENT - SCAN TIMER: Idle End - Current Time = %u\n", kalGetTimeTick());

		eNextState = AIS_STATE_LOOKING_FOR;

		SET_NET_PWR_STATE_ACTIVE(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

		break;

	default:
		break;
	}

	/* Call aisFsmSteps() when we are going to change AIS STATE */
	if (eNextState != prAisFsmInfo->eCurrentState)
		aisFsmSteps(prAdapter, eNextState);
}				/* end of aisFsmBGSleepTimeout() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will indicate an Event of "IBSS ALONE Time-Out" to AIS FSM.
*
* @param[in] u4Param  Unused timer parameter
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmRunEventIbssAloneTimeOut(IN struct ADAPTER *prAdapter, unsigned long ulParamPtr)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	enum ENUM_AIS_STATE eNextState;

	DEBUGFUNC("aisFsmRunEventIbssAloneTimeOut()");

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	eNextState = prAisFsmInfo->eCurrentState;

	switch (prAisFsmInfo->eCurrentState) {
	case AIS_STATE_IBSS_ALONE:

		/* There is no one participate in our AdHoc during this TIMEOUT Interval
		 * so go back to search for a valid IBSS again.
		 */

		DBGLOG(AIS, LOUD, "EVENT-IBSS ALONE TIMER: Start pairing\n");

		prAisFsmInfo->fgTryScan = TRUE;

		/* abort timer */
		aisFsmReleaseCh(prAdapter);

		/* Pull back to SEARCH to find candidate again */
		eNextState = AIS_STATE_SEARCH;

		break;

	default:
		break;
	}

	/* Call aisFsmSteps() when we are going to change AIS STATE */
	if (eNextState != prAisFsmInfo->eCurrentState)
		aisFsmSteps(prAdapter, eNextState);
}				/* end of aisIbssAloneTimeOut() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will indicate an Event of "Join Time-Out" to AIS FSM.
*
* @param[in] u4Param  Unused timer parameter
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmRunEventJoinTimeout(IN struct ADAPTER *prAdapter, unsigned long ulParamPtr)
{
	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;
	enum ENUM_AIS_STATE eNextState;
	OS_SYSTIME rCurrentTime;

	DEBUGFUNC("aisFsmRunEventJoinTimeout()");

	prAisBssInfo = prAdapter->prAisBssInfo;
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	eNextState = prAisFsmInfo->eCurrentState;

	GET_CURRENT_SYSTIME(&rCurrentTime);

	switch (prAisFsmInfo->eCurrentState) {
	case AIS_STATE_JOIN:
		DBGLOG(AIS, WARN, "EVENT- JOIN TIMEOUT\n");

		/* 1. Do abort JOIN */
		aisFsmStateAbort_JOIN(prAdapter);

		/* 2. Increase Join Failure Count */
		/* Support AP Selection */
		aisAddBlacklist(prAdapter, prAisFsmInfo->prTargetBssDesc);
		prAisFsmInfo->prTargetBssDesc->ucJoinFailureCount++;

		if (prAisFsmInfo->prTargetBssDesc->ucJoinFailureCount < JOIN_MAX_RETRY_FAILURE_COUNT) {
			/* 3.1 Retreat to AIS_STATE_SEARCH state for next try */
			eNextState = AIS_STATE_SEARCH;
		} else if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
			/* roaming cases */
			/* 3.2 Retreat to AIS_STATE_WAIT_FOR_NEXT_SCAN state for next try */
			eNextState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
		} else
		    if (prAisFsmInfo->rJoinReqTime != 0 && !CHECK_FOR_TIMEOUT
			(rCurrentTime, prAisFsmInfo->rJoinReqTime, SEC_TO_SYSTIME(AIS_JOIN_TIMEOUT))) {
			/* 3.3 Retreat to AIS_STATE_WAIT_FOR_NEXT_SCAN state for next try */
			eNextState = AIS_STATE_WAIT_FOR_NEXT_SCAN;
		} else {
			/* 3.4 Retreat to AIS_STATE_JOIN_FAILURE to terminate join operation */
			eNextState = AIS_STATE_JOIN_FAILURE;
		}

		break;

	case AIS_STATE_NORMAL_TR:
		/* 1. release channel */
		aisFsmReleaseCh(prAdapter);
		prAisFsmInfo->fgIsInfraChannelFinished = TRUE;

		/* 2. process if there is pending scan */
		if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_SCAN, TRUE) == TRUE) {
			wlanClearScanningResult(prAdapter);
			eNextState = AIS_STATE_ONLINE_SCAN;
		}
		/* 3. Process for pending roaming scan */
		else if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_SEARCH, TRUE) == TRUE)
			eNextState = AIS_STATE_LOOKING_FOR;
		/* 4. Process for pending roaming scan */
		else if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_CONNECT, TRUE) == TRUE)
			eNextState = AIS_STATE_SEARCH;
		else if (aisFsmIsRequestPending(prAdapter, AIS_REQUEST_REMAIN_ON_CHANNEL, TRUE) == TRUE)
			eNextState = AIS_STATE_REQ_REMAIN_ON_CHANNEL;

		break;

	default:
		/* release channel */
		aisFsmReleaseCh(prAdapter);
		break;

	}

	/* Call aisFsmSteps() when we are going to change AIS STATE */
	if (eNextState != prAisFsmInfo->eCurrentState)
		aisFsmSteps(prAdapter, eNextState);
}				/* end of aisFsmRunEventJoinTimeout() */

void aisFsmRunEventDeauthTimeout(IN struct ADAPTER *prAdapter, unsigned long ulParamPtr)
{
	aisDeauthXmitComplete(prAdapter, NULL, TX_RESULT_LIFE_TIMEOUT);
}

void aisFsmRunEventSecModeChangeTimeout(IN struct ADAPTER *prAdapter, unsigned long ulParamPtr)
{
	DBGLOG(AIS, INFO, "Beacon security mode change timeout, trigger disconnect!\n");
	aisBssSecurityChanged(prAdapter);
}
#if defined(CFG_TEST_MGMT_FSM) && (CFG_TEST_MGMT_FSM != 0)
/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
void aisTest(void)
{
	struct MSG_AIS_ABORT *prAisAbortMsg;
	struct CONNECTION_SETTINGS *prConnSettings;
	uint8_t aucSSID[] = "pci-11n";
	uint8_t ucSSIDLen = 7;

	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	/* Set Connection Request Issued Flag */
	prConnSettings->fgIsConnReqIssued = TRUE;
	prConnSettings->ucSSIDLen = ucSSIDLen;
	kalMemCopy(prConnSettings->aucSSID, aucSSID, ucSSIDLen);

	prAisAbortMsg = (struct MSG_AIS_ABORT *) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(struct MSG_AIS_ABORT));
	if (!prAisAbortMsg) {

		ASSERT(0);	/* Can't trigger SCAN FSM */
		return;
	}

	prAisAbortMsg->rMsgHdr.eMsgId = MID_HEM_AIS_FSM_ABORT;

	mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *) prAisAbortMsg, MSG_SEND_METHOD_BUF);

	wifi_send_msg(INDX_WIFI, MSG_ID_WIFI_IST, 0);
}
#endif /* CFG_TEST_MGMT_FSM */

/*----------------------------------------------------------------------------*/
/*!
* \brief    This function is used to handle OID_802_11_BSSID_LIST_SCAN
*
* \param[in] prAdapter  Pointer of ADAPTER_T
* \param[in] prSsid     Pointer of SSID_T if specified
* \param[in] pucIe      Pointer to buffer of extra information elements to be attached
* \param[in] u4IeLength Length of information elements
*
* \return none
*/
/*----------------------------------------------------------------------------*/
void aisFsmScanRequest(IN struct ADAPTER *prAdapter, IN struct PARAM_SSID *prSsid, IN uint8_t *pucIe, IN uint32_t u4IeLength)
{
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;

	DEBUGFUNC("aisFsmScanRequest()");

	ASSERT(prAdapter);
	ASSERT(u4IeLength <= MAX_IE_LENGTH);

	prAisBssInfo = prAdapter->prAisBssInfo;
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	DBGLOG(SCN, TRACE, "eCurrentState=%d, fgIsScanReqIssued=%u\n",
		prAisFsmInfo->eCurrentState, prConnSettings->fgIsScanReqIssued);
	if (!prConnSettings->fgIsScanReqIssued) {
		prConnSettings->fgIsScanReqIssued = TRUE;
		scanInitEssResult(prAdapter);

		if (prSsid == NULL) {
			prAisFsmInfo->ucScanSSIDNum = 0;
		} else {
			prAisFsmInfo->ucScanSSIDNum = 1;

			COPY_SSID(prAisFsmInfo->arScanSSID[0].aucSsid,
				  prAisFsmInfo->arScanSSID[0].u4SsidLen, prSsid->aucSsid, prSsid->u4SsidLen);
		}

		if (u4IeLength > 0 && u4IeLength <= MAX_IE_LENGTH) {
			prAisFsmInfo->u4ScanIELength = u4IeLength;
			kalMemCopy(prAisFsmInfo->aucScanIEBuf, pucIe, u4IeLength);
		} else {
			prAisFsmInfo->u4ScanIELength = 0;
		}

		if (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR) {
			if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE
			    && prAisFsmInfo->fgIsInfraChannelFinished == FALSE) {
				/* 802.1x might not finished yet, pend it for later handling .. */
				aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);
			} else {
				if (prAisFsmInfo->fgIsChannelGranted == TRUE) {
					DBGLOG(SCN, WARN,
					       "Scan Request with channel granted for join operation: %d, %d",
					       prAisFsmInfo->fgIsChannelGranted, prAisFsmInfo->fgIsChannelRequested);
				}

				/* start online scan */
				wlanClearScanningResult(prAdapter);
				aisFsmSteps(prAdapter, AIS_STATE_ONLINE_SCAN);
			}
		} else if (prAisFsmInfo->eCurrentState == AIS_STATE_IDLE) {
			wlanClearScanningResult(prAdapter);
			aisFsmSteps(prAdapter, AIS_STATE_SCAN);
		} else {
			aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);
		}
	} else {
		DBGLOG(SCN, WARN, "Scan Request dropped. (state: %d)\n",
			prAisFsmInfo->eCurrentState);
	}

}	/* end of aisFsmScanRequest() */

/*----------------------------------------------------------------------------*/
/*!
* \brief    This function is used to handle OID_802_11_BSSID_LIST_SCAN
*
* \param[in] prAdapter  Pointer of ADAPTER_T
* \param[in] ucSsidNum  Number of SSID
* \param[in] prSsid     Pointer to the array of SSID_T if specified
* \param[in] pucIe      Pointer to buffer of extra information elements to be attached
* \param[in] u4IeLength Length of information elements
*
* \return none
*/
/*----------------------------------------------------------------------------*/
void
aisFsmScanRequestAdv(IN struct ADAPTER *prAdapter, IN uint8_t ucSsidNum,
		IN struct PARAM_SSID *prSsid,
		IN enum ENUM_SCAN_TYPE eScanType,
		IN uint8_t *pucIe, IN uint32_t u4IeLength,
		IN uint8_t u4ChannelNum, IN struct RF_CHANNEL_INFO *prChannel)
{
	uint32_t i;
	struct CONNECTION_SETTINGS *prConnSettings;
	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;

	DEBUGFUNC("aisFsmScanRequestAdv()");

	ASSERT(prAdapter);
	ASSERT(ucSsidNum <= SCN_SSID_MAX_NUM);
	ASSERT(u4IeLength <= MAX_IE_LENGTH);
	ASSERT(u4ChannelNum <= MAXIMUM_OPERATION_CHANNEL_LIST);

	prAisBssInfo = prAdapter->prAisBssInfo;
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	DBGLOG(SCN, TRACE, "eCurrentState=%d, fgIsScanReqIssued=%u\n",
			prAisFsmInfo->eCurrentState, prConnSettings->fgIsScanReqIssued);

	if (!prConnSettings->fgIsScanReqIssued) {
		prConnSettings->fgIsScanReqIssued = TRUE;
		scanInitEssResult(prAdapter);

		if (ucSsidNum == 0) {
			prAisFsmInfo->ucScanSSIDNum = 0;
		} else {
			prAisFsmInfo->ucScanSSIDNum = ucSsidNum;

			for (i = 0; i < ucSsidNum; i++) {
				COPY_SSID(prAisFsmInfo->arScanSSID[i].aucSsid,
					  prAisFsmInfo->arScanSSID[i].u4SsidLen,
					  prSsid[i].aucSsid, prSsid[i].u4SsidLen);
			}
		}
		prAisFsmInfo->eScanType = eScanType;

		prAisFsmInfo->u4ScanChannelNum = u4ChannelNum;
		kalMemCopy(&prAisFsmInfo->arChannel[0], prChannel,
			sizeof(struct RF_CHANNEL_INFO) * u4ChannelNum);

		if (u4IeLength > 0 && u4IeLength <= MAX_IE_LENGTH) {
			prAisFsmInfo->u4ScanIELength = u4IeLength;
			kalMemCopy(prAisFsmInfo->aucScanIEBuf, pucIe, u4IeLength);
		} else {
			prAisFsmInfo->u4ScanIELength = 0;
		}

		if (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR) {
			if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE
			    && prAisFsmInfo->fgIsInfraChannelFinished == FALSE) {
				/* 802.1x might not finished yet, pend it for later handling .. */
				aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);
			} else {
				if (prAisFsmInfo->fgIsChannelGranted == TRUE) {
					DBGLOG(SCN, WARN,
					       "Scan Request with channel granted for join operation: %d, %d",
					       prAisFsmInfo->fgIsChannelGranted, prAisFsmInfo->fgIsChannelRequested);
				}

				/* start online scan */
				wlanClearScanningResult(prAdapter);
				aisFsmSteps(prAdapter, AIS_STATE_ONLINE_SCAN);
			}
		} else if (prAisFsmInfo->eCurrentState == AIS_STATE_IDLE) {
			wlanClearScanningResult(prAdapter);
			aisFsmSteps(prAdapter, AIS_STATE_SCAN);
		} else {
			aisFsmInsertRequest(prAdapter, AIS_REQUEST_SCAN);
		}
	} else {
		DBGLOG(SCN, WARN, "Scan Request dropped. (state: %d)\n",
			prAisFsmInfo->eCurrentState);
	}

} /* end of aisFsmScanRequestAdv() */

/*----------------------------------------------------------------------------*/
/*!
* \brief    This function is invoked when CNM granted channel privilege
*
* \param[in] prAdapter  Pointer of ADAPTER_T
*
* \return none
*/
/*----------------------------------------------------------------------------*/
void aisFsmRunEventChGrant(IN struct ADAPTER *prAdapter, IN struct MSG_HDR *prMsgHdr)
{
	struct BSS_INFO *prAisBssInfo;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct MSG_CH_GRANT *prMsgChGrant;
	uint8_t ucTokenID;
	uint32_t u4GrantInterval;
	uint32_t u4Entry = 0;

	ASSERT(prAdapter);
	ASSERT(prMsgHdr);

	prAisBssInfo = prAdapter->prAisBssInfo;
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prMsgChGrant = (struct MSG_CH_GRANT *) prMsgHdr;

	ucTokenID = prMsgChGrant->ucTokenID;
	u4GrantInterval = prMsgChGrant->u4GrantInterval;

#if CFG_SISO_SW_DEVELOP
	/* Driver record granted CH in BSS info */
	prAisBssInfo->fgIsGranted = TRUE;
	prAisBssInfo->eBandGranted = prMsgChGrant->eRfBand;
	prAisBssInfo->ucPrimaryChannelGranted = prMsgChGrant->ucPrimaryChannel;
#endif

	/* 1. free message */
	cnmMemFree(prAdapter, prMsgHdr);

	if (prAisFsmInfo->eCurrentState == AIS_STATE_REQ_CHANNEL_JOIN && prAisFsmInfo->ucSeqNumOfChReq == ucTokenID) {
		/* 2. channel privilege has been approved */
		prAisFsmInfo->u4ChGrantedInterval = u4GrantInterval;

		/* 3. state transition to join/ibss-alone/ibss-merge */
		/* 3.1 set timeout timer in cases join could not be completed */
		cnmTimerStartTimer(prAdapter,
				   &prAisFsmInfo->rJoinTimeoutTimer,
				   prAisFsmInfo->u4ChGrantedInterval - AIS_JOIN_CH_GRANT_THRESHOLD);
		/* 3.2 set local variable to indicate join timer is ticking */
		prAisFsmInfo->fgIsInfraChannelFinished = FALSE;

		/* 3.3 switch to join state */
		/* Three cases can switch to join state:
		** 1. There's an available PMKSA in wpa_supplicant
		** 2. found okc pmkid entry for this BSS
		** 3. current state is disconnected. In this case, supplicant may not get a valid pmksa,
		** so no pmkid will be passed to driver, so we no need to wait pmkid anyway.
		*/
		if (!prAdapter->rWifiVar.rConnSettings.fgOkcPmksaReady ||
		    (rsnSearchPmkidEntry(prAdapter, prAisFsmInfo->prTargetBssDesc->aucBSSID, &u4Entry) &&
		     prAdapter->rWifiVar.rAisSpecificBssInfo.arPmkidCache[u4Entry].fgPmkidExist))
			aisFsmSteps(prAdapter, AIS_STATE_JOIN);

		prAisFsmInfo->fgIsChannelGranted = TRUE;
	} else if (prAisFsmInfo->eCurrentState == AIS_STATE_REQ_REMAIN_ON_CHANNEL &&
		   prAisFsmInfo->ucSeqNumOfChReq == ucTokenID) {
		/* 2. channel privilege has been approved */
		prAisFsmInfo->u4ChGrantedInterval = u4GrantInterval;

#if CFG_SUPPORT_NCHO
		if (prAdapter->rNchoInfo.fgECHOEnabled == TRUE &&
			prAdapter->rNchoInfo.fgIsSendingAF == TRUE &&
			prAdapter->rNchoInfo.fgChGranted == FALSE) {
			DBGLOG(INIT, TRACE, "NCHO complete rAisChGrntComp trace time is %u\n", kalGetTimeTick());
			prAdapter->rNchoInfo.fgChGranted = TRUE;
			complete(&prAdapter->prGlueInfo->rAisChGrntComp);
		}
#endif
		/* 3.1 set timeout timer in cases upper layer cancel_remain_on_channel never comes */
		cnmTimerStartTimer(prAdapter, &prAisFsmInfo->rChannelTimeoutTimer, prAisFsmInfo->u4ChGrantedInterval);

		/* 3.2 switch to remain_on_channel state */
		aisFsmSteps(prAdapter, AIS_STATE_REMAIN_ON_CHANNEL);

		/* 3.3. indicate upper layer for channel ready */
		kalReadyOnChannel(prAdapter->prGlueInfo,
				  prAisFsmInfo->rChReqInfo.u8Cookie,
				  prAisFsmInfo->rChReqInfo.eBand,
				  prAisFsmInfo->rChReqInfo.eSco,
				  prAisFsmInfo->rChReqInfo.ucChannelNum, prAisFsmInfo->rChReqInfo.u4DurationMs);

		prAisFsmInfo->fgIsChannelGranted = TRUE;
	} else {		/* mismatched grant */
		/* 2. return channel privilege to CNM immediately */
		aisFsmReleaseCh(prAdapter);
	}
}				/* end of aisFsmRunEventChGrant() */

/*----------------------------------------------------------------------------*/
/*!
* \brief    This function is to inform CNM that channel privilege
*           has been released
*
* \param[in] prAdapter  Pointer of ADAPTER_T
*
* \return none
*/
/*----------------------------------------------------------------------------*/
void aisFsmReleaseCh(IN struct ADAPTER *prAdapter)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct MSG_CH_ABORT *prMsgChAbort;

	ASSERT(prAdapter);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	if (prAisFsmInfo->fgIsChannelGranted == TRUE || prAisFsmInfo->fgIsChannelRequested == TRUE) {

		prAisFsmInfo->fgIsChannelRequested = FALSE;
		prAisFsmInfo->fgIsChannelGranted = FALSE;

		/* 1. return channel privilege to CNM immediately */
		prMsgChAbort = (struct MSG_CH_ABORT *) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(struct MSG_CH_ABORT));
		if (!prMsgChAbort) {
			ASSERT(0);	/* Can't release Channel to CNM */
			return;
		}

		prMsgChAbort->rMsgHdr.eMsgId = MID_MNY_CNM_CH_ABORT;
		prMsgChAbort->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
		prMsgChAbort->ucTokenID = prAisFsmInfo->ucSeqNumOfChReq;
#if CFG_SUPPORT_DBDC
		prMsgChAbort->eDBDCBand = ENUM_BAND_AUTO;
#endif /*CFG_SUPPORT_DBDC*/
		mboxSendMsg(prAdapter, MBOX_ID_0, (struct MSG_HDR *) prMsgChAbort, MSG_SEND_METHOD_BUF);
	}
}				/* end of aisFsmReleaseCh() */

/*----------------------------------------------------------------------------*/
/*!
* \brief    This function is to inform AIS that corresponding beacon has not
*           been received for a while and probing is not successful
*
* \param[in] prAdapter  Pointer of ADAPTER_T
*
* \return none
*/
/*----------------------------------------------------------------------------*/
void aisBssBeaconTimeout(IN struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prAisBssInfo;
	u_int8_t fgDoAbortIndication = FALSE;
	struct CONNECTION_SETTINGS *prConnSettings;

	ASSERT(prAdapter);

	prAisBssInfo = prAdapter->prAisBssInfo;
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	/* 4 <1> Diagnose Connection for Beacon Timeout Event */
	if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
		if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
			struct STA_RECORD *prStaRec = prAisBssInfo->prStaRecOfAP;

			if (prStaRec)
				fgDoAbortIndication = TRUE;

		} else if (prAisBssInfo->eCurrentOPMode == OP_MODE_IBSS) {
			fgDoAbortIndication = TRUE;
		}
	}
	/* 4 <2> invoke abort handler */
	if (fgDoAbortIndication) {
		prConnSettings->fgIsDisconnectedByNonRequest = FALSE;
		if (prConnSettings->eReConnectLevel < RECONNECT_LEVEL_USER_SET) {
			prConnSettings->eReConnectLevel = RECONNECT_LEVEL_BEACON_TIMEOUT;
			prConnSettings->fgIsConnReqIssued = TRUE;
		}
		DBGLOG(AIS, EVENT, "aisBssBeaconTimeout\n");
		aisFsmStateAbort(prAdapter, DISCONNECT_REASON_CODE_RADIO_LOST, TRUE);
	}
}				/* end of aisBssBeaconTimeout() */

void aisBssSecurityChanged(struct ADAPTER *prAdapter)
{
	prAdapter->rWifiVar.rConnSettings.fgIsDisconnectedByNonRequest = TRUE;
	aisFsmStateAbort(prAdapter, DISCONNECT_REASON_CODE_DEAUTHENTICATED, FALSE);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief    This function is to inform AIS that corresponding beacon has not
*           been received for a while and probing is not successful
*
* \param[in] prAdapter  Pointer of ADAPTER_T
*
* \return none
*/
/*----------------------------------------------------------------------------*/
void aisBssLinkDown(IN struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prAisBssInfo;
	u_int8_t fgDoAbortIndication = FALSE;
	struct CONNECTION_SETTINGS *prConnSettings;

	ASSERT(prAdapter);

	prAisBssInfo = prAdapter->prAisBssInfo;
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	/* 4 <1> Diagnose Connection for Beacon Timeout Event */
	if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
		if (prAisBssInfo->eCurrentOPMode == OP_MODE_INFRASTRUCTURE) {
			struct STA_RECORD *prStaRec = prAisBssInfo->prStaRecOfAP;

			if (prStaRec)
				fgDoAbortIndication = TRUE;

		} else if (prAisBssInfo->eCurrentOPMode == OP_MODE_IBSS) {
			fgDoAbortIndication = TRUE;
		}
	}
	/* 4 <2> invoke abort handler */
	if (fgDoAbortIndication) {
		prConnSettings->fgIsDisconnectedByNonRequest = TRUE;
		DBGLOG(AIS, EVENT, "aisBssLinkDown\n");
		aisFsmStateAbort(prAdapter, DISCONNECT_REASON_CODE_DISASSOCIATED, FALSE);
	}

    /* kalIndicateStatusAndComplete(prAdapter->prGlueInfo, WLAN_STATUS_SCAN_COMPLETE, NULL, 0); */
}				/* end of aisBssBeaconTimeout() */

/*----------------------------------------------------------------------------*/
/*!
* \brief    This function is to inform AIS that DEAUTH frame has been
*           sent and thus state machine could go ahead
*
* \param[in] prAdapter  Pointer of ADAPTER_T
* \param[in] prMsduInfo Pointer of MSDU_INFO_T for DEAUTH frame
* \param[in] prAdapter  Pointer of ADAPTER_T
*
* \return WLAN_STATUS_SUCCESS
*/
/*----------------------------------------------------------------------------*/
uint32_t
aisDeauthXmitComplete(IN struct ADAPTER *prAdapter, IN struct MSDU_INFO *prMsduInfo, IN enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct AIS_FSM_INFO *prAisFsmInfo;

	ASSERT(prAdapter);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	if (rTxDoneStatus == TX_RESULT_SUCCESS)
		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rDeauthDoneTimer);

	if (prAisFsmInfo->eCurrentState == AIS_STATE_DISCONNECTING) {
		DBGLOG(AIS, EVENT, "aisDeauthXmitComplete\n");
		if (rTxDoneStatus != TX_RESULT_DROPPED_IN_DRIVER && rTxDoneStatus != TX_RESULT_QUEUE_CLEARANCE)
			aisFsmStateAbort(prAdapter, DISCONNECT_REASON_CODE_NEW_CONNECTION, FALSE);
	} else {
		DBGLOG(AIS, WARN, "DEAUTH frame transmitted without further handling");
	}

	return WLAN_STATUS_SUCCESS;

}				/* end of aisDeauthXmitComplete() */

#if CFG_SUPPORT_ROAMING
/*----------------------------------------------------------------------------*/
/*!
* @brief This function will indicate an Event of "Looking for a candidate due to weak signal" to AIS FSM.
*
* @param[in] u4ReqScan  Requesting Scan or not
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmRunEventRoamingDiscovery(IN struct ADAPTER *prAdapter, uint32_t u4ReqScan)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct CONNECTION_SETTINGS *prConnSettings;
	enum ENUM_AIS_REQUEST_TYPE eAisRequest;

	DBGLOG(AIS, LOUD, "aisFsmRunEventRoamingDiscovery()\n");

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	/* search candidates by best rssi */
	prConnSettings->eConnectionPolicy = CONNECT_BY_SSID_BEST_RSSI;

	/* TODO: Stop roaming event in FW */
#if CFG_SUPPORT_WFD
#if CFG_ENABLE_WIFI_DIRECT
	{
		/* Check WFD is running */
		struct WFD_CFG_SETTINGS *prWfdCfgSettings = (struct WFD_CFG_SETTINGS *) NULL;

		prWfdCfgSettings = &(prAdapter->rWifiVar.rWfdConfigureSettings);
		if ((prWfdCfgSettings->ucWfdEnable != 0)) {
			DBGLOG(ROAMING, INFO, "WFD is running. Stop roaming.\n");
			roamingFsmRunEventRoam(prAdapter);
			roamingFsmRunEventFail(prAdapter, ROAMING_FAIL_REASON_NOCANDIDATE);
			return;
		}
	}
#endif
#endif

	/* results are still new */
	if (!u4ReqScan) {
		roamingFsmRunEventRoam(prAdapter);
		eAisRequest = AIS_REQUEST_ROAMING_CONNECT;
	} else {
		if (prAisFsmInfo->eCurrentState == AIS_STATE_ONLINE_SCAN
		    || prAisFsmInfo->eCurrentState == AIS_STATE_LOOKING_FOR) {
			eAisRequest = AIS_REQUEST_ROAMING_CONNECT;
		} else {
			eAisRequest = AIS_REQUEST_ROAMING_SEARCH;
		}
	}

	if (prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR && prAisFsmInfo->fgIsInfraChannelFinished == TRUE) {
		if (eAisRequest == AIS_REQUEST_ROAMING_SEARCH)
			aisFsmSteps(prAdapter, AIS_STATE_LOOKING_FOR);
		else
			aisFsmSteps(prAdapter, AIS_STATE_SEARCH);
	} else {
		aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_SEARCH, TRUE);
		aisFsmIsRequestPending(prAdapter, AIS_REQUEST_ROAMING_CONNECT, TRUE);

		aisFsmInsertRequest(prAdapter, eAisRequest);
	}
}				/* end of aisFsmRunEventRoamingDiscovery() */

/*----------------------------------------------------------------------------*/
/*!
* @brief Update the time of ScanDone for roaming and transit to Roam state.
*
* @param (none)
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
enum ENUM_AIS_STATE aisFsmRoamingScanResultsUpdate(IN struct ADAPTER *prAdapter)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct ROAMING_INFO *prRoamingFsmInfo;
	enum ENUM_AIS_STATE eNextState;

	DBGLOG(AIS, LOUD, "->aisFsmRoamingScanResultsUpdate()\n");

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prRoamingFsmInfo = (struct ROAMING_INFO *) &(prAdapter->rWifiVar.rRoamingInfo);

	roamingFsmScanResultsUpdate(prAdapter);

	eNextState = prAisFsmInfo->eCurrentState;
	if (prRoamingFsmInfo->eCurrentState == ROAMING_STATE_DISCOVERY) {
		roamingFsmRunEventRoam(prAdapter);
		eNextState = AIS_STATE_SEARCH;
	} else if (prAisFsmInfo->eCurrentState == AIS_STATE_LOOKING_FOR) {
		eNextState = AIS_STATE_SEARCH;
	} else if (prAisFsmInfo->eCurrentState == AIS_STATE_ONLINE_SCAN) {
		eNextState = AIS_STATE_NORMAL_TR;
	}

	return eNextState;
}				/* end of aisFsmRoamingScanResultsUpdate() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will modify and update necessary information to firmware
*        for disconnection of last AP before switching to roaming bss.
*
* @param IN prAdapter          Pointer to the Adapter structure.
*           prTargetStaRec     Target of StaRec of roaming
*
* @retval None
*/
/*----------------------------------------------------------------------------*/
void aisFsmRoamingDisconnectPrevAP(IN struct ADAPTER *prAdapter, IN struct STA_RECORD *prTargetStaRec)
{
	struct BSS_INFO *prAisBssInfo;

	DBGLOG(AIS, EVENT, "aisFsmRoamingDisconnectPrevAP()");

	ASSERT(prAdapter);

	prAisBssInfo = prAdapter->prAisBssInfo;

	nicPmIndicateBssAbort(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	/* Not invoke rlmBssAborted() here to avoid prAisBssInfo->fg40mBwAllowed
	 * to be reset. RLM related parameters will be reset again when handling
	 * association response in rlmProcessAssocRsp(). 20110413
	 */
	/* rlmBssAborted(prAdapter, prAisBssInfo); */

	/* 4 <3> Unset the fgIsConnected flag of BSS_DESC_T and send Deauth if needed. */
	if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED) {
		struct PARAM_SSID rSsid;
		struct BSS_DESC *prBssDesc = NULL;

		COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen, prAisBssInfo->aucSSID, prAisBssInfo->ucSSIDLen);
		prBssDesc = scanSearchBssDescByBssidAndSsid(prAdapter, prAisBssInfo->aucBSSID, TRUE, &rSsid);
		if (prBssDesc) {
			prBssDesc->fgIsConnected = FALSE;
			prBssDesc->fgIsConnecting = FALSE;
		}
	}

	/* 4 <4> Change Media State immediately. */
	aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_DISCONNECTED);

	/* 4 <4.1> sync. with firmware */
	prTargetStaRec->ucBssIndex = (prAdapter->ucHwBssIdNum + 1);	/* Virtial BSSID */
	nicUpdateBss(prAdapter, prAdapter->prAisBssInfo->ucBssIndex);

	secRemoveBssBcEntry(prAdapter, prAisBssInfo, TRUE);
	prTargetStaRec->ucBssIndex = prAdapter->prAisBssInfo->ucBssIndex;
}				/* end of aisFsmRoamingDisconnectPrevAP() */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will update the contain of BSS_INFO_T for AIS network once
*        the roaming was completed.
*
* @param IN prAdapter          Pointer to the Adapter structure.
*           prStaRec           StaRec of roaming AP
*           prAssocRspSwRfb
*
* @retval None
*/
/*----------------------------------------------------------------------------*/
void aisUpdateBssInfoForRoamingAP(IN struct ADAPTER *prAdapter, IN struct STA_RECORD *prStaRec, IN struct SW_RFB *prAssocRspSwRfb)
{
	struct BSS_INFO *prAisBssInfo;

	DBGLOG(AIS, LOUD, "aisUpdateBssInfoForRoamingAP()");

	ASSERT(prAdapter);

	prAisBssInfo = prAdapter->prAisBssInfo;

	/* 4 <1.1> Change FW's Media State immediately. */
	aisChangeMediaState(prAdapter, PARAM_MEDIA_STATE_CONNECTED);

	/* 4 <1.2> Deactivate previous AP's STA_RECORD_T in Driver if have. */
	if ((prAisBssInfo->prStaRecOfAP) &&
	    (prAisBssInfo->prStaRecOfAP != prStaRec) && (prAisBssInfo->prStaRecOfAP->fgIsInUse)) {
		/* cnmStaRecChangeState(prAdapter, prAisBssInfo->prStaRecOfAP, STA_STATE_1); */
		cnmStaRecFree(prAdapter, prAisBssInfo->prStaRecOfAP);
	}

	/* 4 <1.4> Update BSS_INFO_T */
	aisUpdateBssInfoForJOIN(prAdapter, prStaRec, prAssocRspSwRfb);

	/* 4 <1.3> Activate current AP's STA_RECORD_T in Driver. */
	cnmStaRecChangeState(prAdapter, prStaRec, STA_STATE_3);

	/* 4 <1.6> Indicate Connected Event to Host immediately. */
	/* Require BSSID, Association ID, Beacon Interval.. from AIS_BSS_INFO_T */
	aisIndicationOfMediaStateToHost(prAdapter, PARAM_MEDIA_STATE_CONNECTED, FALSE);
}				/* end of aisFsmRoamingUpdateBss() */

#endif /* CFG_SUPPORT_ROAMING */

/*----------------------------------------------------------------------------*/
/*!
* @brief Check if there is any pending request and remove it (optional)
*
* @param prAdapter
*        eReqType
*        bRemove
*
* @return TRUE
*         FALSE
*/
/*----------------------------------------------------------------------------*/
u_int8_t aisFsmIsRequestPending(IN struct ADAPTER *prAdapter, IN enum ENUM_AIS_REQUEST_TYPE eReqType, IN u_int8_t bRemove)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct AIS_REQ_HDR *prPendingReqHdr, *prPendingReqHdrNext;

	ASSERT(prAdapter);
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	/* traverse through pending request list */
	LINK_FOR_EACH_ENTRY_SAFE(prPendingReqHdr,
				 prPendingReqHdrNext, &(prAisFsmInfo->rPendingReqList), rLinkEntry, struct AIS_REQ_HDR) {
		/* check for specified type */
		if (prPendingReqHdr->eReqType == eReqType) {
			/* check if need to remove */
			if (bRemove == TRUE) {
				LINK_REMOVE_KNOWN_ENTRY(&(prAisFsmInfo->rPendingReqList),
							&(prPendingReqHdr->rLinkEntry));

				cnmMemFree(prAdapter, prPendingReqHdr);
			}

			return TRUE;
		}
	}

	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Get next pending request
*
* @param prAdapter
*
* @return P_AIS_REQ_HDR_T
*/
/*----------------------------------------------------------------------------*/
struct AIS_REQ_HDR *aisFsmGetNextRequest(IN struct ADAPTER *prAdapter)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct AIS_REQ_HDR *prPendingReqHdr;

	ASSERT(prAdapter);
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	LINK_REMOVE_HEAD(&(prAisFsmInfo->rPendingReqList), prPendingReqHdr, struct AIS_REQ_HDR *);

	return prPendingReqHdr;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Insert a new request
*
* @param prAdapter
*        eReqType
*
* @return TRUE
*         FALSE
*/
/*----------------------------------------------------------------------------*/
u_int8_t aisFsmInsertRequest(IN struct ADAPTER *prAdapter, IN enum ENUM_AIS_REQUEST_TYPE eReqType)
{
	struct AIS_REQ_HDR *prAisReq;
	struct AIS_FSM_INFO *prAisFsmInfo;

	ASSERT(prAdapter);
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

	prAisReq = (struct AIS_REQ_HDR *) cnmMemAlloc(prAdapter, RAM_TYPE_MSG, sizeof(struct AIS_REQ_HDR));

	if (!prAisReq) {
		ASSERT(0);	/* Can't generate new message */
		return FALSE;
	}
	DBGLOG(AIS, INFO, "aisFsmInsertRequest\n");

	prAisReq->eReqType = eReqType;

	/* attach request into pending request list */
	LINK_INSERT_TAIL(&prAisFsmInfo->rPendingReqList, &prAisReq->rLinkEntry);

	DBGLOG(AIS, TRACE, "eCurrentState=%d, eReqType=%d, u4NumElem=%d\n",
		prAisFsmInfo->eCurrentState, eReqType, prAisFsmInfo->rPendingReqList.u4NumElem);

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Flush all pending requests
*
* @param prAdapter
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void aisFsmFlushRequest(IN struct ADAPTER *prAdapter)
{
	struct AIS_REQ_HDR *prAisReq;

	ASSERT(prAdapter);

	while ((prAisReq = aisFsmGetNextRequest(prAdapter)) != NULL)
		cnmMemFree(prAdapter, prAisReq);
}

void aisFsmRunEventRemainOnChannel(IN struct ADAPTER *prAdapter, IN struct MSG_HDR *prMsgHdr)
{
	struct MSG_REMAIN_ON_CHANNEL *prRemainOnChannel;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct CONNECTION_SETTINGS *prConnSettings;

	DEBUGFUNC("aisFsmRunEventRemainOnChannel()");

	ASSERT(prAdapter);
	ASSERT(prMsgHdr);

	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prConnSettings = &(prAdapter->rWifiVar.rConnSettings);

	prRemainOnChannel = (struct MSG_REMAIN_ON_CHANNEL *) prMsgHdr;

	/* record parameters */
	prAisFsmInfo->rChReqInfo.eBand = prRemainOnChannel->eBand;
	prAisFsmInfo->rChReqInfo.eSco = prRemainOnChannel->eSco;
	prAisFsmInfo->rChReqInfo.ucChannelNum = prRemainOnChannel->ucChannelNum;
	prAisFsmInfo->rChReqInfo.u4DurationMs = prRemainOnChannel->u4DurationMs;
	prAisFsmInfo->rChReqInfo.u8Cookie = prRemainOnChannel->u8Cookie;

	if (prAisFsmInfo->eCurrentState == AIS_STATE_IDLE || prAisFsmInfo->eCurrentState == AIS_STATE_NORMAL_TR) {
		/* transit to next state */
		aisFsmSteps(prAdapter, AIS_STATE_REQ_REMAIN_ON_CHANNEL);
	} else {
		aisFsmInsertRequest(prAdapter, AIS_REQUEST_REMAIN_ON_CHANNEL);
	}

	/* free messages */
	cnmMemFree(prAdapter, prMsgHdr);
}

void aisFsmRunEventCancelRemainOnChannel(IN struct ADAPTER *prAdapter, IN struct MSG_HDR *prMsgHdr)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;
	struct MSG_CANCEL_REMAIN_ON_CHANNEL *prCancelRemainOnChannel;

	ASSERT(prAdapter);
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;

	prCancelRemainOnChannel = (struct MSG_CANCEL_REMAIN_ON_CHANNEL *) prMsgHdr;

	/* 1. Check the cookie first */
	if (prCancelRemainOnChannel->u8Cookie == prAisFsmInfo->rChReqInfo.u8Cookie) {

		/* 2. release channel privilege/request */
		if (prAisFsmInfo->eCurrentState == AIS_STATE_REQ_REMAIN_ON_CHANNEL) {
			/* 2.1 elease channel */
			aisFsmReleaseCh(prAdapter);
		} else if (prAisFsmInfo->eCurrentState == AIS_STATE_REMAIN_ON_CHANNEL) {
			/* 2.1 release channel */
			aisFsmReleaseCh(prAdapter);

			/* 2.2 stop channel timeout timer */
			cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rChannelTimeoutTimer);
		}

		/* 3. clear pending request of remain_on_channel */
		aisFsmIsRequestPending(prAdapter, AIS_REQUEST_REMAIN_ON_CHANNEL, TRUE);

		/* 4. decide which state to retreat */
		if (prAisFsmInfo->eCurrentState == AIS_STATE_REQ_REMAIN_ON_CHANNEL ||
			prAisFsmInfo->eCurrentState == AIS_STATE_REMAIN_ON_CHANNEL) {
			if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED)
				aisFsmSteps(prAdapter, AIS_STATE_NORMAL_TR);
			else
				aisFsmSteps(prAdapter, AIS_STATE_IDLE);
		}
	}

	/* 5. free message */
	cnmMemFree(prAdapter, prMsgHdr);
}

void aisFsmRunEventMgmtFrameTx(IN struct ADAPTER *prAdapter, IN struct MSG_HDR *prMsgHdr)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct MSG_MGMT_TX_REQUEST *prMgmtTxMsg = (struct MSG_MGMT_TX_REQUEST *) NULL;

	do {
		ASSERT((prAdapter != NULL) && (prMsgHdr != NULL));

		prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

		if (prAisFsmInfo == NULL)
			break;

		prMgmtTxMsg = (struct MSG_MGMT_TX_REQUEST *) prMsgHdr;

		aisFuncTxMgmtFrame(prAdapter,
				   &prAisFsmInfo->rMgmtTxInfo, prMgmtTxMsg->prMgmtMsduInfo, prMgmtTxMsg->u8Cookie);

	} while (FALSE);

	if (prMsgHdr)
		cnmMemFree(prAdapter, prMsgHdr);
}				/* aisFsmRunEventMgmtFrameTx */

#if CFG_SUPPORT_NCHO
void aisFsmRunEventNchoActionFrameTx(IN struct ADAPTER *prAdapter, IN struct MSG_HDR *prMsgHdr)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo = (struct BSS_INFO *) NULL;
	struct MSG_MGMT_TX_REQUEST *prMgmtTxMsg = (struct MSG_MGMT_TX_REQUEST *) NULL;
	struct MSDU_INFO *prMgmtFrame = (struct MSDU_INFO *) NULL;
	struct _ACTION_VENDOR_SPEC_FRAME_T *prVendorSpec = NULL;
	uint8_t *pucFrameBuf = (uint8_t *) NULL;
	struct _NCHO_INFO_T *prNchoInfo = NULL;
	uint16_t u2PktLen = 0;

	do {
		ASSERT_BREAK((prAdapter != NULL) && (prMsgHdr != NULL));
		DBGLOG(REQ, TRACE, "NCHO in aisFsmRunEventNchoActionFrameTx\n");

		prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
		prNchoInfo = &(prAdapter->rNchoInfo);
		prAisBssInfo = &(prAdapter->rWifiVar.arBssInfo[NETWORK_TYPE_AIS]);

		if (prAisFsmInfo == NULL)
			break;

		prMgmtTxMsg = (struct MSG_MGMT_TX_REQUEST *) prMsgHdr;
		u2PktLen = (uint16_t) OFFSET_OF(struct _ACTION_VENDOR_SPEC_FRAME_T, aucElemInfo[0]) +
				  prNchoInfo->rParamActionFrame.i4len +
				  MAC_TX_RESERVED_FIELD;
		prMgmtFrame = cnmMgtPktAlloc(prAdapter, u2PktLen);
		if (prMgmtFrame == NULL) {
			ASSERT(FALSE);
			DBGLOG(REQ, ERROR, "NCHO there is no memory for prMgmtFrame\n");
			break;
		}
		prMgmtTxMsg->prMgmtMsduInfo = prMgmtFrame;

		pucFrameBuf = (uint8_t *) ((unsigned long) prMgmtFrame->prPacket + MAC_TX_RESERVED_FIELD);
		prVendorSpec = (struct _ACTION_VENDOR_SPEC_FRAME_T *)pucFrameBuf;
		prVendorSpec->u2FrameCtrl = MAC_FRAME_ACTION;
		prVendorSpec->u2Duration = 0;
		prVendorSpec->u2SeqCtrl = 0;
		COPY_MAC_ADDR(prVendorSpec->aucDestAddr, prNchoInfo->rParamActionFrame.aucBssid);
		COPY_MAC_ADDR(prVendorSpec->aucSrcAddr, prAisBssInfo->aucOwnMacAddr);
		COPY_MAC_ADDR(prVendorSpec->aucBSSID, prAisBssInfo->aucBSSID);

		kalMemCopy(prVendorSpec->aucElemInfo,
			   prNchoInfo->rParamActionFrame.aucData,
			   prNchoInfo->rParamActionFrame.i4len);

		prMgmtFrame->u2FrameLength = u2PktLen;

		aisFuncTxMgmtFrame(prAdapter,
				   &prAisFsmInfo->rMgmtTxInfo, prMgmtTxMsg->prMgmtMsduInfo, prMgmtTxMsg->u8Cookie);

	} while (FALSE);

	if (prMsgHdr)
		cnmMemFree(prAdapter, prMsgHdr);

}				/* aisFsmRunEventNchoActionFrameTx */
#endif

void aisFsmRunEventChannelTimeout(IN struct ADAPTER *prAdapter, unsigned long ulParamPtr)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_INFO *prAisBssInfo;

	DEBUGFUNC("aisFsmRunEventRemainOnChannel()");

	ASSERT(prAdapter);
	prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
	prAisBssInfo = prAdapter->prAisBssInfo;

	if (prAisFsmInfo->eCurrentState == AIS_STATE_REMAIN_ON_CHANNEL) {
		/* 1. release channel */
		aisFsmReleaseCh(prAdapter);

		/* 2. stop channel timeout timer */
		cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rChannelTimeoutTimer);

		/* 3. expiration indication to upper layer */
		kalRemainOnChannelExpired(prAdapter->prGlueInfo,
					  prAisFsmInfo->rChReqInfo.u8Cookie,
					  prAisFsmInfo->rChReqInfo.eBand,
					  prAisFsmInfo->rChReqInfo.eSco, prAisFsmInfo->rChReqInfo.ucChannelNum);

		/* 4. decide which state to retreat */
		if (prAisBssInfo->eConnectionState == PARAM_MEDIA_STATE_CONNECTED)
			aisFsmSteps(prAdapter, AIS_STATE_NORMAL_TR);
		else
			aisFsmSteps(prAdapter, AIS_STATE_IDLE);

	} else {
		DBGLOG(AIS, WARN, "Unexpected remain_on_channel timeout event\n");
		DBGLOG(AIS, STATE, "CURRENT State: [%s]\n", apucDebugAisState[prAisFsmInfo->eCurrentState]);
	}
}

uint32_t
aisFsmRunEventMgmtFrameTxDone(IN struct ADAPTER *prAdapter,
			      IN struct MSDU_INFO *prMsduInfo, IN enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct AIS_MGMT_TX_REQ_INFO *prMgmtTxReqInfo = (struct AIS_MGMT_TX_REQ_INFO *) NULL;
	u_int8_t fgIsSuccess = FALSE;

	do {
		ASSERT((prAdapter != NULL) && (prMsduInfo != NULL));

		prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);
		prMgmtTxReqInfo = &(prAisFsmInfo->rMgmtTxInfo);

		if (rTxDoneStatus != TX_RESULT_SUCCESS) {
			DBGLOG(AIS, ERROR, "Mgmt Frame TX Fail, Status:%d.\n", rTxDoneStatus);
		} else {
			fgIsSuccess = TRUE;
#if CFG_SUPPORT_NCHO
			if (prAdapter->rNchoInfo.fgECHOEnabled == TRUE &&
				prAdapter->rNchoInfo.fgIsSendingAF == TRUE &&
				prAdapter->rNchoInfo.fgChGranted == TRUE) {
				prAdapter->rNchoInfo.fgIsSendingAF = FALSE;
				DBGLOG(AIS, TRACE, "NCHO action frame tx done");
			}
#endif
		}

		if (prMgmtTxReqInfo->prMgmtTxMsdu == prMsduInfo) {
			kalIndicateMgmtTxStatus(prAdapter->prGlueInfo,
						prMgmtTxReqInfo->u8Cookie,
						fgIsSuccess, prMsduInfo->prPacket, (uint32_t) prMsduInfo->u2FrameLength);

			prMgmtTxReqInfo->prMgmtTxMsdu = NULL;
		}

	} while (FALSE);

	return WLAN_STATUS_SUCCESS;

}				/* aisFsmRunEventMgmtFrameTxDone */

void aisFsmRunEventSetOkcPmk(IN struct ADAPTER *prAdapter)
{
	struct AIS_FSM_INFO *prAisFsmInfo = &prAdapter->rWifiVar.rAisFsmInfo;

	prAdapter->rWifiVar.rConnSettings.fgOkcPmksaReady = TRUE;
	cnmTimerStopTimer(prAdapter, &prAisFsmInfo->rWaitOkcPMKTimer);
	if (prAisFsmInfo->eCurrentState == AIS_STATE_REQ_CHANNEL_JOIN &&
		prAisFsmInfo->fgIsChannelGranted)
		aisFsmSteps(prAdapter, AIS_STATE_JOIN);
}

static void aisFsmSetOkcTimeout(IN struct ADAPTER *prAdapter, unsigned long ulParam)
{
	struct AIS_FSM_INFO *prAisFsmInfo = &prAdapter->rWifiVar.rAisFsmInfo;

	DBGLOG(AIS, WARN, "Wait OKC PMKID timeout, current state[%d],fgIsChannelGranted=%d\n"
		, prAisFsmInfo->eCurrentState, prAisFsmInfo->fgIsChannelGranted);
	if (prAisFsmInfo->eCurrentState == AIS_STATE_REQ_CHANNEL_JOIN &&
		prAisFsmInfo->fgIsChannelGranted)
		aisFsmSteps(prAdapter, AIS_STATE_JOIN);
}

uint32_t
aisFuncTxMgmtFrame(IN struct ADAPTER *prAdapter,
		   IN struct AIS_MGMT_TX_REQ_INFO *prMgmtTxReqInfo, IN struct MSDU_INFO *prMgmtTxMsdu, IN uint64_t u8Cookie)
{
	uint32_t rWlanStatus = WLAN_STATUS_SUCCESS;
	struct MSDU_INFO *prTxMsduInfo = (struct MSDU_INFO *) NULL;
	struct WLAN_MAC_HEADER *prWlanHdr = (struct WLAN_MAC_HEADER *) NULL;
	struct STA_RECORD *prStaRec = (struct STA_RECORD *) NULL;

	do {
		ASSERT((prAdapter != NULL) && (prMgmtTxReqInfo != NULL));

		if (prMgmtTxReqInfo->fgIsMgmtTxRequested) {

			/* 1. prMgmtTxReqInfo->prMgmtTxMsdu != NULL */
			/* Packet on driver, not done yet, drop it. */
			prTxMsduInfo = prMgmtTxReqInfo->prMgmtTxMsdu;
			if (prTxMsduInfo != NULL) {

				kalIndicateMgmtTxStatus(prAdapter->prGlueInfo,
							prMgmtTxReqInfo->u8Cookie,
							FALSE,
							prTxMsduInfo->prPacket, (uint32_t) prTxMsduInfo->u2FrameLength);

				/* Leave it to TX Done handler. */
				/* cnmMgtPktFree(prAdapter, prTxMsduInfo); */
				prMgmtTxReqInfo->prMgmtTxMsdu = NULL;
			}
			/* 2. prMgmtTxReqInfo->prMgmtTxMsdu == NULL */
			/* Packet transmitted, wait tx done. (cookie issue) */
		}

		ASSERT(prMgmtTxReqInfo->prMgmtTxMsdu == NULL);

		prWlanHdr = (struct WLAN_MAC_HEADER *) ((unsigned long) prMgmtTxMsdu->prPacket + MAC_TX_RESERVED_FIELD);
		prStaRec = cnmGetStaRecByAddress(prAdapter, prAdapter->prAisBssInfo->ucBssIndex, prWlanHdr->aucAddr1);

		TX_SET_MMPDU(prAdapter,
			     prMgmtTxMsdu,
			     (prStaRec !=
			      NULL) ? (prStaRec->ucBssIndex) : (prAdapter->prAisBssInfo->ucBssIndex),
			     (prStaRec != NULL) ? (prStaRec->ucIndex) : (STA_REC_INDEX_NOT_FOUND),
			     WLAN_MAC_MGMT_HEADER_LEN, prMgmtTxMsdu->u2FrameLength,
			     aisFsmRunEventMgmtFrameTxDone, MSDU_RATE_MODE_AUTO);
		prMgmtTxReqInfo->u8Cookie = u8Cookie;
		prMgmtTxReqInfo->prMgmtTxMsdu = prMgmtTxMsdu;
		prMgmtTxReqInfo->fgIsMgmtTxRequested = TRUE;

		nicTxConfigPktControlFlag(prMgmtTxMsdu, MSDU_CONTROL_FLAG_FORCE_TX, TRUE);

		/* send to TX queue */
		nicTxEnqueueMsdu(prAdapter, prMgmtTxMsdu);

	} while (FALSE);

	return rWlanStatus;
}				/* aisFuncTxMgmtFrame */

/*----------------------------------------------------------------------------*/
/*!
* @brief This function will validate the Rx Action Frame and indicate to uppoer layer
*           if the specified conditions were matched.
*
* @param[in] prAdapter          Pointer to the Adapter structure.
* @param[in] prSwRfb            Pointer to SW RFB data structure.
* @param[out] pu4ControlFlags   Control flags for replying the Probe Response
*
* @retval none
*/
/*----------------------------------------------------------------------------*/
void aisFuncValidateRxActionFrame(IN struct ADAPTER *prAdapter, IN struct SW_RFB *prSwRfb)
{
	struct AIS_FSM_INFO *prAisFsmInfo = (struct AIS_FSM_INFO *) NULL;

	DEBUGFUNC("aisFuncValidateRxActionFrame");

	do {

		ASSERT((prAdapter != NULL) && (prSwRfb != NULL));

		prAisFsmInfo = &(prAdapter->rWifiVar.rAisFsmInfo);

		if (1 /* prAisFsmInfo->u4AisPacketFilter & PARAM_PACKET_FILTER_ACTION_FRAME */) {
			/* Leave the action frame to wpa_supplicant. */
			kalIndicateRxMgmtFrame(prAdapter->prGlueInfo, prSwRfb);
		}

	} while (FALSE);

	return;

}				/* aisFuncValidateRxActionFrame */

/* Support AP Selection */
void aisRefreshFWKBlacklist(struct ADAPTER *prAdapter)
{
	struct CONNECTION_SETTINGS *prConnSettings =
		&prAdapter->rWifiVar.rConnSettings;
	struct AIS_BLACKLIST_ITEM *prEntry = NULL;
	struct LINK *prBlackList = &prConnSettings->rBlackList.rUsingLink;

	DBGLOG(AIS, INFO, "Refresh all the BSSes' fgIsInFWKBlacklist to FALSE\n");
	LINK_FOR_EACH_ENTRY(prEntry, prBlackList, rLinkEntry,
		struct AIS_BLACKLIST_ITEM) {
		prEntry->fgIsInFWKBlacklist = FALSE;
	}
}


struct AIS_BLACKLIST_ITEM *
aisAddBlacklist(struct ADAPTER *prAdapter, struct BSS_DESC *prBssDesc)
{
	struct CONNECTION_SETTINGS *prConnSettings =
		&prAdapter->rWifiVar.rConnSettings;
	struct AIS_BLACKLIST_ITEM *prEntry = NULL;
	struct LINK *prFreeList = &prConnSettings->rBlackList.rFreeLink;
	struct LINK *prBlackList = &prConnSettings->rBlackList.rUsingLink;

	if (!prBssDesc) {
		DBGLOG(AIS, ERROR, "bss descriptor is NULL\n");
		return NULL;
	}
	if (prBssDesc->prBlack) {
		GET_CURRENT_SYSTIME(&prBssDesc->prBlack->rAddTime);
		prBssDesc->prBlack->ucCount++;
		if (prBssDesc->prBlack->ucCount > 10)
			prBssDesc->prBlack->ucCount = 10;
		DBGLOG(AIS, INFO, "update blacklist for %pM, count %d\n",
			prBssDesc->aucBSSID, prBssDesc->prBlack->ucCount);
		return prBssDesc->prBlack;
	}

	prEntry = aisQueryBlackList(prAdapter, prBssDesc);

	if (prEntry) {
		GET_CURRENT_SYSTIME(&prEntry->rAddTime);
		prBssDesc->prBlack = prEntry;
		prEntry->ucCount++;
		if (prEntry->ucCount > 10)
			prEntry->ucCount = 10;
		DBGLOG(AIS, INFO, "update blacklist for %pM, count %d\n",
			prBssDesc->aucBSSID, prEntry->ucCount);
		return prEntry;
	}

	LINK_REMOVE_HEAD(prFreeList, prEntry, struct AIS_BLACKLIST_ITEM *);
	if (!prEntry)
		prEntry = kalMemAlloc(sizeof(struct AIS_BLACKLIST_ITEM), VIR_MEM_TYPE);
	if (!prEntry) {
		DBGLOG(AIS, WARN, "No memory to allocate\n");
		return NULL;
	}
	kalMemZero(prEntry, sizeof(*prEntry));
	prEntry->ucCount = 1;
	/* Support AP Selection */
	prEntry->fgIsInFWKBlacklist = FALSE;
	COPY_MAC_ADDR(prEntry->aucBSSID, prBssDesc->aucBSSID);
	COPY_SSID(prEntry->aucSSID, prEntry->ucSSIDLen, prBssDesc->aucSSID,
		prBssDesc->ucSSIDLen);
	GET_CURRENT_SYSTIME(&prEntry->rAddTime);
	LINK_INSERT_HEAD(prBlackList, &prEntry->rLinkEntry);
	prBssDesc->prBlack = prEntry;

	DBGLOG(AIS, INFO, "Add %pM to black List\n", prBssDesc->aucBSSID);
	return prEntry;
}

void aisRemoveBlackList(struct ADAPTER *prAdapter, struct BSS_DESC *prBssDesc)
{
	struct CONNECTION_SETTINGS *prConnSettings =
		&prAdapter->rWifiVar.rConnSettings;
	struct AIS_BLACKLIST_ITEM *prEntry = NULL;
	struct LINK *prFreeList = &prConnSettings->rBlackList.rFreeLink;
	struct LINK *prBlackList = &prConnSettings->rBlackList.rUsingLink;

	prEntry = aisQueryBlackList(prAdapter, prBssDesc);
	if (!prEntry)
		return;
	LINK_REMOVE_KNOWN_ENTRY(prBlackList, &prEntry->rLinkEntry);
	LINK_INSERT_HEAD(prFreeList, &prEntry->rLinkEntry);
	prBssDesc->prBlack = NULL;
	DBGLOG(AIS, INFO, "Remove %pM from blacklist\n", prBssDesc->aucBSSID);
}

struct AIS_BLACKLIST_ITEM *
aisQueryBlackList(struct ADAPTER *prAdapter, struct BSS_DESC *prBssDesc)
{
	struct CONNECTION_SETTINGS *prConnSettings =
		&prAdapter->rWifiVar.rConnSettings;
	struct AIS_BLACKLIST_ITEM *prEntry = NULL;
	struct LINK *prBlackList = &prConnSettings->rBlackList.rUsingLink;

	if (!prBssDesc)
		return NULL;
	else if (prBssDesc->prBlack)
		return prBssDesc->prBlack;

	LINK_FOR_EACH_ENTRY(prEntry, prBlackList, rLinkEntry,
		struct AIS_BLACKLIST_ITEM) {
		if (EQUAL_MAC_ADDR(prBssDesc->aucBSSID, prEntry) &&
			EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
			prEntry->aucSSID, prEntry->ucSSIDLen)) {
			prBssDesc->prBlack = prEntry;
			return prEntry;
		}
	}
	DBGLOG(AIS, TRACE, "%pM is not in blacklist\n", prBssDesc->aucBSSID);
	return NULL;
}

void aisRemoveTimeoutBlacklist(struct ADAPTER *prAdapter)
{
	struct CONNECTION_SETTINGS *prConnSettings =
		&prAdapter->rWifiVar.rConnSettings;
	struct AIS_BLACKLIST_ITEM *prEntry = NULL;
	struct AIS_BLACKLIST_ITEM *prNextEntry = NULL;
	struct LINK *prBlackList = &prConnSettings->rBlackList.rUsingLink;
	struct LINK *prFreeList = &prConnSettings->rBlackList.rFreeLink;
	OS_SYSTIME rCurrent;

	GET_CURRENT_SYSTIME(&rCurrent);

	LINK_FOR_EACH_ENTRY_SAFE(prEntry, prNextEntry, prBlackList, rLinkEntry,
		struct AIS_BLACKLIST_ITEM) {
			/* Support AP Selection */
			if (prEntry->fgIsInFWKBlacklist == TRUE)
				continue;
			/* end Support AP Selection */
			if (!CHECK_FOR_TIMEOUT(rCurrent, prEntry->rAddTime,
				SEC_TO_MSEC(AIS_BLACKLIST_TIMEOUT)))
				continue;
			LINK_REMOVE_KNOWN_ENTRY(prBlackList, &prEntry->rLinkEntry);
			LINK_INSERT_HEAD(prFreeList, &prEntry->rLinkEntry);
	}
}

static void aisRemoveDisappearedBlacklist(struct ADAPTER *prAdapter)
{
	struct CONNECTION_SETTINGS *prConnSettings =
		&prAdapter->rWifiVar.rConnSettings;
	struct AIS_BLACKLIST_ITEM *prEntry = NULL;
	struct AIS_BLACKLIST_ITEM *prNextEntry = NULL;
	struct LINK *prBlackList = &prConnSettings->rBlackList.rUsingLink;
	struct LINK *prFreeList = &prConnSettings->rBlackList.rFreeLink;
	struct BSS_DESC *prBssDesc = NULL;
	struct LINK *prBSSDescList = &prAdapter->rWifiVar.rScanInfo.rBSSDescList;
	uint32_t u4Current = (uint32_t)kalGetBootTime();
	u_int8_t fgDisappeared = TRUE;

	LINK_FOR_EACH_ENTRY_SAFE(prEntry, prNextEntry, prBlackList, rLinkEntry,
		struct AIS_BLACKLIST_ITEM) {
		fgDisappeared = TRUE;
		LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry,
			struct BSS_DESC) {
			if (prBssDesc->prBlack == prEntry ||
				(EQUAL_MAC_ADDR(prBssDesc->aucBSSID, prEntry) &&
				EQUAL_SSID(prBssDesc->aucSSID, prBssDesc->ucSSIDLen,
				prEntry->aucSSID, prEntry->ucSSIDLen))) {
				fgDisappeared = FALSE;
				break;
			}
		}
		if (!fgDisappeared || (u4Current - prEntry->u4DisapperTime) < 600 * USEC_PER_SEC)
			continue;
		DBGLOG(AIS, INFO, "Remove disappeared blacklist %s %pM\n",
			prEntry->aucSSID, prEntry->aucBSSID);
		LINK_REMOVE_KNOWN_ENTRY(prBlackList, &prEntry->rLinkEntry);
		LINK_INSERT_HEAD(prFreeList, &prEntry->rLinkEntry);
	}
}

u_int8_t aisApOverload(struct AIS_BLACKLIST_ITEM *prBlack)
{
	switch (prBlack->u2AuthStatus) {
	case STATUS_CODE_ASSOC_DENIED_AP_OVERLOAD:
	case STATUS_CODE_ASSOC_DENIED_BANDWIDTH:
		return TRUE;
	}
	switch (prBlack->u2DeauthReason) {
	case REASON_CODE_DISASSOC_LACK_OF_BANDWIDTH:
	case REASON_CODE_DISASSOC_AP_OVERLOAD:
		return TRUE;
	}
	return FALSE;
}

uint16_t aisCalculateBlackListScore(struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc)
{
	if (!prBssDesc->prBlack)
		prBssDesc->prBlack = aisQueryBlackList(prAdapter, prBssDesc);

	if (!prBssDesc->prBlack)
		return 100;
	else if (aisApOverload(prBssDesc->prBlack) ||
		prBssDesc->prBlack->ucCount >= 10)
		return 0;
	return 100 - prBssDesc->prBlack->ucCount * 10;
}
