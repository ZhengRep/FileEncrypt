#include "QuerySetInfoOperation.h"
#include "CommonSetting.h"
#include "ContextData.h"
#include "EncryptEngine.h"
#include "ProcessHelper.h"

FLT_PREOP_CALLBACK_STATUS PreviousQueryInfoCallback(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
	FLT_PREOP_CALLBACK_STATUS Status911 = FLT_PREOP_SYNCHRONIZE;
	PSTREAM_CONTEXT StreamContext;
	__try
	{
		if (!FltIsOperationSynchronous(Data))
		{
			Status911 = FLT_PREOP_SUCCESS_WITH_CALLBACK;
		}

		if (FLT_IS_FASTIO_OPERATION(Data))
		{
			Status911 = FLT_PREOP_DISALLOW_FASTIO;
			__leave;
		}

		NTSTATUS Status = FindOrCreateStreamContext(Data, FltObjects, FALSE, &StreamContext, NULL);
		if (!NT_SUCCESS(Status))
		{
			Status911 = FLT_PREOP_SUCCESS_NO_CALLBACK;
			__leave;
		}

		if (!StreamContext->IsFileCrypt)
		{
			Status911 = FLT_PREOP_SUCCESS_NO_CALLBACK;
			__leave;
		}

		//获得请求原因
		FILE_INFORMATION_CLASS FileInformationClass = Data->Iopb->Parameters.QueryFileInformation.FileInformationClass;
		if (FileInformationClass == FileBasicInformation || FileInformationClass == FileAllInformation || FileInformationClass == FileAllocationInformation ||
			FileInformationClass == FileEndOfFileInformation || FileInformationClass == FileStandardInformation || FileInformationClass == FilePositionInformation || FileInformationClass == FileValidDataLengthInformation)
		{
			Status911 = FLT_PREOP_SUCCESS_WITH_CALLBACK;   //下发该请求并最后调用Post
		}
		else
		{
			Status911 = FLT_PREOP_SUCCESS_NO_CALLBACK;
		}
	}
	__finally 
	{
		if (NULL != StreamContext)
			FltReleaseContext(StreamContext);
	}

	return Status911;
}

FLT_POSTOP_CALLBACK_STATUS PostQueryInfoCallback(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags)
{
	PSTREAM_CONTEXT StreamContext;
	FLT_POSTOP_CALLBACK_STATUS Status911 = FLT_POSTOP_FINISHED_PROCESSING;
	NTSTATUS Status = FindOrCreateStreamContext(Data, FltObjects, FALSE, &StreamContext, NULL);
	if (!NT_SUCCESS(Status))
	{
		return Status911;
	}

	if (!IsCurrentProcessMonitored(StreamContext->FileName.Buffer, StreamContext->FileName.Length / sizeof(WCHAR), NULL, NULL))
	{
		FltReleaseContext(StreamContext);
		return Status911;
	}
	//获取文件大小的请求  
	FILE_INFORMATION_CLASS FileInformationClass = Data->Iopb->Parameters.QueryFileInformation.FileInformationClass;
	PVOID InfoBuffer = Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
	ULONG BufferLength = Data->IoStatus.Information;
	switch (FileInformationClass)
	{
	case FileAllInformation:
	{
		PFILE_ALL_INFORMATION FileAllInfo = (PFILE_ALL_INFORMATION)InfoBuffer;
		if (BufferLength >= (sizeof(FILE_BASIC_INFORMATION) + sizeof(FILE_STANDARD_INFORMATION)))
		{
			//减掉文件中的加密信息的大小
			FileAllInfo->StandardInformation.EndOfFile = StreamContext->FileLength;
			FileAllInfo->StandardInformation.AllocationSize.QuadPart = StreamContext->FileLength.QuadPart + (PAGE_SIZE - (StreamContext->FileLength.QuadPart % PAGE_SIZE));
		}
		break;
	}
	case FileAllocationInformation:
	{
		PFILE_ALLOCATION_INFORMATION FileAllocInfo = (PFILE_ALLOCATION_INFORMATION)InfoBuffer;
		FileAllocInfo->AllocationSize = StreamContext->FileLength;
		break;
	}
	case FileValidDataLengthInformation:
	{
		PFILE_VALID_DATA_LENGTH_INFORMATION FileValidLengthInfo = (PFILE_VALID_DATA_LENGTH_INFORMATION)InfoBuffer;
		break;
	}
	case FileStandardInformation:
	{
		PFILE_STANDARD_INFORMATION FileStandardInfo = (PFILE_STANDARD_INFORMATION)InfoBuffer;
		FileStandardInfo->AllocationSize.QuadPart = StreamContext->FileLength.QuadPart + (PAGE_SIZE - (StreamContext->FileLength.QuadPart % PAGE_SIZE));
		FileStandardInfo->EndOfFile = StreamContext->FileLength;
		break;
	}
	case FileEndOfFileInformation:
	{
		PFILE_END_OF_FILE_INFORMATION FileEndOfFileInfo = (PFILE_END_OF_FILE_INFORMATION)InfoBuffer;
		FileEndOfFileInfo->EndOfFile = StreamContext->FileLength;
		break;
	}
	case FilePositionInformation:
	{
		PFILE_POSITION_INFORMATION FilePositionInfo = (PFILE_POSITION_INFORMATION)InfoBuffer;
		break;
	}
	case FileBasicInformation:
	{
		PFILE_BASIC_INFORMATION FileBasicInfo = (PFILE_BASIC_INFORMATION)InfoBuffer;
		break;
	}
	default:
		ASSERT(FALSE);
	};

	FltReleaseContext(StreamContext);

	return  Status911;
}

