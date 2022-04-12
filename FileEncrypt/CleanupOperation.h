#pragma once
#include <fltKernel.h>

FLT_PREOP_CALLBACK_STATUS
PreviousCleanupCallback(
	__inout			PFLT_CALLBACK_DATA		Data,
	__in			PCFLT_RELATED_OBJECTS	FltObjects,
	__deref_out_opt PVOID*					CompletionContext
);
