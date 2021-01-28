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
/*! \file   connac.c
*    \brief  Internal driver stack will export the required procedures here for GLUE Layer.
*
*    This file contains all routines which are exported from MediaTek 802.11 Wireless
*    LAN driver stack to GLUE Layer.
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


/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*
* Common Default Setting
* TODO: This table can be modified by command or cfg file.
*/
UINT_8 arAcQIdx2GroupId[MAC_TXQ_NUM] = {
	GROUP0_INDEX,    /* MAC_TXQ_AC0_INDEX */
	GROUP1_INDEX,    /* MAC_TXQ_AC1_INDEX */
	GROUP2_INDEX,    /* MAC_TXQ_AC2_INDEX */
	GROUP4_INDEX,    /* MAC_TXQ_AC3_INDEX */

	GROUP0_INDEX,    /* MAC_TXQ_AC10_INDEX */
	GROUP1_INDEX,    /* MAC_TXQ_AC11_INDEX */
	GROUP2_INDEX,    /* MAC_TXQ_AC12_INDEX */
	GROUP4_INDEX,    /* MAC_TXQ_AC13_INDEX */

	GROUP0_INDEX,    /* MAC_TXQ_AC20_INDEX */
	GROUP1_INDEX,    /* MAC_TXQ_AC21_INDEX */
	GROUP2_INDEX,    /* MAC_TXQ_AC22_INDEX */
	GROUP4_INDEX,    /* MAC_TXQ_AC23_INDEX */

	GROUP0_INDEX,    /* MAC_TXQ_AC30_INDEX */
	GROUP1_INDEX,    /* MAC_TXQ_AC31_INDEX */
	GROUP2_INDEX,    /* MAC_TXQ_AC32_INDEX */
	GROUP4_INDEX,    /* MAC_TXQ_AC33_INDEX */

	GROUP5_INDEX,    /* MAC_TXQ_ALTX_0_INDEX */
	GROUP5_INDEX,    /* MAC_TXQ_BMC_0_INDEX */
	GROUP5_INDEX,    /* MAC_TXQ_BCN_0_INDEX */
	GROUP5_INDEX,    /* MAC_TXQ_PSMP_0_INDEX */

	GROUP5_INDEX,    /* Reserved */
	GROUP5_INDEX,    /* Reserved */
	GROUP5_INDEX,    /* Reserved */
	GROUP5_INDEX,    /* Reserved */

	GROUP5_INDEX,    /* Reserved */
	GROUP5_INDEX,    /* Reserved */
};

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
VOID asicCapInit(IN P_ADAPTER_T prAdapter)
{
	P_GLUE_INFO_T prGlueInfo;
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);

	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;

	prChipInfo->u2HifTxdSize = 0;
	prChipInfo->u2TxInitCmdPort = 0;
	prChipInfo->u2TxFwDlPort = 0;
	prChipInfo->fillHifTxDesc = NULL;
	prChipInfo->u4ExtraTxByteCount = 0;

	switch (prGlueInfo->u4InfType) {
#if defined(_HIF_PCIE)
	case MT_DEV_INF_PCIE:
		prChipInfo->u2TxInitCmdPort = TX_RING_FWDL_IDX_3;
		prChipInfo->u2TxFwDlPort = TX_RING_FWDL_IDX_3;
		prChipInfo->ucPacketFormat = TXD_PKT_FORMAT_TXD;
		prChipInfo->u4HifDmaShdlBaseAddr = PCIE_HIF_DMASHDL_BASE;

		HAL_MCR_WR(prAdapter, CONN_HIF_ON_IRQ_ENA, BIT(0));
#if 0 /* PCIE is not ready to use driver dma scheduler setting now. */
		asicPcieDmaShdlInit(prAdapter);
#endif
		break;
#endif /* _HIF_PCIE */
#if defined(_HIF_USB)
	case MT_DEV_INF_USB:
		prChipInfo->u2HifTxdSize = USB_HIF_TXD_LEN;
		prChipInfo->fillHifTxDesc = fillUsbHifTxDesc;
		prChipInfo->u2TxInitCmdPort = USB_DATA_BULK_OUT_EP8;
		prChipInfo->u2TxFwDlPort = USB_DATA_BULK_OUT_EP4;
		prChipInfo->ucPacketFormat = TXD_PKT_FORMAT_TXD_PAYLOAD;
		prChipInfo->u4ExtraTxByteCount = EXTRA_TXD_SIZE_FOR_TX_BYTE_COUNT;
		prChipInfo->u4HifDmaShdlBaseAddr = USB_HIF_DMASHDL_BASE;
		asicUsbDmaShdlInit(prAdapter);
		break;
#endif /* _HIF_USB */
#if defined(_HIF_SDIO)
	case MT_DEV_INF_SDIO:
		prChipInfo->ucPacketFormat = TXD_PKT_FORMAT_TXD_PAYLOAD;
		prChipInfo->u4ExtraTxByteCount = EXTRA_TXD_SIZE_FOR_TX_BYTE_COUNT;
		break;
#endif /* _HIF_SDIO */
	default:
		break;
	}
}

