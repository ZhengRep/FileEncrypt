#include "FileHelper.h"
#include "CommonVariable.h"

//////////////////////////////////////////////////////////////////////////
UCHAR				__Signature[SIGNATURE_LENGTH] = { 0x3a, 0x8a, 0x2c, 0xd1, 0x50, 0xe8, 0x47, 0x5f, \
													  0xbe, 0xdb, 0xd7, 0x6c, 0xa2, 0xe9, 0x8e, 0x1d };

NTSTATUS InitializeEncryptHeader()
{
	if (NULL != __EncryptHeader)
	{
		return STATUS_SUCCESS;
	}

	__EncryptHeader = (PENCRYPT_HEADER)ExAllocatePool(NonPagedPool, sizeof(ENCRYPT_HEADER));
	if (NULL == __EncryptHeader)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlZeroMemory(__EncryptHeader, sizeof(ENCRYPT_HEADER));
	RtlCopyMemory(__EncryptHeader->Signature, __Signature, SIGNATURE_LENGTH);

	return STATUS_SUCCESS;
}

