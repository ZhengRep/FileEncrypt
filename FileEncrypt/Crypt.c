#include "Crypt.h"

NTSTATUS Encrypting(PVOID BufferData, ULONG BufferLength)
{
	PUCHAR ByteBuffer = (PUCHAR)BufferData;
	ULONG i;
	for (i = 0; i < BufferLength; i++)
	{
		ByteBuffer[i] ^= ENCRYPT_BUFFER_KEY_BIT;
	}

	return STATUS_SUCCESS;
}

NTSTATUS Decrypting(PVOID BufferData, ULONG BufferLength)
{
	PUCHAR ByteBuffer = (PUCHAR)BufferData;
	ULONG i;
	for (i = 0; i < BufferLength; i++)
	{
		ByteBuffer[i] ^= ENCRYPT_BUFFER_KEY_BIT;
	}

	return STATUS_SUCCESS;
}