VOID asicEnableFWDownload(IN P_ADAPTER_T prAdapter, IN BOOL fgEnable)
{
	P_GLUE_INFO_T prGlueInfo;

	ASSERT(prAdapter);

	prGlueInfo = prAdapter->prGlueInfo;

	switch (prGlueInfo->u4InfType) {
#if defined(_HIF_PCIE)
	case MT_DEV_INF_PCIE:
	{
		WPDMA_GLO_CFG_STRUCT GloCfg;

		kalDevRegRead(prGlueInfo, WPDMA_GLO_CFG, &GloCfg.word);

		GloCfg.field_conn.bypass_dmashdl_txring3 = fgEnable;

		kalDevRegWrite(prGlueInfo, WPDMA_GLO_CFG, GloCfg.word);
	}
	break;
#endif /* _HIF_PCIE */

#if defined(_HIF_USB)
	case MT_DEV_INF_USB:
	{
		UINT_32 u4Value = 0;

		ASSERT(prAdapter);

		HAL_MCR_RD(prAdapter, CONNAC_UDMA_TX_QSEL, &u4Value);

		if (fgEnable)
			u4Value |= FW_DL_EN;
		else
			u4Value &= ~FW_DL_EN;

		HAL_MCR_WR(prAdapter, CONNAC_UDMA_TX_QSEL, u4Value);
	}
	break;
#endif /* _HIF_USB */

	default:
		break;
	}
}

