#pragma once
#include <fltKernel.h>

FLT_PREOP_CALLBACK_STATUS
PreviousCreateCallback(
	_Inout_							PFLT_CALLBACK_DATA		Data,
	_In_							PCFLT_RELATED_OBJECTS	FltObjects,
	_Flt_CompletionContext_Outptr_	PVOID*					CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
PostCreateCallback(
	_Inout_		PFLT_CALLBACK_DATA			Data,
	_In_		PCFLT_RELATED_OBJECTS		FltObjects,
	_In_		PVOID						CompletionContext,
	_In_		FLT_POST_OPERATION_FLAGS	Flags
);

