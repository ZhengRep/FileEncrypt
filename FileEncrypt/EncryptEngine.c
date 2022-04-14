#include "EncryptEngine.h"
//#include "QuerySetInfoOperation.h"
//#include "CreateOperation.h"
//#include "CloseOpetration.h"
//#include "CleanupOperation.h"
#include "WriteOperation.h"
#include "FileHelper.h"
#include "ProcessHelper.h"
#include "ReadOperation.h"
#include "CommonVariable.h"

//////////////////////////////////////////////////////////////////////////
//Macro
#define PREVIOUS_2_POST_TAG			'XXXX'

//////////////////////////////////////////////////////////////////////////
//Global Variables
ULONG						__ImageFileNameOffset;
PFLT_FILTER					__FilterHandle;
//Global Vars
LIST_ENTRY					__ListHeadOfMonitorProcess;
KSPIN_LOCK					__SpinLockOfListEntry;
PENCRYPT_HEADER				__EncryptHeader;
NPAGED_LOOKASIDE_LIST		__NpagedLookasideListOfPrevious2Post;

//CreateFile
CONST FLT_OPERATION_REGISTRATION __Callbacks[] = {

	/*{
		IRP_MJ_CREATE,
		FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
		PreviousCreateCallback,
		PostCreateCallback 
	},*/
	/*{
		IRP_MJ_CLEANUP, 
		FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
		PreviousCleanupCallback,
		NULL 
	},
	{ 
		IRP_MJ_CLOSE,
		0, 
		PreviousCloseCallback,
		NULL
	},
	{ 
		IRP_MJ_QUERY_INFORMATION,
		0,
		PreviousQueryInfoCallback,
		PostQueryInfoCallback
	},
	{ 
		IRP_MJ_SET_INFORMATION,
		0,
		PreviousSetInfoCallback,
		PostSetInfoCallback
	},
	{
		IRP_MJ_DIRECTORY_CONTROL,
		0,
		NULL,
		NULL
	},*/
	{
		IRP_MJ_READ,
		0,
		PreviousReadCallback,
		PostReadCallback
	},
	//{
	//	IRP_MJ_WRITE,
	//	0,
	//	PreviousWriteCallback,
	//	PostWriteCallback
	//},
	{ IRP_MJ_OPERATION_END }
};

//
//  This defines what we want to filter with FltMgr
//

#define VOLUME_CONTEXT_TAG       'xcBS'  
#define STREAM_CONTEXT_TAG       'cSxC'  
const FLT_CONTEXT_REGISTRATION __Contexts[] =
{
	{ FLT_VOLUME_CONTEXT, 0, CleanupContext, sizeof(VOLUME_CONTEXT), VOLUME_CONTEXT_TAG },
	{ FLT_STREAM_CONTEXT, 0, CleanupContext, sizeof(STREAM_CONTEXT), STREAM_CONTEXT_TAG },
	{ FLT_CONTEXT_END }
};


CONST FLT_REGISTRATION __FilterRegistration = {

	sizeof(FLT_REGISTRATION),         //  Size
	FLT_REGISTRATION_VERSION,           //  Version
	0,                                  //  Flags
	__Contexts,                               //  Context
	__Callbacks,                          //  Operation callbacks
	FilterUnloadCallback,
	InstanceSetupCallback,
	NULL,            //  InstanceQueryTeardown
	NULL,            //  InstanceTeardownStart
	NULL,         //  InstanceTeardownComplete
	NULL,                               //  GenerateFileName
	NULL,                               //  GenerateDestinationFileName
	NULL                                //  NormalizeNameComponent
};

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	UNREFERENCED_PARAMETER(RegistryPath);

	//��ʼ��һ���ڴ�����ڴ洢����������ڴ�
	ExInitializeNPagedLookasideList(&__NpagedLookasideListOfPrevious2Post,NULL, NULL, 0, sizeof(PREVIOUS_2_POST_CONTEXT), PREVIOUS_2_POST_TAG, 0);

	InitializeEncryptHeader();

	//��ý�������EProcess�е�ƫ��
	__ImageFileNameOffset = GetImageFileNameOffset();

	InitializeListHead(&__ListHeadOfMonitorProcess);
	KeInitializeSpinLock(&__SpinLockOfListEntry);

	//���Դ���
	AddProcessContext((PUCHAR)"System", TRUE);
	AddProcessContext((PUCHAR)"explorer.exe", TRUE);
	AddProcessContext((PUCHAR)"notepad.exe", TRUE);

	__try
	{
		//ע��MiniFilter
		Status = FltRegisterFilter(DriverObject,&__FilterRegistration, &__FilterHandle);

		if (!NT_SUCCESS(Status)) {

			__leave;
		}

		//����MiniFilter
		Status = FltStartFiltering(__FilterHandle);
	}
	__finally 
	{
		if (!NT_SUCCESS(Status)) {
			ExDeleteNPagedLookasideList(&__NpagedLookasideListOfPrevious2Post);
			if (__EncryptHeader != NULL)
			{
				ExFreePool(__EncryptHeader);
			}
		}
	}
	return Status;
}

