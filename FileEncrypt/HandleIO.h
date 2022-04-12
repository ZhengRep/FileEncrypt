#pragma once
#include <fltKernel.h>

NTSTATUS HandleIO(
	__in ULONG					MajorFunction,
	__in PFLT_INSTANCE			Instance,
	__in PFILE_OBJECT			FileObject,
	__in PLARGE_INTEGER			ByteOffset,
	__in PVOID					BufferData,
	__in ULONG					BufferLength,
	__out PULONG				ReturnLength,
	__in FLT_IO_OPERATION_FLAGS FltFlags);


NTSTATUS CompleteProcedure(
	IN PDEVICE_OBJECT			DeviceObject,
	IN PIRP						Irp,
	IN PVOID					Context);