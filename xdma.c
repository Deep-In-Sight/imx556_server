// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2020.2 (64-bit)
// Copyright 1986-2020 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xdma.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XDma_CfgInitialize(XDma *InstancePtr, XDma_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XDma_Start(XDma *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDma_ReadReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_AP_CTRL) & 0x80;
    XDma_WriteReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XDma_IsDone(XDma *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDma_ReadReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XDma_IsIdle(XDma *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDma_ReadReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XDma_IsReady(XDma *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDma_ReadReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XDma_EnableAutoRestart(XDma *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDma_WriteReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XDma_DisableAutoRestart(XDma *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDma_WriteReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_AP_CTRL, 0);
}

void XDma_Set_s2mm_offset(XDma *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDma_WriteReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_S2MM_OFFSET_DATA, Data);
}

u32 XDma_Get_s2mm_offset(XDma *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDma_ReadReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_S2MM_OFFSET_DATA);
    return Data;
}

void XDma_Set_len(XDma *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDma_WriteReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_LEN_DATA, Data);
}

u32 XDma_Get_len(XDma *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDma_ReadReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_LEN_DATA);
    return Data;
}

void XDma_InterruptGlobalEnable(XDma *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDma_WriteReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_GIE, 1);
}

void XDma_InterruptGlobalDisable(XDma *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDma_WriteReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_GIE, 0);
}

void XDma_InterruptEnable(XDma *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XDma_ReadReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_IER);
    XDma_WriteReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_IER, Register | Mask);
}

void XDma_InterruptDisable(XDma *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XDma_ReadReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_IER);
    XDma_WriteReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_IER, Register & (~Mask));
}

void XDma_InterruptClear(XDma *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDma_WriteReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_ISR, Mask);
}

u32 XDma_InterruptGetEnabled(XDma *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XDma_ReadReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_IER);
}

u32 XDma_InterruptGetStatus(XDma *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XDma_ReadReg(InstancePtr->Control_BaseAddress, XDMA_CONTROL_ADDR_ISR);
}

