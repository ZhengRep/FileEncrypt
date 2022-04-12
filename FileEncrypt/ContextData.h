#pragma once
#include<fltKernel.h>

#pragma pack(push, 1)

#define _LOCK_(Resource)	(ASSERT(KeGetCurrentIrql() <= APC_LEVEL), \
							ASSERT(ExIsResourceAcquiredExclusiveLite(Resource) || !ExIsResourceAcquiredSharedLite(Resource)), \
							KeEnterCriticalRegion(), \
							ExAcquireResourceExclusiveLite(Resource, TRUE))

#define _UNLOCK_(Resource)	(ASSERT(KeGetCurrentIrql() <= APC_LEVEL), \
							ASSERT(ExIsResourceAcquiredSharedLite(Resource) || ExIsResourceAcquiredExclusiveLite(Resource)),\
							ExReleaseResourceLite(Resource),\
							KeLeaveCriticalRegion())

typedef struct _VOLUME_CONTEXT {
	UNICODE_STRING		VolumeName;
	UNICODE_STRING		FileSystemName;
	KSPIN_LOCK			SpinLock;
	FAST_MUTEX			FastMutex;
	ULONG				SectorSize;
}VOLUME_CONTEXT, * PVOLUME_CONTEXT;

typedef struct _STREAM_CONTEXT {
	UNICODE_STRING		FileName;
	WCHAR				VolumeName[64];
	LONG				ReferenceCount;
	LARGE_INTEGER		FileLength;            //文件的真实大小
	LARGE_INTEGER		FileLengthEx;          //文件的真实大小 + EncryptHeader
	ULONG				EncryptHeaderLength;
	ULONG				Access;
	BOOLEAN				IsFileCrypt;
	BOOLEAN				EncryptOnWrite;
	BOOLEAN				DecryptOnRead;
	BOOLEAN				HasWriteData;
	BOOLEAN				FirstWriteNotFromBeg;
	BOOLEAN				HasPPTWriteData;
	PERESOURCE			Resource;
	KSPIN_LOCK			SpinLock;
} STREAM_CONTEXT, * PSTREAM_CONTEXT;

NTSTATUS
FindOrCreateStreamContext(
	__in PFLT_CALLBACK_DATA				Data,
	__in PFLT_RELATED_OBJECTS			FltObjects,
	__in BOOLEAN						CreateIfNotFound,
	__deref_out PSTREAM_CONTEXT*		StreamContextParamater,
	__out_opt PBOOLEAN					IsContextCreated);

NTSTATUS
CreateStreamContext(
	__in PFLT_RELATED_OBJECTS			FltObjects,
	__deref_out PSTREAM_CONTEXT*		StreamContext);

NTSTATUS
UpdateNameInStreamContext(
	__in PUNICODE_STRING				Name,
	__inout PSTREAM_CONTEXT				StreamContext);

VOID _Lock(PSTREAM_CONTEXT StreamContext, PKIRQL Irql);
VOID _Unlock(PSTREAM_CONTEXT StreamContext, KIRQL Irql);

#pragma pack(pop)