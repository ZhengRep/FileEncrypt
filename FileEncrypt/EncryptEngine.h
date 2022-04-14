#pragma once
#include <fltKernel.h>
#include "ContextData.h"
#include "CommonVariable.h"

typedef struct _PREVIOUS_2_POST_CONTEXT
{
	PVOLUME_CONTEXT			VolumeContext;
	PSTREAM_CONTEXT			StreamContext;
	PVOID					BufferData;
} PREVIOUS_2_POST_CONTEXT, * PPREVIOUS_2_POST_CONTEXT;

NTSTATUS FilterUnloadCallback(_In_ FLT_FILTER_UNLOAD_FLAGS Flags);

NTSTATUS InstanceSetupCallback(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_SETUP_FLAGS Flags, __in DEVICE_TYPE VolumeDeviceType, __in FLT_FILESYSTEM_TYPE VolumeFilesystemType);

VOID CleanupContext( __in PFLT_CONTEXT Context, __in FLT_CONTEXT_TYPE ContextType );