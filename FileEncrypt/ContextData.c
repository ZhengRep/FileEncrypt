#include "ContextData.h"
#include "CommonSetting.h"

NTSTATUS
FindOrCreateStreamContext(
	__in PFLT_CALLBACK_DATA				Data,
	__in PFLT_RELATED_OBJECTS			FltObjects,
	__in BOOLEAN						CreateIfNotFound,
	__deref_out PSTREAM_CONTEXT*		StreamContextParamater,
	__out_opt PBOOLEAN					IsContextCreated)
{
	PAGED_CODE();

	NTSTATUS Status = STATUS_SUCCESS;
	*StreamContextParamater = NULL;
	if (IsContextCreated != NULL) 
		*IsContextCreated = FALSE;

	//ËÑË÷
	PSTREAM_CONTEXT StreamContext;
	Status = FltGetStreamContext(Data->Iopb->TargetInstance, Data->Iopb->TargetFileObject, &StreamContext);

	if (!NT_SUCCESS(Status) && (Status == STATUS_NOT_FOUND) && CreateIfNotFound)
	{
		Status = CreateStreamContext(FltObjects, &StreamContext);
		if (!NT_SUCCESS(Status))
			return Status;

		PSTREAM_CONTEXT OldStreamContext;
		Status = FltSetStreamContext(Data->Iopb->TargetInstance, Data->Iopb->TargetFileObject, FLT_SET_CONTEXT_KEEP_IF_EXISTS, StreamContext, &OldStreamContext);

		if (!NT_SUCCESS(Status))
		{
			FltReleaseContext(StreamContext);

			if (Status != STATUS_FLT_CONTEXT_ALREADY_DEFINED)
			{
				return Status;
			}
			//´ÅÅÌ¼äÃÜÎÄ¿½±´
			StreamContext = OldStreamContext;
			Status = STATUS_SUCCESS;
		}
		else
		{
			if (IsContextCreated != NULL) *IsContextCreated = TRUE;
		}
	}
	*StreamContextParamater = StreamContext;

	return Status;
}

NTSTATUS
CreateStreamContext(
	__in PFLT_RELATED_OBJECTS			FltObjects,
	__deref_out PSTREAM_CONTEXT*		StreamContext)
{
	PAGED_CODE();

	PSTREAM_CONTEXT StreamContextLocal;
	NTSTATUS Status = FltAllocateContext(FltObjects->Filter, FLT_STREAM_CONTEXT, sizeof(STREAM_CONTEXT), NonPagedPool, &StreamContextLocal);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	RtlZeroMemory(StreamContextLocal, sizeof(STREAM_CONTEXT));
	StreamContextLocal->Resource = ExAllocatePool(NonPagedPool, sizeof(ERESOURCE));
	if (StreamContextLocal->Resource == NULL)
	{
		FltReleaseContext(StreamContextLocal);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	ExInitializeResourceLite(StreamContextLocal->Resource);
	KeInitializeSpinLock(&(StreamContextLocal->SpinLock));
	*StreamContext = StreamContextLocal;

	return Status;
}

NTSTATUS
UpdateNameInStreamContext(
	__in PUNICODE_STRING				Name,
	__inout PSTREAM_CONTEXT				StreamContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	return status;
}

VOID _Lock(PSTREAM_CONTEXT StreamContext, PKIRQL Irql)
{
}

VOID _Unlock(PSTREAM_CONTEXT StreamContext, KIRQL Irql)
{
}
