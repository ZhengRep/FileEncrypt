#include "WriteOperation.h"
#include "EncryptEngine.h"
#include "ContextData.h"
#include "ProcessHelper.h"
#include "Crypt.h"
#include "CommonVariable.h"

FLT_PREOP_CALLBACK_STATUS PreviousWriteCallback(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
	FLT_PREOP_CALLBACK_STATUS Status911 = FLT_PREOP_SUCCESS_NO_CALLBACK; 
	NTSTATUS Status;
	PSTREAM_CONTEXT StreamContext;
	PVOLUME_CONTEXT VolumeContext;
	PFLT_IO_PARAMETER_BLOCK FltIopb = Data->Iopb;
	PVOID WriteBuffer = ExAllocatePool(NonPagedPool, FltIopb->Parameters.Write.Length);
	PMDL Mdl;
	__try
	{
		Status = FltGetVolumeContext(FltObjects->Filter, FltObjects->Volume, &VolumeContext);  //获取在InstanceSetupCallback函数中申请的内存结构
		if (!NT_SUCCESS(Status))
		{
			__leave;
		}

		Status = FindOrCreateStreamContext(Data, FltObjects, FALSE, &StreamContext, NULL);
		if (!NT_SUCCESS(Status))
		{
			__leave;
		}

		BOOLEAN IsSpecialProcess, IsPPTFile;
		if (!IsCurrentProcessMonitored(StreamContext->FileName.Buffer, StreamContext->FileName.Length / sizeof(WCHAR), &IsSpecialProcess, &IsPPTFile))
		{
			__leave;
		}

		if (FLT_IS_FASTIO_OPERATION(Data))
		{
			Status911 = FLT_PREOP_DISALLOW_FASTIO;  //FastIo
			__leave;
		}

		KIRQL Irql;
		ULONG WriteLength = FltIopb->Parameters.Write.Length;
		LARGE_INTEGER WriteOffset = FltIopb->Parameters.Write.ByteOffset;
		if (!(Data->Iopb->IrpFlags & (IRP_NOCACHE | IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO)))
		{
			_Lock(StreamContext, &Irql);
			if ((WriteLength + WriteOffset.QuadPart) > StreamContext->FileLength.QuadPart)
			{
				StreamContext->FileLength.QuadPart = WriteLength + WriteOffset.QuadPart;    //更新文件总长
			}
			_Unlock(StreamContext, Irql);
			__leave;
		}

		if (WriteLength == 0)
		{
			__leave;
		}

		_Lock(StreamContext, &Irql);

		if (!StreamContext->IsFileCrypt && !FltObjects->FileObject->WriteAccess)
		{
			if (IsSpecialProcess == POWERPNT_PROCESS)
			{
				StreamContext->HasPPTWriteData = TRUE;
			}
			_Unlock(StreamContext, Irql);
			__leave;
		}

		if (IsPPTFile && !StreamContext->IsFileCrypt && (IsSpecialProcess == POWERPNT_PROCESS))
		{
			StreamContext->HasPPTWriteData = TRUE;
			_Unlock(StreamContext, Irql);
			__leave;
		}

		_Unlock(StreamContext, Irql);

		if (FlagOn(IRP_NOCACHE, FltIopb->IrpFlags))
		{
			WriteLength = (ULONG)ROUND_TO_SIZE(WriteLength, VolumeContext->SectorSize);
		}

		if (WriteBuffer == NULL)
		{
			__leave;
		}

		if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_IRP_OPERATION))//如果是操作IRP，就
		{
			Mdl = IoAllocateMdl(WriteBuffer, WriteLength, FALSE, FALSE, NULL);
			if (Mdl == NULL)
			{
				__leave;
			}
			MmBuildMdlForNonPagedPool(Mdl);
		}

		PVOID KernelBuffer;
		if (FltIopb->Parameters.Write.MdlAddress != NULL)
		{
			KernelBuffer = MmGetSystemAddressForMdlSafe(FltIopb->Parameters.Write.MdlAddress, NormalPagePriority);
			if (KernelBuffer == NULL)
			{
				Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
				Data->IoStatus.Information = 0;
				Status911 = FLT_PREOP_COMPLETE;
				__leave;
			}
		}
		else
		{

			KernelBuffer = FltIopb->Parameters.Write.WriteBuffer;      //用户内存  
		}

		__try
		{
			RtlCopyMemory(WriteBuffer, KernelBuffer, WriteLength);    //将用户的真实数据拷贝到我们的内存中
			do
			{
				if ((StreamContext->EncryptOnWrite || StreamContext->IsFileCrypt) && (Data->Iopb->IrpFlags & (IRP_NOCACHE | IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO)))
				{
					KeEnterCriticalRegion();
					Status = Encrypting(WriteBuffer, WriteLength);
					KeLeaveCriticalRegion();
				}
			} while (FALSE);
		} 
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Data->IoStatus.Status = GetExceptionCode();
			Data->IoStatus.Information = 0;
			Status911 = FLT_PREOP_COMPLETE;
			__leave;
		}

		//保存所有动态资源到数据结构中
		PPREVIOUS_2_POST_CONTEXT p2pContext = ExAllocateFromNPagedLookasideList(&__NpagedLookasideListOfPrevious2Post);
		if (p2pContext == NULL)
		{
			__leave;
		}

		//改变请求中的内存
		FltIopb->Parameters.Write.WriteBuffer = WriteBuffer;
		FltIopb->Parameters.Write.MdlAddress = Mdl;

		//以前的的数据是不能丢失
		FltSetCallbackDataDirty(Data);                  //通知下层数据已经被修改

		//记录内存方便回收
		p2pContext->BufferData = WriteBuffer;
		p2pContext->VolumeContext = VolumeContext;
		p2pContext->StreamContext = StreamContext;
		*CompletionContext = p2pContext;

		Status911 = FLT_PREOP_SUCCESS_WITH_CALLBACK;

	}
	__finally
	{
		if (Status911 != FLT_PREOP_SUCCESS_WITH_CALLBACK)
		{
			if (WriteBuffer != NULL)
			{
				ExFreePool(WriteBuffer);
			}
			if (Mdl != NULL)
			{
				IoFreeMdl(Mdl);
			}
			if (VolumeContext != NULL)
			{
				FltReleaseContext(VolumeContext);
			}
			if (NULL != StreamContext)
			{
				FltReleaseContext(StreamContext);
			}
		}
		return Status911;
	}
}

