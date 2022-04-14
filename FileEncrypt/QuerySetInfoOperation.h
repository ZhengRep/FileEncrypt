#pragma once
#include <fltKernel.h>

FLT_PREOP_CALLBACK_STATUS
PreviousQueryInfoCallback(
	__inout			PFLT_CALLBACK_DATA			Data,
	__in			PCFLT_RELATED_OBJECTS		FltObjects,
	__deref_out_opt PVOID*						CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
PostQueryInfoCallback(
	__inout			PFLT_CALLBACK_DATA			Data,
	__in			PCFLT_RELATED_OBJECTS		FltObjects,
	__inout_opt		PVOID						CompletionContext,
	__in			FLT_POST_OPERATION_FLAGS	Flags
);

FLT_PREOP_CALLBACK_STATUS
PreviousSetInfoCallback(
	__inout			PFLT_CALLBACK_DATA			Data,
	__in			PCFLT_RELATED_OBJECTS		FltObjects,
	__deref_out_opt	PVOID*						CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
PostSetInfoCallback(
	__inout			PFLT_CALLBACK_DATA			Data,
	__in			PCFLT_RELATED_OBJECTS		FltObjects,
	__inout_opt		PVOID						CompletionContext,
	__in			FLT_POST_OPERATION_FLAGS	Flags
);

 