VOID asicCmmDmaShdlInit(IN P_ADAPTER_T prAdapter, UINT_32 u4RefillGroup)
{
	UINT_32 u4BaseAddr, u4MacVal = 0;
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);

	prChipInfo = prAdapter->chip_info;
	u4BaseAddr = prChipInfo->u4HifDmaShdlBaseAddr;

	HAL_MCR_RD(prAdapter, CONN_HIF_DMASHDL_PACKET_MAX_SIZE(u4BaseAddr), &u4MacVal);
	u4MacVal &= ~(PLE_PKT_MAX_SIZE_MASK | PSE_PKT_MAX_SIZE_MASK);
	u4MacVal |= PLE_PKT_MAX_SIZE_NUM(0x1);
	u4MacVal |= PSE_PKT_MAX_SIZE_NUM(0x8);
	HAL_MCR_WR(prAdapter, CONN_HIF_DMASHDL_PACKET_MAX_SIZE(u4BaseAddr), u4MacVal);

	HAL_MCR_WR(prAdapter, CONN_HIF_DMASHDL_REFILL_CONTROL(u4BaseAddr), u4RefillGroup);

	u4MacVal = DMASHDL_MIN_QUOTA_NUM(0x3);
	u4MacVal |= DMASHDL_MAX_QUOTA_NUM(0x1ff);
	HAL_MCR_WR(prAdapter, CONN_HIF_DMASHDL_GROUP0_CTRL(u4BaseAddr), u4MacVal);
	HAL_MCR_WR(prAdapter, CONN_HIF_DMASHDL_GROUP1_CTRL(u4BaseAddr), u4MacVal);
	HAL_MCR_WR(prAdapter, CONN_HIF_DMASHDL_GROUP2_CTRL(u4BaseAddr), u4MacVal);
	HAL_MCR_WR(prAdapter, CONN_HIF_DMASHDL_GROUP3_CTRL(u4BaseAddr), u4MacVal);
	HAL_MCR_WR(prAdapter, CONN_HIF_DMASHDL_GROUP4_CTRL(u4BaseAddr), u4MacVal);

	u4MacVal = ((arAcQIdx2GroupId[MAC_TXQ_AC0_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE0_MAPPING) |
		(arAcQIdx2GroupId[MAC_TXQ_AC1_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE1_MAPPING) |
		(arAcQIdx2GroupId[MAC_TXQ_AC2_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE2_MAPPING) |
		(arAcQIdx2GroupId[MAC_TXQ_AC3_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE3_MAPPING) |
		(arAcQIdx2GroupId[MAC_TXQ_AC10_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE4_MAPPING) |
		(arAcQIdx2GroupId[MAC_TXQ_AC11_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE5_MAPPING) |
		(arAcQIdx2GroupId[MAC_TXQ_AC12_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE6_MAPPING) |
		(arAcQIdx2GroupId[MAC_TXQ_AC13_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING0_QUEUE7_MAPPING));
	HAL_MCR_WR(prAdapter, CONN_HIF_DMASHDL_Q_MAP0(u4BaseAddr), u4MacVal);

	u4MacVal = ((arAcQIdx2GroupId[MAC_TXQ_AC20_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING1_QUEUE8_MAPPING) |
		(arAcQIdx2GroupId[MAC_TXQ_AC21_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING1_QUEUE9_MAPPING) |
		(arAcQIdx2GroupId[MAC_TXQ_AC22_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING1_QUEUE10_MAPPING) |
		(arAcQIdx2GroupId[MAC_TXQ_AC23_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING1_QUEUE11_MAPPING) |
		(arAcQIdx2GroupId[MAC_TXQ_AC30_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING1_QUEUE12_MAPPING) |
		(arAcQIdx2GroupId[MAC_TXQ_AC31_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING1_QUEUE13_MAPPING) |
		(arAcQIdx2GroupId[MAC_TXQ_AC32_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING1_QUEUE14_MAPPING) |
		(arAcQIdx2GroupId[MAC_TXQ_AC33_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING1_QUEUE15_MAPPING));
	HAL_MCR_WR(prAdapter, CONN_HIF_DMASHDL_Q_MAP1(u4BaseAddr), u4MacVal);

	u4MacVal = ((arAcQIdx2GroupId[MAC_TXQ_ALTX_0_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING2_QUEUE16_MAPPING) |
		(arAcQIdx2GroupId[MAC_TXQ_BMC_0_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING2_QUEUE17_MAPPING)|
		(arAcQIdx2GroupId[MAC_TXQ_BCN_0_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING2_QUEUE18_MAPPING) |
		(arAcQIdx2GroupId[MAC_TXQ_PSMP_0_INDEX] << CONN_HIF_DMASHDL_TOP_QUEUE_MAPPING2_QUEUE19_MAPPING));
	HAL_MCR_WR(prAdapter, CONN_HIF_DMASHDL_Q_MAP2(u4BaseAddr), u4MacVal);
}

VOID fillTxDescAppendByHost(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo, IN UINT_16 u4MsduId,
			    IN dma_addr_t rDmaAddr, OUT PUINT_8 pucBuffer)
{
	P_HW_MAC_TX_DESC_APPEND_T prHwTxDescAppend;

	prHwTxDescAppend = (P_HW_MAC_TX_DESC_APPEND_T) (pucBuffer + NIC_TX_DESC_LONG_FORMAT_LENGTH);
	prHwTxDescAppend->CONNAC_APPEND.au2MsduId[0] = u4MsduId | TXD_MSDU_ID_VLD;
	prHwTxDescAppend->CONNAC_APPEND.arPtrLen[0].u4Ptr0 = rDmaAddr;
	prHwTxDescAppend->CONNAC_APPEND.arPtrLen[0].u2Len0 = prMsduInfo->u2FrameLength | TXD_LEN_AL | TXD_LEN_ML;
}

VOID fillTxDescAppendByHostV2(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo, IN UINT_16 u4MsduId,
			      IN dma_addr_t rDmaAddr, OUT PUINT_8 pucBuffer)
{
	P_HW_MAC_TX_DESC_APPEND_T prHwTxDescAppend;

	prHwTxDescAppend = (P_HW_MAC_TX_DESC_APPEND_T) (pucBuffer + NIC_TX_DESC_LONG_FORMAT_LENGTH);
	prHwTxDescAppend->CONNAC_APPEND.au2MsduId[0] = u4MsduId | TXD_MSDU_ID_VLD;
	prHwTxDescAppend->CONNAC_APPEND.arPtrLen[0].u4Ptr0 = rDmaAddr;
	prHwTxDescAppend->CONNAC_APPEND.arPtrLen[0].u2Len0 = (prMsduInfo->u2FrameLength & TXD_LEN_MASK_V2) |
		((rDmaAddr >> TXD_ADDR2_OFFSET) & TXD_ADDR2_MASK) | TXD_LEN_ML_V2;
}

VOID fillTxDescAppendByCR4(IN P_ADAPTER_T prAdapter, IN P_MSDU_INFO_T prMsduInfo, IN UINT_16 u4MsduId,
			   IN dma_addr_t rDmaAddr, OUT PUINT_8 pucBuffer)
{
	P_HW_MAC_TX_DESC_APPEND_T prHwTxDescAppend;

	prHwTxDescAppend = (P_HW_MAC_TX_DESC_APPEND_T) (pucBuffer + NIC_TX_DESC_LONG_FORMAT_LENGTH);
	prHwTxDescAppend->CR4_APPEND.u2MsduToken = u4MsduId;
	prHwTxDescAppend->CR4_APPEND.ucBufNum = 1;
	prHwTxDescAppend->CR4_APPEND.au4BufPtr[0] = rDmaAddr;
	prHwTxDescAppend->CR4_APPEND.au2BufLen[0] = prMsduInfo->u2FrameLength;
}

#if defined(_HIF_PCIE)
VOID asicPcieDmaShdlInit(IN P_ADAPTER_T prAdapter)
{
	UINT_32 u4BaseAddr, u4MacVal;
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);

	prChipInfo = prAdapter->chip_info;
	u4BaseAddr = prChipInfo->u4HifDmaShdlBaseAddr;

	/*
	* Enable refill control group 0, 1, 2, 4, 5. (Keep group3 for bypass usage.)
	* Keep all group low refill priority to prevent low group starvation if we have high group.
	*/
	u4MacVal = (CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP3_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP6_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP7_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP8_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP9_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP10_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP11_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP12_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP13_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP14_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP15_REFILL_DISABLE_MASK);

	asicCmmDmaShdlInit(prAdapter, u4MacVal);

	/*
	* HIF Scheduler Setting
	* Group15(CMD) is highest priority.
	*/
	HAL_MCR_WR(prAdapter, CONN_HIF_DMASHDL_SHDL_SET0(u4BaseAddr), 0x6012345f);
	HAL_MCR_WR(prAdapter, CONN_HIF_DMASHDL_SHDL_SET1(u4BaseAddr), 0xedcba987);
}

VOID asicLowPowerOwnRead(IN P_ADAPTER_T prAdapter, OUT PBOOLEAN pfgResult)
{
	UINT_32 u4RegValue;

	HAL_MCR_RD(prAdapter, CONN_HIF_ON_LPCTL, &u4RegValue);
	*pfgResult = (u4RegValue & PCIE_LPCR_HOST_SET_OWN) == 0 ? TRUE : FALSE;
}

VOID asicLowPowerOwnSet(IN P_ADAPTER_T prAdapter, OUT PBOOLEAN pfgResult)
{
	UINT_32 u4RegValue;

	HAL_MCR_WR(prAdapter, CONN_HIF_ON_LPCTL, PCIE_LPCR_HOST_SET_OWN);
	HAL_MCR_RD(prAdapter, CONN_HIF_ON_LPCTL, &u4RegValue);
	*pfgResult = (u4RegValue & PCIE_LPCR_HOST_SET_OWN) == 1;
}

VOID asicLowPowerOwnClear(IN P_ADAPTER_T prAdapter, OUT PBOOLEAN pfgResult)
{
	UINT_32 u4RegValue;

	HAL_MCR_WR(prAdapter, CONN_HIF_ON_LPCTL, PCIE_LPCR_HOST_CLR_OWN);
	HAL_MCR_RD(prAdapter, CONN_HIF_ON_LPCTL, &u4RegValue);
	*pfgResult = (u4RegValue & PCIE_LPCR_HOST_SET_OWN) == 0;
}
#endif /* _HIF_PCIE */

#if defined(_HIF_USB)
/* DMS Scheduler Init */
VOID asicUsbDmaShdlInit(IN P_ADAPTER_T prAdapter)
{
	UINT_32 u4BaseAddr, u4MacVal;
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);

	prChipInfo = prAdapter->chip_info;
	u4BaseAddr = prChipInfo->u4HifDmaShdlBaseAddr;

	/*
	* USB Endpoint OUT/DMA Scheduler Group Mapping (HW Define)
	* EP#4 / Group0  (DATA)
	* EP#5 / Group1  (DATA)
	* EP#6 / Group2  (DATA)
	* EP#7 / Group3  (DATA)
	* EP#9 / Group4  (DATA)
	* EP#8 / Group15 (CMD)
	*/
	arAcQIdx2GroupId[MAC_TXQ_AC3_INDEX] = GROUP3_INDEX;
	arAcQIdx2GroupId[MAC_TXQ_AC13_INDEX] = GROUP3_INDEX;
	arAcQIdx2GroupId[MAC_TXQ_AC23_INDEX] = GROUP3_INDEX;
	arAcQIdx2GroupId[MAC_TXQ_AC33_INDEX] = GROUP3_INDEX;
	arAcQIdx2GroupId[MAC_TXQ_ALTX_0_INDEX] = GROUP4_INDEX;
	arAcQIdx2GroupId[MAC_TXQ_BMC_0_INDEX] = GROUP4_INDEX;
	arAcQIdx2GroupId[MAC_TXQ_BCN_0_INDEX] = GROUP4_INDEX;
	arAcQIdx2GroupId[MAC_TXQ_PSMP_0_INDEX] = GROUP4_INDEX;

	/*
	* Enable refill control group 0, 1, 2, 3, 4.
	* Keep all group low refill priority to prevent low group starvation if we have high group.
	*/
	u4MacVal = (CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP5_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP6_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP7_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP8_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP9_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP10_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP11_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP12_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP13_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP14_REFILL_DISABLE_MASK |
				CONN_HIF_DMASHDL_TOP_REFILL_CONTROL_GROUP15_REFILL_DISABLE_MASK);

	asicCmmDmaShdlInit(prAdapter, u4MacVal);

	/*
	* HIF Scheduler Setting
	* Group15(CMD) is highest priority.
	*/
	HAL_MCR_WR(prAdapter, CONN_HIF_DMASHDL_SHDL_SET0(u4BaseAddr), 0x6501234f);
	HAL_MCR_WR(prAdapter, CONN_HIF_DMASHDL_SHDL_SET1(u4BaseAddr), 0xedcba987);
}

VOID fillUsbHifTxDesc(IN PUINT_8 * pDest, IN PUINT_16 pInfoBufLen)
{
	/*USB TX Descriptor (4 bytes)*/
	/*BIT[15:0] - TX Bytes Count (Not including USB TX Descriptor and 4-bytes zero padding.*/
	kalMemZero((PVOID)*pDest, sizeof(UINT_32));
	kalMemCopy((PVOID)*pDest, (PVOID) pInfoBufLen, sizeof(UINT_16));
}
#endif /* _HIF_USB */
