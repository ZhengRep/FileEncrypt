#pragma once
#include <fltKernel.h>

#define ENCRYPT_BUFFER_KEY_BIT			0x77

NTSTATUS Encrypting(
	__in PVOID		BufferData,
	__in ULONG		BufferLength
);

NTSTATUS Decrypting(
	__in PVOID		BufferData,
	__in ULONG		BufferLength
);
