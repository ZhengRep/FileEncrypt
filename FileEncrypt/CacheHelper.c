#include "CacheHelper.h"
#include "CommonSetting.h"

void ClearFileCache(PFILE_OBJECT FileObject, BOOLEAN IsFlushCache, PLARGE_INTEGER FileOffset, ULONG Length)
{
	LARGE_INTEGER Delay50Milliseconds = { (ULONG)(-50 * 1000 * 10), -1 };
	IO_STATUS_BLOCK IoStatus = { 0 };

	if ((FileObject == NULL))
	{
		return;
	}
	//FSRTL_COMMON_FCB_HEADER结构包含文件系统维护的关于文件、目录、卷或备用数据流的上下文信息。
	PFSRTL_COMMON_FCB_HEADER Fcb = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;
	if (Fcb == NULL)
	{
		return;
	}

Acquire:
	FsRtlEnterFileSystem();

	BOOLEAN IsResource;
	BOOLEAN IsPagingIoResource;
	if (Fcb->Resource)
		IsResource = ExAcquireResourceExclusiveLite(Fcb->Resource, TRUE);
	if (Fcb->PagingIoResource)
		IsPagingIoResource = ExAcquireResourceExclusive(Fcb->PagingIoResource, FALSE);
	else
		IsPagingIoResource = TRUE;
	if (!IsPagingIoResource)
	{
		if (Fcb->Resource)  ExReleaseResource(Fcb->Resource);
		FsRtlExitFileSystem();
		KeDelayExecutionThread(KernelMode, FALSE, &Delay50Milliseconds);
		goto Acquire;
	}

	if (FileObject->SectionObjectPointer)
	{
		IoSetTopLevelIrp((PIRP)FSRTL_FSP_TOP_LEVEL_IRP);
		if (IsFlushCache)
		{
			CcFlushCache(FileObject->SectionObjectPointer, FileOffset, Length, &IoStatus);
		}

		if (FileObject->SectionObjectPointer->ImageSectionObject)
		{
			MmFlushImageSection( FileObject->SectionObjectPointer, MmFlushForWrite );
		}

		if (FileObject->SectionObjectPointer->DataSectionObject)
		{
			CcPurgeCacheSection(FileObject->SectionObjectPointer,NULL,0, FALSE);
		}

		IoSetTopLevelIrp(NULL);
	}

	if (Fcb->PagingIoResource)
		ExReleaseResourceLite(Fcb->PagingIoResource);
	if (Fcb->Resource)
		ExReleaseResourceLite(Fcb->Resource);

	FsRtlExitFileSystem();
}
