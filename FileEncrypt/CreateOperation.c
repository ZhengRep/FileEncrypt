#include "CreateOperation.h"
#include "ContextData.h"
#include "FileHelper.h"
#include "CommonVariable.h"
#include "HandleIO.h"
#include "ProcessHelper.h"

FLT_PREOP_CALLBACK_STATUS PreviousCreateCallback(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, _Flt_CompletionContext_Outptr_ PVOID* CompletionContext)
{
    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS PostCreateCallback(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags)
{
	PAGED_CODE();
	NTSTATUS Status;
	PVOLUME_CONTEXT VolumeContext;
	PSTREAM_CONTEXT StreamContext;
	PENCRYPT_HEADER EncryptHeader;
	PFLT_FILE_NAME_INFORMATION FltFileNameInfo;
	__try 
	{
		if (!NT_SUCCESS(Data->IoStatus.Status))
		{
			__leave;
		}

		Status = FltGetVolumeContext(FltObjects->Filter, FltObjects->Volume, &VolumeContext);   //����������ڴ� ����Ҫ���ǻ���
		if (!NT_SUCCESS(Status) || (NULL == VolumeContext))
		{
			__leave;
		}
		Status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FltFileNameInfo);

		if (!NT_SUCCESS(Status))
		{
			__leave;
		}
		if (0 == FltFileNameInfo->Name.Length)
		{
			__leave;   //�򿪻��ߴ�����һ�����ļ�
		}

		if (0 == RtlCompareUnicodeString(&FltFileNameInfo->Name, &FltFileNameInfo->Volume, TRUE))
		{
			__leave;
		}

		//�ж��Ƿ���Ŀ¼
		BOOLEAN IsDirectory;
		GetFileStandardInformation(Data, FltObjects, NULL, NULL, &IsDirectory);
		if (IsDirectory)
		{
			__leave;
		}

		BOOLEAN IsMonitoredProcess;
		if (!IsCurrentProcessMonitored(FltFileNameInfo->Name.Buffer, FltFileNameInfo->Name.Length / sizeof(WCHAR), &IsMonitoredProcess, NULL))
		{
			__leave;
		}

		//��ط�Χ
		//�½�һ���ļ����ݽṹ
		BOOLEAN IsContextCreated;
		Status = FindOrCreateStreamContext(Data, FltObjects, TRUE, &StreamContext, &IsContextCreated);
		//StreamContext �������� 
		//StreamContext �������
		if (!NT_SUCCESS(Status))
		{
			__leave;
		}
		//�����ļ�����������
		UpdateNameInStreamContext(&FltFileNameInfo->Name, StreamContext);

		KIRQL Irql;
		ULONG DesiredAccess = Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;
		if (!IsContextCreated)
		{
			//StreamContext����������
			_Lock(StreamContext, &Irql);
			StreamContext->ReferenceCount++;
			StreamContext->Access = DesiredAccess;
			_Unlock(StreamContext, Irql);
			__leave;
		}
		//Create
		_Lock(StreamContext, &Irql);
		RtlCopyMemory(StreamContext->VolumeName, FltFileNameInfo->Volume.Buffer, FltFileNameInfo->Volume.Length);

		StreamContext->IsFileCrypt = FALSE;
		StreamContext->DecryptOnRead = FALSE;
		StreamContext->EncryptOnWrite = TRUE;
		StreamContext->HasWriteData = FALSE;
		StreamContext->HasPPTWriteData = FALSE;
		StreamContext->ReferenceCount++;
		StreamContext->Access = DesiredAccess;
		StreamContext->EncryptHeaderLength = sizeof(ENCRYPT_HEADER);
		_Unlock(StreamContext, Irql);

		//��ȡ�ļ���С
		LARGE_INTEGER FileLength;
		Status = GetFileStandardInformation(Data, FltObjects, NULL, &FileLength, NULL);
		if (!NT_SUCCESS(Status))
		{
			__leave;
		}

		_Lock(StreamContext, &Irql);
		StreamContext->FileLength = FileLength;
		_Unlock(StreamContext, Irql);

		if ((0 == FileLength.QuadPart) && (DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)))
		{
			__leave;
		}
		if ((0 == FileLength.QuadPart) && !(DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)))
		{
			__leave;
		}


		if (FileLength.QuadPart < sizeof(ENCRYPT_HEADER))
		{
			__leave;   //������ ֱ�ӷ����û�
		}

		//HelloWorld[Shit]14
		LARGE_INTEGER OriginalByteOffset;
		GetFileOffset(Data, FltObjects, &OriginalByteOffset);   //��ȡ�˲���ԭʼ�ļ������е�����

		//�ļ���־  ˵��������
		EncryptHeader = (PENCRYPT_HEADER)ExAllocatePool(NonPagedPool, sizeof(ENCRYPT_HEADER));

		//���¶�λƫ��
		LARGE_INTEGER ByteOffset;
		ByteOffset.QuadPart = FileLength.QuadPart - sizeof(ENCRYPT_HEADER);   ////HelloWorld10[Shit]14    14-4

		//���´ӵ�ǰ�豸��������鿴�������Ƿ���ڼ���β
		ULONG ReturnLength;
		Status = HandleIO(IRP_MJ_READ, FltObjects->Instance, FltObjects->FileObject, &ByteOffset, EncryptHeader, sizeof(ENCRYPT_HEADER), &ReturnLength, FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET);
		//Status �ǵ�ǰ�豸�����Irp�Ƿ�ɹ�
		if (!NT_SUCCESS(Status))
		{
			__leave;
		}

		SetFileOffset(Data, FltObjects, &OriginalByteOffset);   //�ָ�ԭʼ�ļ������е�����

		if (SIGNATURE_LENGTH != RtlCompareMemory(__Signature, EncryptHeader, SIGNATURE_LENGTH))
		{
			//����һ�����������ܵ��ļ�
			_Lock(StreamContext, &Irql);
			StreamContext->FileLength = FileLength;
			_Unlock(StreamContext, Irql);
			__leave;
		}

		//�õ�һ�����������ܵ��ļ�
		_Lock(StreamContext, &Irql);
		StreamContext->FileLength.QuadPart = EncryptHeader->FileLength;   //���ܵĵ�ʱ����е�����
		StreamContext->IsFileCrypt = TRUE;
		StreamContext->EncryptOnWrite = TRUE;
		StreamContext->DecryptOnRead = TRUE;
		StreamContext->EncryptHeaderLength = sizeof(ENCRYPT_HEADER);
		_Unlock(StreamContext, Irql);
	}
	__finally
	{
		//�����Ƿ��쳣
		if (NULL != VolumeContext)
			FltReleaseContext(VolumeContext);

		if (NULL != FltFileNameInfo)
			FltReleaseFileNameInformation(FltFileNameInfo);

		if (NULL != StreamContext)
			FltReleaseContext(StreamContext);

		if (NULL != EncryptHeader)
			ExFreePool(EncryptHeader);
	}

	return FLT_POSTOP_FINISHED_PROCESSING;
}
