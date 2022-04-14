#include "ReadOperation.h"
#include "ContextData.h"
#include "EncryptEngine.h"
#include "ProcessHelper.h"
#include "CommonVariable.h"
#include "Crypt.h"

FLT_PREOP_CALLBACK_STATUS PreviousReadCallback(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
	NTSTATUS Status;
	FLT_PREOP_CALLBACK_STATUS Status911 = FLT_PREOP_SUCCESS_NO_CALLBACK;
	PSTREAM_CONTEXT StreamContext;
	PVOLUME_CONTEXT VolumeContext;
	PPREVIOUS_2_POST_CONTEXT p2pContext;
	PVOID ReadBuffer;
	PMDL Mdl;
	__try
	{

		Status = FltGetVolumeContext(FltObjects->Filter, FltObjects->Volume, &VolumeContext);   //调式
		if (!NT_SUCCESS(Status))
		{
			return Status911;
		}

		Status = FindOrCreateStreamContext(Data, FltObjects, FALSE, &StreamContext, NULL);
		if (!NT_SUCCESS(Status))
		{
			__leave;
		}

		if (!IsCurrentProcessMonitored(StreamContext->FileName.Buffer, StreamContext->FileName.Length / sizeof(WCHAR), NULL, NULL))
		{
			__leave;
		}

		if (FLT_IS_FASTIO_OPERATION(Data))
		{
			Status911 = FLT_PREOP_DISALLOW_FASTIO;   //迫使Io管理器重新分配Irp
			__leave;
		}

		if (!(Data->Iopb->IrpFlags & (IRP_NOCACHE | IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO)))
		{
			__leave;
		}

		//获取的内容的位置发生错误
		PFLT_IO_PARAMETER_BLOCK FltIopb = Data->Iopb;
		if (FltIopb->Parameters.Read.ByteOffset.QuadPart >= StreamContext->FileLength.QuadPart)
		{
			Data->IoStatus.Status = STATUS_END_OF_FILE;
			Data->IoStatus.Information = 0;
			Status911 = FLT_PREOP_COMPLETE;
			__leave;
		}

		//获取0字节的文件长度
		ULONG ReadLength = FltIopb->Parameters.Read.Length;
		if (ReadLength == 0)
		{
			__leave;
		}

		if (FlagOn(IRP_NOCACHE, FltIopb->IrpFlags))
		{
			ReadLength = (ULONG)ROUND_TO_SIZE(ReadLength, VolumeContext->SectorSize);  //有个地方对其设置
		}

		ReadBuffer = ExAllocatePool(NonPagedPool, ReadLength);   //0x10000
		if (ReadBuffer == NULL)
		{
			__leave;
		}

		if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_IRP_OPERATION))   //问题
		{
			Mdl = IoAllocateMdl(ReadBuffer, ReadLength, FALSE, FALSE, NULL);
			if (Mdl == NULL)
			{
				__leave;
			}
			MmBuildMdlForNonPagedPool(Mdl);
		}
		p2pContext = ExAllocateFromNPagedLookasideList(&__NpagedLookasideListOfPrevious2Post);
		if (p2pContext == NULL)
		{
			__leave;
		}

		//曾经对该文件进行读取的内存数据
		FltIopb->Parameters.Read.ReadBuffer = ReadBuffer;
		FltIopb->Parameters.Read.MdlAddress = Mdl;

		FltSetCallbackDataDirty(Data);              //调式 数据交换

		p2pContext->BufferData = ReadBuffer;    //依然是我们动态的内存
		p2pContext->VolumeContext = VolumeContext;
		p2pContext->StreamContext = StreamContext;
		*CompletionContext = p2pContext;

		Status911 = FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}
	__finally 
	{
		if (Status911 != FLT_PREOP_SUCCESS_WITH_CALLBACK)
		{
			if (ReadBuffer != NULL)
			{
				ExFreePool(ReadBuffer);
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
	}
	return Status911;
}

FLT_POSTOP_CALLBACK_STATUS PostReadCallback(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags)
{
	FLT_POSTOP_CALLBACK_STATUS Status911 = FLT_POSTOP_FINISHED_PROCESSING;
	PFLT_IO_PARAMETER_BLOCK FltIopb = Data->Iopb;
	PPREVIOUS_2_POST_CONTEXT p2pContext = CompletionContext;   //该成员就是在Previous函数中投递的内存
	PVOID ReadBuffer;
	BOOLEAN IsCleanupAllocatedBuffer;
	__try 
	{
		if (!NT_SUCCESS(Data->IoStatus.Status) || (Data->IoStatus.Information == 0))
		{
			__leave;
		}

		if (FltIopb->Parameters.Read.MdlAddress != NULL)
		{
			//根据MDL获取之前我们与之关联的虚拟内存
			ReadBuffer = MmGetSystemAddressForMdlSafe(FltIopb->Parameters.Read.MdlAddress, NormalPagePriority);
			if (ReadBuffer == NULL)
			{
				Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
				Data->IoStatus.Information = 0;
				__leave;
			}

		}
		else if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) || FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_FAST_IO_OPERATION))
		{
			ReadBuffer = FltIopb->Parameters.Read.ReadBuffer;
		}
		else
		{
			if (FltDoCompletionProcessingWhenSafe(Data, FltObjects, CompletionContext, Flags, PostReadCallbackSafe, &Status911))
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
			do {
				//判断是否是加密文件
				if (p2pContext->StreamContext->IsFileCrypt || p2pContext->StreamContext->DecryptOnRead)
				{
					if (p2pContext->StreamContext->FileLength.QuadPart < (Data->Iopb->Parameters.Read.ByteOffset.QuadPart + Data->IoStatus.Information))
					{
						Data->IoStatus.Information = (ULONG)(p2pContext->StreamContext->FileLength.QuadPart - Data->Iopb->Parameters.Read.ByteOffset.QuadPart);
						FltSetCallbackDataDirty(Data);
					}

					KeEnterCriticalRegion();

					Decrypting(p2pContext->BufferData, Data->IoStatus.Information);

					KeLeaveCriticalRegion();
				}

			} while (FALSE);
			//调式
			RtlCopyMemory(ReadBuffer, p2pContext->BufferData, Data->IoStatus.Information);
		} 
		__except(EXCEPTION_EXECUTE_HANDLER) 
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
			FltReleaseContext(p2pContext->StreamContext);
			ExFreeToNPagedLookasideList(&__NpagedLookasideListOfPrevious2Post, p2pContext);
		}
	}
	return Status911;
}