FLT_PREOP_CALLBACK_STATUS PreviousSetInfoCallback(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
	PAGED_CODE();
	NTSTATUS Status;
	FLT_PREOP_CALLBACK_STATUS Status911 = FLT_PREOP_SUCCESS_WITH_CALLBACK;
	PSTREAM_CONTEXT StreamContext;
	__try 
	{
		if (FLT_IS_FASTIO_OPERATION(Data))
		{
			Status911 = FLT_PREOP_DISALLOW_FASTIO;
			__leave;
		}

		Status = FindOrCreateStreamContext(Data, FltObjects, FALSE, &StreamContext, NULL);
		if (!NT_SUCCESS(Status))
		{
			Status911 = FLT_PREOP_SUCCESS_NO_CALLBACK;
			__leave;
		}

		if (!IsCurrentProcessMonitored(StreamContext->FileName.Buffer, StreamContext->FileName.Length / sizeof(WCHAR), NULL, NULL))
		{
			Status911 = FLT_PREOP_SUCCESS_NO_CALLBACK;
			__leave;
		}

		FILE_INFORMATION_CLASS FileInformationClass;
		if (!StreamContext->EncryptOnWrite && ((FileInformationClass != FileDispositionInformation) && (FileInformationClass != FileRenameInformation))) 
		{
			Status911 = FLT_PREOP_SUCCESS_NO_CALLBACK;
			__leave;
		}

		if (FileInformationClass == FileAllInformation || FileInformationClass == FileAllocationInformation || FileInformationClass == FileEndOfFileInformation ||
			FileInformationClass == FileStandardInformation || FileInformationClass == FilePositionInformation || FileInformationClass == FileValidDataLengthInformation ||
			FileInformationClass == FileDispositionInformation || FileInformationClass == FileRenameInformation)
		{
			Status911 = FLT_PREOP_SYNCHRONIZE;
		}
		else
		{
			Status911 = FLT_PREOP_SUCCESS_NO_CALLBACK;
			__leave;
		}

		KIRQL Irql;
		PVOID BufferData = Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
		switch (FileInformationClass)
		{
		case FileAllInformation:
		{
			PFILE_ALL_INFORMATION FileAllInfo = (PFILE_ALL_INFORMATION)BufferData;
			break;
		}
		case FileAllocationInformation:
		{
			PFILE_ALLOCATION_INFORMATION FileAllocationInfo = (PFILE_ALLOCATION_INFORMATION)BufferData;
			break;
		}
		case FileEndOfFileInformation:
		{
			PFILE_END_OF_FILE_INFORMATION FileEndOfFileInfo = (PFILE_END_OF_FILE_INFORMATION)BufferData;
			_Lock(StreamContext, &Irql);
			StreamContext->FileLength = FileEndOfFileInfo->EndOfFile;
			_Unlock(StreamContext, Irql);
			break;
		}
		case FileStandardInformation:
		{
			PFILE_STANDARD_INFORMATION FileStandardInfo = (PFILE_STANDARD_INFORMATION)BufferData;
			break;
		}
		case FilePositionInformation:
		{
			PFILE_POSITION_INFORMATION FilePositionInfo = (PFILE_POSITION_INFORMATION)BufferData;
			break;
		}
		case FileValidDataLengthInformation:
		{
			PFILE_VALID_DATA_LENGTH_INFORMATION FileValidDataLengthInfo =
				(PFILE_VALID_DATA_LENGTH_INFORMATION)BufferData;
			break;
		}
		case FileRenameInformation:
		{
			PFILE_RENAME_INFORMATION FileRenameInfo = (PFILE_RENAME_INFORMATION)BufferData;
			break;
		}
		case FileDispositionInformation:
		{
			PFILE_DISPOSITION_INFORMATION FileDispositionInfo = (PFILE_DISPOSITION_INFORMATION)BufferData;
			break;
		}
		default:
			ASSERT(FALSE);
		};
	}
	__finally 
	{
		if (NULL != StreamContext)
		{
			FltReleaseContext(StreamContext);
		}
	}

	return Status911;
}

FLT_POSTOP_CALLBACK_STATUS PostSetInfoCallback(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags)
{
	PAGED_CODE();

	PSTREAM_CONTEXT StreamContext;
	PFLT_FILE_NAME_INFORMATION FileNameInfo;
	__try 
	{
		NTSTATUS Status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);
		if (!NT_SUCCESS(Status))
		{
			__leave;
		}
		if (0 == FileNameInfo->Name.Length)
		{
			__leave;
		}

		if (0 == RtlCompareUnicodeString(&FileNameInfo->Name, &FileNameInfo->Volume, TRUE))
		{
			__leave;
		}

		Status = FindOrCreateStreamContext(Data, FltObjects, FALSE, &StreamContext, NULL);
		if (!NT_SUCCESS(Status))
		{
			__leave;
		}

		//重命名
		Status = UpdateNameInStreamContext(&FileNameInfo->Name, StreamContext);   //更新我们为其xx对象的数据结构
		if (!NT_SUCCESS(Status))
		{
			__leave;
		}
	}
	__finally 
	{
		if (NULL != StreamContext)
		{
			FltReleaseContext(StreamContext);
		}
		if (NULL != FileNameInfo)
		{
			FltReleaseFileNameInformation(FileNameInfo);
		}
	}
	return FLT_POSTOP_FINISHED_PROCESSING;
}
