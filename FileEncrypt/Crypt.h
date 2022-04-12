#pragma once
#include <fltKernel.h>

NTSTATUS Encrypting(
	__in PVOID		BufferData,
	__in ULONG		BufferLength);

NTSTATUS Decrypting(
	__in PVOID		BufferData,
	__in ULONG		BufferLength);
