#pragma once
#include<fltKernel.h>

FLT_PREOP_CALLBACK_STATUS
PreviousDirectoryControl(
	__inout				PFLT_CALLBACK_DATA				Data,
	__in				PCFLT_RELATED_OBJECTS			FltObjects,
	__deref_out_opt		PVOID*							CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
PostDirectoryControl(
	__inout				PFLT_CALLBACK_DATA				Data,
	__in				PCFLT_RELATED_OBJECTS			FltObjects,
	__in				PVOID							CompletionContext,
	__in				FLT_POST_OPERATION_FLAGS		Flags
);

FLT_POSTOP_CALLBACK_STATUS
PostDirectoryControlSafe(
	__inout				PFLT_CALLBACK_DATA				Data,
	__in				PCFLT_RELATED_OBJECTS			FltObjects,
	__in				PVOID							CompletionContext,
	__in				FLT_POST_OPERATION_FLAGS		Flags
);