#pragma once
#include <fltKernel.h>
#include "ContextData.h"

#pragma pack(push, 1)

#define  SIGNATURE_LENGTH		16
#define  MAX_LENGTH				512

typedef struct _ENCRYPT_HEADER
{
	UCHAR			Signature[SIGNATURE_LENGTH];
	LONGLONG		FileLength;										//文件的真实大小
	UCHAR			Reserved[MAX_LENGTH - SIGNATURE_LENGTH - 8];	//维持一个扇区的大小
}ENCRYPT_HEADER, * PENCRYPT_HEADER;

NTSTATUS InitializeEncryptHeader();
NTSTATUS GetFileStandardInformation(
	__in	PFLT_CALLBACK_DATA			Data,
	__in	PFLT_RELATED_OBJECTS		FltObjects,
	__in	PLARGE_INTEGER				FileAllocationSize,
	__in	PLARGE_INTEGER				FileSize,
	__in	PBOOLEAN					IsDirectory);

NTSTATUS
GetFileStandardInformation(
	__in	PFLT_CALLBACK_DATA			Data,
	__in	PFLT_RELATED_OBJECTS		FltObjects,
	__in	PLARGE_INTEGER				FileAllocationSize,
	__in	PLARGE_INTEGER				FileSize,
	__in	PBOOLEAN					IsDirectory
);

NTSTATUS GetFileOffset(
	__in	PFLT_CALLBACK_DATA			Data,
	__in	PFLT_RELATED_OBJECTS		FltObjects,
	__out	PLARGE_INTEGER				FileOffset);

NTSTATUS SetFileOffset(
	__in	PFLT_CALLBACK_DATA			Data,
	__in	PFLT_RELATED_OBJECTS		FltObjects,
	__in	PLARGE_INTEGER				FileOffset
);

NTSTATUS GetFileLength(
	__in	PFLT_CALLBACK_DATA			Data,
	__in	PFLT_RELATED_OBJECTS		FltObjects,
	__in	PLARGE_INTEGER				FileLength);

NTSTATUS WriteEncryptHeader(
	__in	PFLT_CALLBACK_DATA			Data,
	__in	PFLT_RELATED_OBJECTS		FltObjects,
	__in	PFILE_OBJECT				FileObject,
	__in	PSTREAM_CONTEXT				StreamContext);

NTSTATUS UpdateEncryptHeader(
	__in	PFLT_CALLBACK_DATA			Data,
	__in	PFLT_RELATED_OBJECTS		FltObjects,
	__in	PFILE_OBJECT				FileObject,
	__in	PSTREAM_CONTEXT				StreamContext,
	__in	PVOLUME_CONTEXT				VolumeContext);

#pragma pack(pop)
