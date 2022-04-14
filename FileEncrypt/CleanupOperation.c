#include "CleanupOperation.h"
#include "CommonVariable.h"
#include "ContextData.h"
#include "ProcessHelper.h"
#include "CacheHelper.h"

FLT_PREOP_CALLBACK_STATUS PreviousCleanupCallback(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
	PAGED_CODE();
	NTSTATUS Status;
	PVOLUME_CONTEXT VolumeContext;
	PFLT_FILE_NAME_INFORMATION FltFileNameInfo;
	BOOLEAN IsSpecialProcess;
	__try
	{
		Status = FltGetVolumeContext(FltObjects->Filter, FltObjects->Volume, &VolumeContext);
		if (!NT_SUCCESS(Status) || (NULL == VolumeContext))
		{
			__leave;
		}

		Status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FltFileNameInfo);
		if (!NT_SUCCESS(Status))
		{
			__leave;
		}
		if (0 != FltFileNameInfo->Name.Length)
		{
			BOOLEAN IsDirectory;
			GetFileStandardInformation(Data, FltObjects, NULL, NULL, &IsDirectory);
			if (IsDirectory)
			{
				__leave;
			}

			if (!IsCurrentProcessMonitored(FltFileNameInfo->Name.Buffer, FltFileNameInfo->Name.Length / sizeof(WCHAR), &IsSpecialProcess, NULL))
			{
				ClearFileCache(FltObjects->FileObject, TRUE, NULL, 0);

			}
		}

		if (IsSpecialProcess != EXCEL_PROCESS)
		{
			ClearFileCache(FltObjects->FileObject, TRUE, NULL, 0);
		}
	}
	__finally
	{
		if (NULL != VolumeContext)
			FltReleaseContext(VolumeContext);

		if (NULL != FltFileNameInfo)
			FltReleaseFileNameInformation(FltFileNameInfo);
	}

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}
