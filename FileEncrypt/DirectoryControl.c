#include "DirectoryControl.h"
#include "ContextData.h"
#include "EncryptEngine.h"
#include "CommonVariable.h"

FLT_PREOP_CALLBACK_STATUS PreviousDirectoryControl(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
	FLT_PREOP_CALLBACK_STATUS Status911 = FLT_PREOP_SUCCESS_NO_CALLBACK;
	PVOLUME_CONTEXT VolumeContext;
	PMDL Mdl;
	PVOID Buffer;
	__try
	{
		if (FLT_IS_FASTIO_OPERATION(Data))
		{
			Status911 = FLT_PREOP_DISALLOW_FASTIO;
			__leave;
		}

		PFLT_IO_PARAMETER_BLOCK FltIoParameterBlock = Data->Iopb;
		if ((FltIoParameterBlock->MinorFunction != IRP_MN_QUERY_DIRECTORY) || (FltIoParameterBlock->Parameters.DirectoryControl.QueryDirectory.Length == 0))
			__leave;

		NTSTATUS Status = FltGetVolumeContext(FltObjects->Filter, FltObjects->Volume, &VolumeContext);
		if (!NT_SUCCESS(Status))
			__leave;

		Buffer = ExAllocatePool(NonPagedPool, FltIoParameterBlock->Parameters.DirectoryControl.QueryDirectory.Length);
		if (Buffer == NULL)
			__leave;

		if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_IRP_OPERATION))
		{
			Mdl = IoAllocateMdl(Buffer, FltIoParameterBlock->Parameters.DirectoryControl.QueryDirectory.Length, FALSE, FALSE, NULL);
			if (Mdl == NULL)
				__leave;
			MmBuildMdlForNonPagedPool(Mdl);
		}


		PPREVIOUS_2_POST_CONTEXT p2pContext = (PPREVIOUS_2_POST_CONTEXT)ExAllocateFromNPagedLookasideList(&__NpagedLookasideListOfPrevious2Post);
		if (p2pContext == NULL)
			__leave;

		FltIoParameterBlock->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer = Buffer;
		FltIoParameterBlock->Parameters.DirectoryControl.QueryDirectory.MdlAddress = Mdl;
		FltSetCallbackDataDirty(Data);

		p2pContext->BufferData = Buffer;  //记录自己动态申请的内存
		p2pContext->VolumeContext = VolumeContext;
		*CompletionContext = p2pContext;

		Status911 = FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}
	__finally 
	{
		if (Status911 != FLT_PREOP_SUCCESS_WITH_CALLBACK)
		{
			if (Buffer != NULL)
			{
				ExFreePool(Buffer);
			}

			if (Mdl != NULL)
			{
				IoFreeMdl(Mdl);
			}

			if (VolumeContext != NULL)
			{
				FltReleaseContext(VolumeContext);
			}
		}
	}
	return Status911;
}

FLT_POSTOP_CALLBACK_STATUS PostDirectoryControl(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags)
{
	ASSERT(!FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING));

	PFLT_IO_PARAMETER_BLOCK FltIoParameterBlock = Data->Iopb;
	FLT_POSTOP_CALLBACK_STATUS Status911 = FLT_POSTOP_FINISHED_PROCESSING;
	PPREVIOUS_2_POST_CONTEXT p2pContext = CompletionContext;
	BOOLEAN IsCleanupAllocatedBuffer;
	__try 
	{
		if (!NT_SUCCESS(Data->IoStatus.Status) || (Data->IoStatus.Information == 0))
			__leave;

		PVOID Buffer;
		if (FltIoParameterBlock->Parameters.DirectoryControl.QueryDirectory.MdlAddress != NULL)
		{
			//请求的内存
			Buffer = MmGetSystemAddressForMdlSafe(FltIoParameterBlock->Parameters.DirectoryControl.QueryDirectory.MdlAddress, NormalPagePriority);
			if (Buffer == NULL)
			{
				Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
				Data->IoStatus.Information = 0;
				__leave;
			}
		}
		else if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) || FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_FAST_IO_OPERATION))
		{
			Buffer = FltIoParameterBlock->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;
		}
		else
		{
			if (FltDoCompletionProcessingWhenSafe(Data, FltObjects, CompletionContext, Flags, PostDirectoryControlSafe, &Status911))
			{
				IsCleanupAllocatedBuffer = FALSE;
			}
			else
			{
				Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
				Data->IoStatus.Information = 0;
			}
			__leave;
		}
		__try 
		{
			RtlCopyMemory(Buffer, p2pContext->BufferData, FltIoParameterBlock->Parameters.DirectoryControl.QueryDirectory.Length);
		} 
		except(EXCEPTION_EXECUTE_HANDLER) 
		{
			Data->IoStatus.Status = GetExceptionCode();
			Data->IoStatus.Information = 0;
		}
	}
	__finally 
	{
		if (IsCleanupAllocatedBuffer)
		{
			ExFreePool(p2pContext->BufferData);
			FltReleaseContext(p2pContext->VolumeContext);
			ExFreeToNPagedLookasideList(&__NpagedLookasideListOfPrevious2Post, p2pContext);
		}
	}
	return Status911;
}

FLT_POSTOP_CALLBACK_STATUS PostDirectoryControlSafe(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags)
{
	ASSERT(Data->IoStatus.Information != 0);
	PFLT_IO_PARAMETER_BLOCK FltIoParameterBlock = Data->Iopb;
	PPREVIOUS_2_POST_CONTEXT p2pContext = CompletionContext;
	NTSTATUS Status = FltLockUserBuffer(Data);

	if (!NT_SUCCESS(Status))
	{
		Data->IoStatus.Status = Status;
		Data->IoStatus.Information = 0;
	}
	else
	{
		PVOID Buffer = MmGetSystemAddressForMdlSafe(FltIoParameterBlock->Parameters.DirectoryControl.QueryDirectory.MdlAddress, NormalPagePriority);
		if (Buffer == NULL)
		{
			Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
			Data->IoStatus.Information = 0;
		}
		else
		{
			RtlCopyMemory(Buffer, p2pContext->BufferData, FltIoParameterBlock->Parameters.DirectoryControl.QueryDirectory.Length);
		}
	}

	ExFreePool(p2pContext->BufferData);
	FltReleaseContext(p2pContext->VolumeContext);
	ExFreeToNPagedLookasideList(&__NpagedLookasideListOfPrevious2Post, p2pContext);
	return FLT_POSTOP_FINISHED_PROCESSING;
}
