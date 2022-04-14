#pragma once
#include <fltKernel.h>

FLT_PREOP_CALLBACK_STATUS PreviousWriteCallback(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);

FLT_POSTOP_CALLBACK_STATUS
PostWriteCallback(
	__inout			PFLT_CALLBACK_DATA			Data,
	__in			PCFLT_RELATED_OBJECTS		FltObjects,
	__in			PVOID						CompletionContext,
	__in			FLT_POST_OPERATION_FLAGS	Flags
);