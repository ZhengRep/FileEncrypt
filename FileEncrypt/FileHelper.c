#include "FileHelper.h"
#include "CommonVariable.h"
#include "HandleIo.h"
#include "CommonVariable.h"

#define FILE_FLAG_POOL_TAG 'FASV'

//////////////////////////////////////////////////////////////////////////
UCHAR				__Signature[SIGNATURE_LENGTH] = { 0x3a, 0x8a, 0x2c, 0xd1, 0x50, 0xe8, 0x47, 0x5f, \
													  0xbe, 0xdb, 0xd7, 0x6c, 0xa2, 0xe9, 0x8e, 0x1d };

NTSTATUS InitializeEncryptHeader()
{
	if (NULL != __EncryptHeader)
	{
		return STATUS_SUCCESS;
	}

	__EncryptHeader = (PENCRYPT_HEADER)ExAllocatePool(NonPagedPool, sizeof(ENCRYPT_HEADER));
	if (NULL == __EncryptHeader)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlZeroMemory(__EncryptHeader, sizeof(ENCRYPT_HEADER));
	RtlCopyMemory(__EncryptHeader->Signature, __Signature, SIGNATURE_LENGTH);

	return STATUS_SUCCESS;
}

NTSTATUS GetFileStandardInformation(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects, PLARGE_INTEGER FileAllocationSize, PLARGE_INTEGER FileSize, PBOOLEAN IsDirectory)
{
	NTSTATUS Status = STATUS_SUCCESS;

	FILE_STANDARD_INFORMATION FileStandardInfo;
	Status = FltQueryInformationFile(FltObjects->Instance, FltObjects->FileObject, &FileStandardInfo, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation, NULL );
	if (NT_SUCCESS(Status))
	{
		if (NULL != FileSize)
			*FileSize = FileStandardInfo.EndOfFile;
		if (NULL != FileAllocationSize)
			*FileAllocationSize = FileStandardInfo.AllocationSize;
		if (NULL != IsDirectory)
			*IsDirectory = FileStandardInfo.Directory;
	}

	return Status;
}

NTSTATUS GetFileOffset(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects, PLARGE_INTEGER FileOffset)
{
	NTSTATUS Status;

	FILE_POSITION_INFORMATION FilePositionInfo;
	Status = FltQueryInformationFile(FltObjects->Instance, FltObjects->FileObject, &FilePositionInfo, sizeof(FILE_POSITION_INFORMATION), FilePositionInformation, NULL
	);
	if (NT_SUCCESS(Status))
	{
		FileOffset->QuadPart = FilePositionInfo.CurrentByteOffset.QuadPart;
	}

	return Status;
}

NTSTATUS SetFileOffset(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects, PLARGE_INTEGER FileOffset)
{
	NTSTATUS Status;

	FILE_POSITION_INFORMATION FilePositionInfo;
	LARGE_INTEGER v1 = { 0 };

	v1.QuadPart = FileOffset->QuadPart;
	v1.LowPart = FileOffset->LowPart;

	FilePositionInfo.CurrentByteOffset = v1;

	Status = FltSetInformationFile(FltObjects->Instance, FltObjects->FileObject, &FilePositionInfo, sizeof(FILE_POSITION_INFORMATION), FilePositionInformation );
	return Status;
}

NTSTATUS GetFileLength(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects, PLARGE_INTEGER FileLength)
{
	NTSTATUS Status;

	FILE_STANDARD_INFORMATION FileStandardInfo;
	//修改为向下层Call
	Status = FltQueryInformationFile(FltObjects->Instance, FltObjects->FileObject, &FileStandardInfo, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation, NULL );
	if (NT_SUCCESS(Status))
	{
		FileLength->QuadPart = FileStandardInfo.EndOfFile.QuadPart;
	}
	else
	{
		FileLength->QuadPart = 0;
	}

	return Status;
}

NTSTATUS WriteEncryptHeader(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects, PFILE_OBJECT FileObject, PSTREAM_CONTEXT StreamContext)
{
	NTSTATUS Status;
	__try 
	{
		//获取文件的写入的数据原始长度
		LARGE_INTEGER FileLength;
		GetFileLength(Data, FltObjects, &FileLength);

		//动态申请加密信息
#define ENCRYPT_HEADER_LENGTH sizeof(ENCRYPT_HEADER)
		PENCRYPT_HEADER EncryptHeader = (PENCRYPT_HEADER)ExAllocatePool(NonPagedPool, ENCRYPT_HEADER_LENGTH);
		if (NULL == EncryptHeader)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}
		RtlCopyMemory(EncryptHeader, __Signature, SIGNATURE_LENGTH);

		//初始化加密信息
		EncryptHeader->FileLength = StreamContext->FileLength.QuadPart;
		FileLength.QuadPart = StreamContext->FileLength.QuadPart;

		if (FileLength.QuadPart % MAX_LENGTH)
		{
			FileLength.QuadPart = FileLength.QuadPart + (ENCRYPT_HEADER_LENGTH - FileLength.QuadPart % ENCRYPT_HEADER_LENGTH) + ENCRYPT_HEADER_LENGTH;
		}
		else
		{
			FileLength.QuadPart += ENCRYPT_HEADER_LENGTH;
		}

		//定位在文件的什么位置写入加密信息
		LARGE_INTEGER ByteOffset;
		ByteOffset.QuadPart = FileLength.QuadPart - ENCRYPT_HEADER_LENGTH;

		//向文件只写入加密信息
		ULONG ReturnLength;
		Status = HandleIO(IRP_MJ_WRITE, FltObjects->Instance, FileObject, &ByteOffset, EncryptHeader, ENCRYPT_HEADER_LENGTH, &ReturnLength, FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET);
	}
	__finally 
	{

	}

	return Status;
}