FLT_POSTOP_CALLBACK_STATUS PostWriteCallback(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	PPREVIOUS_2_POST_CONTEXT p2pContext;
	KIRQL Irql;
	LARGE_INTEGER ByteOffset = Data->Iopb->Parameters.Write.ByteOffset;
	ULONG ReturnLength = Data->IoStatus.Information;

	_Lock(p2pContext->StreamContext, &Irql);

	if ((ByteOffset.QuadPart + (LONGLONG)ReturnLength) > p2pContext->StreamContext->FileLength.QuadPart)
	{
		p2pContext->StreamContext->FileLength.QuadPart = ByteOffset.QuadPart + (LONGLONG)ReturnLength;
	}

	if (!p2pContext->StreamContext->DecryptOnRead)
	{
		p2pContext->StreamContext->DecryptOnRead = TRUE;
	}

	if (!p2pContext->StreamContext->HasWriteData)
	{
		p2pContext->StreamContext->HasWriteData = TRUE;
	}
	_Unlock(p2pContext->StreamContext, Irql);

	ExFreePool(p2pContext->BufferData);
	FltReleaseContext(p2pContext->VolumeContext);
	FltReleaseContext(p2pContext->StreamContext);

	ExFreeToNPagedLookasideList(&__NpagedLookasideListOfPrevious2Post, p2pContext);

	return FLT_POSTOP_FINISHED_PROCESSING;
}
