// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2020.2 (64-bit)
// Copyright 1986-2020 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef XGETPHASEMAP_H
#define XGETPHASEMAP_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/
#ifndef __linux__
#include "xil_types.h"
#include "xil_assert.h"
#include "xstatus.h"
#include "xil_io.h"
#else
#include <stdint.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>
#endif
#include "xgetphasemap_hw.h"

/**************************** Type Definitions ******************************/
#ifdef __linux__
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#else
typedef struct {
    u16 DeviceId;
    u32 Control_BaseAddress;
} XGetphasemap_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XGetphasemap;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XGetphasemap_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XGetphasemap_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XGetphasemap_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XGetphasemap_ReadReg(BaseAddress, RegOffset) \
    *(volatile u32*)((BaseAddress) + (RegOffset))

#define Xil_AssertVoid(expr)    assert(expr)
#define Xil_AssertNonvoid(expr) assert(expr)

#define XST_SUCCESS             0
#define XST_DEVICE_NOT_FOUND    2
#define XST_OPEN_DEVICE_FAILED  3
#define XIL_COMPONENT_IS_READY  1
#endif

/************************** Function Prototypes *****************************/
#ifndef __linux__
int XGetphasemap_Initialize(XGetphasemap *InstancePtr, u16 DeviceId);
XGetphasemap_Config* XGetphasemap_LookupConfig(u16 DeviceId);
int XGetphasemap_CfgInitialize(XGetphasemap *InstancePtr, XGetphasemap_Config *ConfigPtr);
#else
int XGetphasemap_Initialize(XGetphasemap *InstancePtr, const char* InstanceName);
int XGetphasemap_Release(XGetphasemap *InstancePtr);
#endif

void XGetphasemap_Start(XGetphasemap *InstancePtr);
u32 XGetphasemap_IsDone(XGetphasemap *InstancePtr);
u32 XGetphasemap_IsIdle(XGetphasemap *InstancePtr);
u32 XGetphasemap_IsReady(XGetphasemap *InstancePtr);
void XGetphasemap_EnableAutoRestart(XGetphasemap *InstancePtr);
void XGetphasemap_DisableAutoRestart(XGetphasemap *InstancePtr);

void XGetphasemap_Set_regCtrl(XGetphasemap *InstancePtr, u32 Data);
u32 XGetphasemap_Get_regCtrl(XGetphasemap *InstancePtr);
void XGetphasemap_Set_frame02_offset(XGetphasemap *InstancePtr, u32 Data);
u32 XGetphasemap_Get_frame02_offset(XGetphasemap *InstancePtr);
void XGetphasemap_Set_frame13_offset(XGetphasemap *InstancePtr, u32 Data);
u32 XGetphasemap_Get_frame13_offset(XGetphasemap *InstancePtr);

void XGetphasemap_InterruptGlobalEnable(XGetphasemap *InstancePtr);
void XGetphasemap_InterruptGlobalDisable(XGetphasemap *InstancePtr);
void XGetphasemap_InterruptEnable(XGetphasemap *InstancePtr, u32 Mask);
void XGetphasemap_InterruptDisable(XGetphasemap *InstancePtr, u32 Mask);
void XGetphasemap_InterruptClear(XGetphasemap *InstancePtr, u32 Mask);
u32 XGetphasemap_InterruptGetEnabled(XGetphasemap *InstancePtr);
u32 XGetphasemap_InterruptGetStatus(XGetphasemap *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