FLT_POSTOP_CALLBACK_STATUS PostReadCallbackSafe(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);
	ASSERT(Data->IoStatus.Information != 0);
	PFLT_IO_PARAMETER_BLOCK FltIopb = Data->Iopb;
	PPREVIOUS_2_POST_CONTEXT p2pContext;
	NTSTATUS Status = FltLockUserBuffer(Data);
	PVOID KernelMdl;
	if (!NT_SUCCESS(Status))
	{
		Data->IoStatus.Status = Status;
		Data->IoStatus.Information = 0;

	}
	else
	{
		KernelMdl = MmGetSystemAddressForMdlSafe(FltIopb->Parameters.Read.MdlAddress, NormalPagePriority);
		if (KernelMdl == NULL)
		{
			Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
			Data->IoStatus.Information = 0;
		}
		else
		{
			do 
			{
				if (p2pContext->StreamContext->IsFileCrypt || p2pContext->StreamContext->DecryptOnRead)
				{
					if (p2pContext->StreamContext->FileLength.QuadPart < (Data->Iopb->Parameters.Read.ByteOffset.QuadPart + Data->IoStatus.Information))
					{
						Data->IoStatus.Information = (ULONG)(p2pContext->StreamContext->FileLength.QuadPart - Data->Iopb->Parameters.Read.ByteOffset.QuadPart);
						FltSetCallbackDataDirty(Data);
					}

					KeEnterCriticalRegion();

					Decrypting(p2pContext->BufferData, Data->IoStatus.Information);

					KeLeaveCriticalRegion();
				}

			} while (FALSE);

			RtlCopyMemory(KernelMdl, p2pContext->BufferData, Data->IoStatus.Information);
		}
	}

	ExFreePool(p2pContext->BufferData);

	FltReleaseContext(p2pContext->VolumeContext);
	FltReleaseContext(p2pContext->StreamContext);

	ExFreeToNPagedLookasideList(&__NpagedLookasideListOfPrevious2Post, p2pContext);

	return FLT_POSTOP_FINISHED_PROCESSING;
}