NTSTATUS FilterUnloadCallback(FLT_FILTER_UNLOAD_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(Flags);
	PAGED_CODE();

	FltUnregisterFilter(__FilterHandle);

	ExDeleteNPagedLookasideList(&__NpagedLookasideListOfPrevious2Post);

	if (__EncryptHeader != NULL)
	{
		ExFreePool(__EncryptHeader);
	}

	return STATUS_SUCCESS;
}

//ö�����еľ�ѡ���԰�
NTSTATUS InstanceSetupCallback(PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_SETUP_FLAGS Flags, DEVICE_TYPE VolumeDeviceType, FLT_FILESYSTEM_TYPE VolumeFilesystemType)
{
	PAGED_CODE();
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(VolumeDeviceType);
	UNREFERENCED_PARAMETER(VolumeFilesystemType);

	NTSTATUS Status;
	PDEVICE_OBJECT DeviceObject = NULL;
	PVOLUME_CONTEXT  VolumeContext = NULL;
	__try 
	{
		//�����������
		Status = FltAllocateContext(FltObjects->Filter, FLT_VOLUME_CONTEXT, sizeof(VOLUME_CONTEXT), NonPagedPool, (PFLT_CONTEXT *)&VolumeContext);
		if (!NT_SUCCESS(Status))
		{
			__leave;
		}

		//��þ�����
		UCHAR BufferData[sizeof(FLT_VOLUME_PROPERTIES) + 512];
		PFLT_VOLUME_PROPERTIES FltVolumeProperties = (PFLT_VOLUME_PROPERTIES)BufferData;
		ULONG ReturnedLength;
		Status = FltGetVolumeProperties(FltObjects->Volume, FltVolumeProperties, sizeof(BufferData), &ReturnedLength);

		if (!NT_SUCCESS(Status))
		{
			__leave;
		}

		//��ʼ��VolumeContext
		VolumeContext->SectorSize = max(FltVolumeProperties->SectorSize, MAX_LENGTH);
		VolumeContext->VolumeName.Buffer = NULL;

		//��ô����豸����
		Status = FltGetDiskDeviceObject(FltObjects->Volume, &DeviceObject);

		//���µ������豸���� ��\Driver\volmgr
		if (NT_SUCCESS(Status))
		{
			Status = RtlVolumeDeviceToDosName(DeviceObject, &VolumeContext->VolumeName);
		}
		if (!NT_SUCCESS(Status))
		{
			PUNICODE_STRING DeviceName;
			if (FltVolumeProperties->RealDeviceName.Length > 0)
			{
				DeviceName = &FltVolumeProperties->RealDeviceName;
			}
			else if (FltVolumeProperties->FileSystemDeviceName.Length > 0)
			{
				DeviceName = &FltVolumeProperties->FileSystemDeviceName;
			}
			else
			{
				Status = STATUS_FLT_DO_NOT_ATTACH;
				__leave;
			}

			USHORT StrLength;
			StrLength = DeviceName->Length + sizeof(WCHAR);
			VolumeContext->VolumeName.Buffer = (PWCH)ExAllocatePool(NonPagedPool, StrLength);
			if (VolumeContext->VolumeName.Buffer == NULL)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				__leave;
			}
			//��þ�����
			VolumeContext->VolumeName.Length = 0;
			VolumeContext->VolumeName.MaximumLength = StrLength;
			RtlCopyUnicodeString(&VolumeContext->VolumeName, DeviceName);
			RtlAppendUnicodeToString(&VolumeContext->VolumeName, L":");
		}
		///������
		KeInitializeSpinLock(&VolumeContext->SpinLock);
		ExInitializeFastMutex(&VolumeContext->FastMutex);

		//������������ڴ����õ�FltObjects->Volume,
		Status = FltSetVolumeContext(FltObjects->Volume, FLT_SET_CONTEXT_KEEP_IF_EXISTS, VolumeContext, NULL);
		if (Status == STATUS_FLT_CONTEXT_ALREADY_DEFINED)
		{
			Status = STATUS_SUCCESS;
		}

	}
	__finally
	{
		if (VolumeContext)
		{
			FltReleaseContext(VolumeContext);
#if DBG			
			if ((VolumeContext->VolumeName.Buffer[0] == L'D') || (VolumeContext->VolumeName.Buffer[0] == L'd'))
			{
				FltDeleteContext(VolumeContext);          //������D������
				Status = STATUS_FLT_DO_NOT_ATTACH;        //
			}
#endif
		}
		if (DeviceObject)
		{
			ObDereferenceObject(DeviceObject);
		}
	}
	return Status;
}

VOID CleanupContext(PFLT_CONTEXT Context, FLT_CONTEXT_TYPE ContextType)
{
}