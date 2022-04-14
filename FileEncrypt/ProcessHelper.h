#pragma once
#include <fltKernel.h>

#pragma pack(push, 1)
# define MAX_PATH			256

typedef struct _PROCESS_INFORMATION
{
	CHAR				ImageFileName[15];//当前请求的发起者的名称
	BOOLEAN				IsMonitor;
	WCHAR				FileType[64][6];//进程的操作类型
	LIST_ENTRY			ListEntry;
}PROCESS_INFORMATION, * PPROCESS_INFORMATION;

PCHAR GetImageFileName(PCHAR ImageFileName, PEPROCESS EProcess);
ULONG GetImageFileNameOffset();
NTSTATUS AddProcessContext(PUCHAR ImageFileName, BOOLEAN IsMonitor);
BOOLEAN SearchForSpecifiedProcess(PUCHAR ImageFileName, BOOLEAN IsRemove);
BOOLEAN IsCurrentProcessMonitored(WCHAR* FileFullPath, ULONG FileFullPathLength, BOOLEAN* IsSpecialProcess, BOOLEAN* IsPPTFile);

#pragma pack(pop)