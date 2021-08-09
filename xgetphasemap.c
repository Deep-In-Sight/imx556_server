// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2020.2 (64-bit)
// Copyright 1986-2020 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xgetphasemap.h"
#include <poll.h>
#include <stdio.h>

//#define PHASE_DBG

/************************** Function Implementation *************************/
#ifndef __linux__
int XGetphasemap_CfgInitialize(XGetphasemap *InstancePtr, XGetphasemap_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XGetphasemap_Start(XGetphasemap *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGetphasemap_ReadReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_AP_CTRL) & 0x80;
    XGetphasemap_WriteReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XGetphasemap_IsDone(XGetphasemap *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGetphasemap_ReadReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

#define PRINT_POLL(revents, flag) if (revents & flag){printf("%s event polled\n", #flag);}
void XGetphasemap_IsDonePoll(XGetphasemap *InstancePtr, int msec_timeout) {
    u32 Data;
    static u32 Data_last = 0;
    u32 uio_fd = InstancePtr->uio_fd;
    u32 isr;

    struct pollfd fds = {
    	.fd = uio_fd,
		.events = POLLIN,
    };

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    //wait for interrupt
    int ret = poll(&fds, 1, msec_timeout);

    if (ret < 0) {
    	perror("poll() failed");
    	return;
//    	close(uio_fd);
//    	exit(EXIT_FAILURE);
    } else if (ret == 0) {
    	perror("poll(0 timed out");
    	return;
    }

#ifdef PHASE_DBG
    u16 revents = fds.revents;
    PRINT_POLL(revents, POLLIN);
    PRINT_POLL(revents, POLLERR);
    PRINT_POLL(revents, POLLHUP);
    PRINT_POLL(revents, POLLNVAL);
#endif

    ret = read(uio_fd, &Data, sizeof(Data));
    if (ret != 4) {
    	printf("UIO read failed\b=n");
    }

#ifdef PHASE_DBG
    printf("Data read: %d\n", Data);
    if (Data > Data_last) {
    	Data_last = Data;
    	printf("New interrupt...\n");
    } else {
    	printf("Weird +___+ ....\n");
    }
#endif

    isr = XGetphasemap_ReadReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_ISR);
//    printf("ISR = %d\n", isr);
    if (isr & XGETPHASEMAP_CONTROL_INTERRUPTS_DONE_MASK) {
    	//write 1 to toggle to 0
    	XGetphasemap_WriteReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_ISR, XGETPHASEMAP_CONTROL_INTERRUPTS_DONE_MASK);
    }

    //re-enable uio driver irq
    Data = 0x01;
    write(uio_fd, &Data, sizeof(u32));
}

u32 XGetphasemap_IsIdle(XGetphasemap *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGetphasemap_ReadReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XGetphasemap_IsReady(XGetphasemap *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGetphasemap_ReadReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XGetphasemap_EnableAutoRestart(XGetphasemap *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGetphasemap_WriteReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XGetphasemap_DisableAutoRestart(XGetphasemap *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGetphasemap_WriteReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_AP_CTRL, 0);
}

void XGetphasemap_Set_regCtrl(XGetphasemap *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGetphasemap_WriteReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_REGCTRL_DATA, Data);
}

u32 XGetphasemap_Get_regCtrl(XGetphasemap *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGetphasemap_ReadReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_REGCTRL_DATA);
    return Data;
}

void XGetphasemap_Set_frame02_offset(XGetphasemap *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGetphasemap_WriteReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_FRAME02_OFFSET_DATA, Data);
}

u32 XGetphasemap_Get_frame02_offset(XGetphasemap *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGetphasemap_ReadReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_FRAME02_OFFSET_DATA);
    return Data;
}

void XGetphasemap_Set_frame13_offset(XGetphasemap *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGetphasemap_WriteReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_FRAME13_OFFSET_DATA, Data);
}

u32 XGetphasemap_Get_frame13_offset(XGetphasemap *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGetphasemap_ReadReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_FRAME13_OFFSET_DATA);
    return Data;
}

void XGetphasemap_InterruptGlobalEnable(XGetphasemap *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGetphasemap_WriteReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_GIE, 1);
}

void XGetphasemap_InterruptGlobalDisable(XGetphasemap *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGetphasemap_WriteReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_GIE, 0);
}

void XGetphasemap_InterruptEnable(XGetphasemap *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XGetphasemap_ReadReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_IER);
    XGetphasemap_WriteReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_IER, Register | Mask);
}

void XGetphasemap_InterruptDisable(XGetphasemap *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XGetphasemap_ReadReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_IER);
    XGetphasemap_WriteReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_IER, Register & (~Mask));
}

void XGetphasemap_InterruptClear(XGetphasemap *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGetphasemap_WriteReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_ISR, Mask);
}

u32 XGetphasemap_InterruptGetEnabled(XGetphasemap *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XGetphasemap_ReadReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_IER);
}

u32 XGetphasemap_InterruptGetStatus(XGetphasemap *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XGetphasemap_ReadReg(InstancePtr->Control_BaseAddress, XGETPHASEMAP_CONTROL_ADDR_ISR);
}