NTSTATUS UpdateEncryptHeader(PFLT_CALLBACK_DATA Data, PFLT_RELATED_OBJECTS FltObjects, PFILE_OBJECT FileObject, PSTREAM_CONTEXT StreamContext, PVOLUME_CONTEXT VolumeContext)
{
	NTSTATUS Status;
	PENCRYPT_HEADER EncryptHeader;
	PUCHAR VirutalAddress;
	__try 
	{
		ULONG ViewSize = 1024*64;
		if ((ViewSize % VolumeContext->SectorSize) != 0)
		{
#define ERR_CORE_LENGTH_NOT_ALIGNED  ((ULONG)0xE000000FL)
			Status = ERR_CORE_LENGTH_NOT_ALIGNED;
			__leave;
		}

		VirutalAddress = FltAllocatePoolAlignedWithTag(FltObjects->Instance, PagedPool, ViewSize, FILE_FLAG_POOL_TAG);
		if (!VirutalAddress)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}


		EncryptHeader = (PENCRYPT_HEADER)ExAllocatePool(NonPagedPool, ENCRYPT_HEADER_LENGTH);
		if (NULL == EncryptHeader)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}
		RtlCopyMemory(EncryptHeader, __Signature, SIGNATURE_LENGTH);

		LARGE_INTEGER FileLength;
		GetFileLength(Data, FltObjects, &FileLength);
		EncryptHeader->FileLength = FileLength.QuadPart;
		if (FileLength.QuadPart % MAX_LENGTH)
		{
			FileLength.QuadPart = FileLength.QuadPart + (MAX_LENGTH - FileLength.QuadPart % MAX_LENGTH) + SIGNATURE_LENGTH;
		}
		else
		{
			FileLength.QuadPart += SIGNATURE_LENGTH;
		}

		ULONG WriteReturnLength;
		LARGE_INTEGER ReadWriteOffset;
		while (TRUE)
		{
			//从头获取文件的原始数据
			ULONG ReadReturnLength;
			Status = HandleIO(IRP_MJ_READ, FltObjects->Instance, FileObject, &ReadWriteOffset, VirutalAddress, ViewSize, &ReadReturnLength, FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET);
			if (!NT_SUCCESS(Status))
			{
				break;
			}

			if (0 == ReadReturnLength)
			{
				break;
			}

			KIRQL Irql;
			if (KeGetCurrentIrql() > PASSIVE_LEVEL)
				ExAcquireSpinLock(&VolumeContext->SpinLock, &Irql);
			else
				ExAcquireFastMutex(&VolumeContext->FastMutex);

			if (KeGetCurrentIrql() > PASSIVE_LEVEL)
				ExReleaseSpinLock(&VolumeContext->SpinLock, Irql);
			else
				ExReleaseFastMutex(&VolumeContext->FastMutex);

			BOOLEAN IsEndOfFile;
			if (ReadReturnLength < ViewSize)
				IsEndOfFile = TRUE;

			Status = HandleIO(IRP_MJ_WRITE, FltObjects->Instance, FileObject, &ReadWriteOffset, VirutalAddress, ReadReturnLength, &WriteReturnLength, FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET);
			if (!NT_SUCCESS(Status))
				break;

			if (IsEndOfFile)
				break;

			ULONG Offset = 0;
			Offset += ViewSize;
			ReadWriteOffset .QuadPart += ViewSize;
			RtlZeroMemory(VirutalAddress, ViewSize);
		}

		ReadWriteOffset.QuadPart = FileLength.QuadPart - ENCRYPT_HEADER_LENGTH;
		HandleIO(IRP_MJ_WRITE, FltObjects->Instance, FileObject, &ReadWriteOffset, EncryptHeader, ENCRYPT_HEADER_LENGTH, &WriteReturnLength, FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET);
	}
	__finally 
	{
		if (VirutalAddress)
		{
			FltFreePoolAlignedWithTag(FltObjects->Instance, VirutalAddress, FILE_FLAG_POOL_TAG);
			VirutalAddress = NULL;
		}

		if (EncryptHeader)
		{
			ExFreePool(EncryptHeader);
			EncryptHeader = NULL;
		}
	}

	return Status;
}

