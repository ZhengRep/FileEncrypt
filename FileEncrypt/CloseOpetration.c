#include "CloseOpetration.h"
#include "CacheHelper.h"
#include "CommonVariable.h"
#include "FileHelper.h"
#include "ProcessHelper.h"

FLT_PREOP_CALLBACK_STATUS PreviousCloseCallback(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
	NTSTATUS Status;
	PVOLUME_CONTEXT VolumeContext;
	FLT_PREOP_CALLBACK_STATUS Status911 = FLT_PREOP_SUCCESS_NO_CALLBACK;
	HANDLE FileHandle;
	PFILE_OBJECT FileObject;
	PSTREAM_CONTEXT StreamContext;
	BOOLEAN IsSpecialProcess;
	BOOLEAN IsDelete = TRUE;
	__try
	{
		BOOLEAN IsDirectory;
		GetFileStandardInformation(Data, FltObjects, NULL, NULL, &IsDirectory);
		if (IsDirectory)
		{
			__leave;
		}

		Status = FltGetVolumeContext(FltObjects->Filter, FltObjects->Volume, &VolumeContext);
		if (!NT_SUCCESS(Status))
		{
			__leave;
		}

		Status = FindOrCreateStreamContext(Data, FltObjects, FALSE, &StreamContext, NULL);
		if (!NT_SUCCESS(Status))
		{
			__leave;
		}

		if (!IsCurrentProcessMonitored(StreamContext->FileName.Buffer, StreamContext->FileName.Length / sizeof(WCHAR), &IsSpecialProcess, NULL))
		{
			__leave;
		}

		KIRQL Irql;
		_Lock(StreamContext, &Irql);

		if ((FltObjects->FileObject->Flags & FO_STREAM_FILE) != FO_STREAM_FILE) 
			StreamContext->ReferenceCount--;

		if (0 == StreamContext->ReferenceCount)
		{

			if ((StreamContext->HasWriteData || StreamContext->IsFileCrypt) || (!StreamContext->HasWriteData && !StreamContext->IsFileCrypt && StreamContext->HasPPTWriteData))
			{
				_Unlock(StreamContext, Irql);

				WCHAR FilePathName[256];
				RtlCopyMemory(FilePathName, StreamContext->FileName.Buffer, StreamContext->FileName.Length);

				WCHAR VolumePathName[64];
				RtlCopyMemory(VolumePathName, StreamContext->VolumeName, wcslen(StreamContext->VolumeName) * sizeof(WCHAR));
				PWCHAR DeviceFilePath = wcsstr(FilePathName, VolumePathName);

				//"\Device\HarddiskVolume1"
				if (!DeviceFilePath)
				{
					__leave;
				}
				DeviceFilePath = DeviceFilePath + wcslen(StreamContext->VolumeName);
				WCHAR FileDosFullPath[256];
				wcscpy(FileDosFullPath, L"\\??\\");
				RtlCopyMemory(FileDosFullPath + 4, VolumeContext->VolumeName.Buffer, VolumeContext->VolumeName.Length);
				RtlCopyMemory(FileDosFullPath + 4 + VolumeContext->VolumeName.Length / sizeof(WCHAR), DeviceFilePath, StreamContext->FileName.Length - wcslen(StreamContext->VolumeName) * sizeof(WCHAR));
				UNICODE_STRING unFileDosPath;
				RtlInitUnicodeString(&unFileDosPath, FileDosFullPath);

				OBJECT_ATTRIBUTES ObjectAttributes;
				InitializeObjectAttributes(&ObjectAttributes, &unFileDosPath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

				IO_STATUS_BLOCK IoStatusBlock;
				Status = FltCreateFile(FltObjects->Filter, FltObjects->Instance, &FileHandle, FILE_READ_DATA | FILE_WRITE_DATA, &ObjectAttributes, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL,\
					FILE_SHARE_READ, FILE_OPEN, FILE_NON_DIRECTORY_FILE, NULL, 0, IO_IGNORE_SHARE_ACCESS_CHECK);
				if (!NT_SUCCESS(Status))
				{
					__leave;
				}

				Status = ObReferenceObjectByHandle(FileHandle, STANDARD_RIGHTS_ALL, *IoFileObjectType, KernelMode, &FileObject, NULL );
				if (!NT_SUCCESS(Status))
				{
					__leave;
				}

				if (StreamContext->HasWriteData || StreamContext->IsFileCrypt)
				{
					WriteEncryptHeader(Data, FltObjects, FileObject, StreamContext);   //第一次写入加密信息
				}
				else
				{

					UpdateEncryptHeader(Data, FltObjects, FileObject, StreamContext, VolumeContext);    //以后的写   更新加密信息
				}

				_Lock(StreamContext, &Irql);
				StreamContext->IsFileCrypt = TRUE;
				StreamContext->DecryptOnRead = TRUE;
				_Unlock(StreamContext, Irql);

				//向文件刷入真实数据
				ClearFileCache(FileObject, TRUE, NULL, 0);   //向文件写入真正数据   FCB  CCB

			}
			else
			{
				_Unlock(StreamContext, Irql);
			}
		}
		else
		{
			_Unlock(StreamContext, Irql);

		}
	}
	__finally 
	{
		if (NULL != StreamContext)
			FltReleaseContext(StreamContext);

		if (IsDelete && (IsSpecialProcess != EXCEL_PROCESS))
			FltDeleteContext(StreamContext);

		if (NULL != VolumeContext)
			FltReleaseContext(VolumeContext);

		if (NULL != FileHandle)
			FltClose(FileHandle);

		if (NULL != FileObject)
			ObDereferenceObject(FileObject);
	}

	return Status911;
}
