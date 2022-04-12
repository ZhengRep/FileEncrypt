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
	NTSTATUS status = STATUS_SUCCESS;
	return status;
}

NTSTATUS
CreateStreamContext(
	__in PFLT_RELATED_OBJECTS			FltObjects,
	__deref_out PSTREAM_CONTEXT*		StreamContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	return status;
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
