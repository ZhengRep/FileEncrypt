#include "HandleIO.h"
#include "CommonSetting.h"

NTSTATUS HandleIO(ULONG MajorFunction, PFLT_INSTANCE Instance, PFILE_OBJECT FileObject, PLARGE_INTEGER ByteOffset, PVOID BufferData, ULONG BufferLength, PULONG ReturnLength, FLT_IO_OPERATION_FLAGS FltFlags)
{
	PDEVICE_OBJECT DeviceObject = IoGetDeviceAttachmentBaseRef(FileObject->DeviceObject);

	if (NULL == DeviceObject)
	{
		return STATUS_UNSUCCESSFUL;
	}

	PDEVICE_OBJECT FSDeviceObject = DeviceObject->Vpb->DeviceObject; //FileSystemDevice

	if (NULL == FSDeviceObject)
	{
		ObDereferenceObject(DeviceObject);
		return STATUS_UNSUCCESSFUL;
	}

	KEVENT KEvent;
	KeInitializeEvent(&KEvent, SynchronizationEvent, FALSE);   //不授信 回收内存

	PIRP Irp = IoAllocateIrp(FSDeviceObject->StackSize, FALSE);
	IO_STATUS_BLOCK IoStatusBlock;
	if (Irp == NULL)
	{
		ObDereferenceObject(DeviceObject);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	Irp->AssociatedIrp.SystemBuffer = NULL;
	Irp->MdlAddress = NULL;
	Irp->UserBuffer = BufferData;
	Irp->UserEvent = &KEvent;
	Irp->UserIosb = &IoStatusBlock;
	Irp->Tail.Overlay.Thread = PsGetCurrentThread();
	Irp->RequestorMode = KernelMode;
	if (MajorFunction == IRP_MJ_READ)
		Irp->Flags = IRP_DEFER_IO_COMPLETION | IRP_READ_OPERATION | IRP_NOCACHE;
	else if (MajorFunction == IRP_MJ_WRITE)
		Irp->Flags = IRP_DEFER_IO_COMPLETION | IRP_WRITE_OPERATION | IRP_NOCACHE;
	else
	{
		ObDereferenceObject(DeviceObject);
		return STATUS_UNSUCCESSFUL;
	}

	if ((FltFlags & FLTFL_IO_OPERATION_PAGING) == FLTFL_IO_OPERATION_PAGING)
	{
		Irp->Flags |= IRP_PAGING_IO;
	}

	//当前设备必须为下层设备堆栈进行赋值
	PIO_STACK_LOCATION IoStackLocation;
	IoStackLocation = IoGetNextIrpStackLocation(Irp);
	IoStackLocation->MajorFunction = (UCHAR)MajorFunction;
	IoStackLocation->MinorFunction = (UCHAR)IRP_MN_NORMAL;
	IoStackLocation->DeviceObject = FSDeviceObject;
	IoStackLocation->FileObject = FileObject;
	if (MajorFunction == IRP_MJ_READ)
	{
		IoStackLocation->Parameters.Read.ByteOffset = *ByteOffset;
		IoStackLocation->Parameters.Read.Length = BufferLength;
	}
	else
	{
		IoStackLocation->Parameters.Write.ByteOffset = *ByteOffset;
		IoStackLocation->Parameters.Write.Length = BufferLength;
	}

	IoSetCompletionRoutine(Irp, CompleteProcedure, 0, TRUE, TRUE, TRUE);
	(void)IoCallDriver(FSDeviceObject, Irp);

	KeWaitForSingleObject(&KEvent, Executive, KernelMode, TRUE, 0);
	*ReturnLength = IoStatusBlock.Information;

	ObDereferenceObject(DeviceObject);

	return IoStatusBlock.Status;
}

NTSTATUS CompleteProcedure(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
	*Irp->UserIosb = Irp->IoStatus;
	KeSetEvent(Irp->UserEvent, 0, FALSE);
	IoFreeIrp(Irp);
	return STATUS_MORE_PROCESSING_REQUIRED;
}
