#include "ProcessHelper.h"
#include "CommonVariable.h"

//////////////////////////////////////////////////////////////////////////
ULONG				__ImageFileNameOffset;

PCHAR GetImageFileName(PCHAR ImageFileName, PEPROCESS EProcess)
{
	if (__ImageFileNameOffset)
	{
		if (!EProcess)
		{
			EProcess = PsGetCurrentProcess();
		}
		CHAR* ImageFileNameBuffer= (PCHAR)EProcess + __ImageFileNameOffset;
		strncpy(ImageFileName, ImageFileNameBuffer, 15);
	}
	else
	{
		strcpy(ImageFileName, "???");
	}
	return ImageFileName;
}

ULONG GetImageFileNameOffset()
{
	PEPROCESS Eprocess = PsGetCurrentProcess();   //当前进程体指针

	for (int i = 0; i < 3 * PAGE_SIZE; i++)
	{
		if (!strncmp("System", (PCHAR)Eprocess + i, strlen("System")))
		{
			return i;
		}
	}
	return 0;
}

//Add monitor process to MonitorList
NTSTATUS AddProcessContext(PUCHAR ImageFileName, BOOLEAN IsMonitor)
{
	NTSTATUS Status = STATUS_ERROR;
	PPROCESS_INFORMATION ProcessInfo = NULL;
	BOOLEAN IsOk;
	__try 
	{
		if (NULL == ImageFileName)
		{
			Status = STATUS_ERROR;
			__leave;
		}

		IsOk = SearchForSpecifiedProcess(ImageFileName, FALSE);    //已经存在于监控列表
		if (IsOk)
		{
			Status = STATUS_EXIST;
			__leave;
		}

		ProcessInfo = (PPROCESS_INFORMATION)ExAllocatePool(PagedPool, sizeof(PROCESS_INFORMATION));
		if (NULL == ProcessInfo)
		{
			Status = STATUS_ERROR;
			__leave;
		}

		RtlZeroMemory(ProcessInfo, sizeof(PROCESS_INFORMATION));

		if (!_strnicmp((const char*)ImageFileName, "wps.exe", strlen("wps.exe")))   //需要对wps进程监控
		{
			wcscpy(ProcessInfo->FileType[0], L".html");
			wcscpy(ProcessInfo->FileType[1], L".txt");
			wcscpy(ProcessInfo->FileType[2], L".mh_");
			wcscpy(ProcessInfo->FileType[3], L".rtf");
			wcscpy(ProcessInfo->FileType[4], L".ht_");
			wcscpy(ProcessInfo->FileType[5], L".xml");
			wcscpy(ProcessInfo->FileType[6], L".mht");
			wcscpy(ProcessInfo->FileType[7], L".mhtml");
			wcscpy(ProcessInfo->FileType[8], L".htm");
			wcscpy(ProcessInfo->FileType[9], L".dot");
			wcscpy(ProcessInfo->FileType[10], L".tmp");
			wcscpy(ProcessInfo->FileType[11], L".docm");
			wcscpy(ProcessInfo->FileType[12], L".docx");
			wcscpy(ProcessInfo->FileType[13], L".doc");
		}
		else if (!_strnicmp((const char*)ImageFileName, "notepad.exe", strlen("notepad.exe")))
		{
			wcscpy(ProcessInfo->FileType[0], L".txt");
		}
		//		else if (!_strnicmp(pszProcessName, "EXCEL.EXE", strlen("EXCEL.EXE")))
		else if (!_strnicmp((const char*)ImageFileName, "et.exe", strlen("et.exe")))
		{
			wcscpy(ProcessInfo->FileType[0], L".xls");
			wcscpy(ProcessInfo->FileType[1], L".xml");
			wcscpy(ProcessInfo->FileType[2], L".mht");
			wcscpy(ProcessInfo->FileType[3], L".mhtml");
			wcscpy(ProcessInfo->FileType[4], L".htm");
			wcscpy(ProcessInfo->FileType[5], L".html");
			wcscpy(ProcessInfo->FileType[6], L".mh_");
			wcscpy(ProcessInfo->FileType[7], L".ht_");
			wcscpy(ProcessInfo->FileType[8], L".xlt");
			wcscpy(ProcessInfo->FileType[9], L".txt");
			wcscpy(ProcessInfo->FileType[10], L".");
		}
		// 		else if (!_strnicmp(pszProcessName, "POWERPNT.EXE", strlen("POWERPNT.EXE")))
		else if (!_strnicmp((const char*)ImageFileName, "wpp.exe", strlen("wpp.exe")))
		{
			wcscpy(ProcessInfo->FileType[0], L".ppt");
			wcscpy(ProcessInfo->FileType[1], L".tmp");
			wcscpy(ProcessInfo->FileType[2], L".rtf");
			wcscpy(ProcessInfo->FileType[3], L".pot");
			wcscpy(ProcessInfo->FileType[4], L".ppsm");
			wcscpy(ProcessInfo->FileType[5], L".mht");
			wcscpy(ProcessInfo->FileType[6], L".mhtml");
			wcscpy(ProcessInfo->FileType[7], L".htm");
			wcscpy(ProcessInfo->FileType[8], L".html");
			wcscpy(ProcessInfo->FileType[9], L".pps");
			wcscpy(ProcessInfo->FileType[10], L".ppa");
			wcscpy(ProcessInfo->FileType[11], L".pptx");
			wcscpy(ProcessInfo->FileType[12], L".pptm");
			wcscpy(ProcessInfo->FileType[13], L".potx");
			wcscpy(ProcessInfo->FileType[14], L".potm");
			wcscpy(ProcessInfo->FileType[15], L".ppsx");
			wcscpy(ProcessInfo->FileType[16], L".mh_");
			wcscpy(ProcessInfo->FileType[17], L".ht_");
		}
		else if (!_strnicmp((const char*)ImageFileName, "explorer.exe", strlen("explorer.exe")))
		{
			wcscpy(ProcessInfo->FileType[0], L".mp3");
			wcscpy(ProcessInfo->FileType[1], L".mp2");
			wcscpy(ProcessInfo->FileType[2], L".xml");
			wcscpy(ProcessInfo->FileType[3], L".mht");
			wcscpy(ProcessInfo->FileType[4], L".mhtml");
			wcscpy(ProcessInfo->FileType[5], L".htm");
			wcscpy(ProcessInfo->FileType[6], L".html");
			wcscpy(ProcessInfo->FileType[7], L".xlt");
			wcscpy(ProcessInfo->FileType[8], L".mid");
			wcscpy(ProcessInfo->FileType[9], L".rmi");
			wcscpy(ProcessInfo->FileType[10], L".midi");
			wcscpy(ProcessInfo->FileType[11], L".asf");
			wcscpy(ProcessInfo->FileType[12], L".wm");
			wcscpy(ProcessInfo->FileType[13], L".wma");
			wcscpy(ProcessInfo->FileType[14], L".wmv");
			wcscpy(ProcessInfo->FileType[15], L".avi");
			wcscpy(ProcessInfo->FileType[16], L".wav");
			wcscpy(ProcessInfo->FileType[17], L".mpg");
			wcscpy(ProcessInfo->FileType[18], L".mpeg");
			wcscpy(ProcessInfo->FileType[19], L".xls");
			wcscpy(ProcessInfo->FileType[20], L".ppt");
		}
		else if (!_strnicmp((const char*)ImageFileName, "System", strlen("System")))
		{
			wcscpy(ProcessInfo->FileType[0], L".doc");
			wcscpy(ProcessInfo->FileType[1], L".xls");
			wcscpy(ProcessInfo->FileType[2], L".ppt");
			wcscpy(ProcessInfo->FileType[3], L".txt");
			wcscpy(ProcessInfo->FileType[4], L".mp2");
			wcscpy(ProcessInfo->FileType[5], L".rtf");
			wcscpy(ProcessInfo->FileType[6], L".mp3");
			wcscpy(ProcessInfo->FileType[7], L".xml");
			wcscpy(ProcessInfo->FileType[8], L".mht");
			wcscpy(ProcessInfo->FileType[9], L".mhtml");
			wcscpy(ProcessInfo->FileType[10], L".htm");
			wcscpy(ProcessInfo->FileType[11], L".html");
			wcscpy(ProcessInfo->FileType[12], L".docx");
			wcscpy(ProcessInfo->FileType[13], L".docm");
			wcscpy(ProcessInfo->FileType[14], L".pps");
			wcscpy(ProcessInfo->FileType[15], L".ppa");
			wcscpy(ProcessInfo->FileType[16], L".pptx");
			wcscpy(ProcessInfo->FileType[17], L".pptm");
			wcscpy(ProcessInfo->FileType[18], L".potx");
			wcscpy(ProcessInfo->FileType[19], L".potm");
			wcscpy(ProcessInfo->FileType[20], L".ppsx");
			wcscpy(ProcessInfo->FileType[21], L".pot");
			wcscpy(ProcessInfo->FileType[22], L".ppsm");
			wcscpy(ProcessInfo->FileType[23], L".mh_");
			wcscpy(ProcessInfo->FileType[24], L".ht_");
			wcscpy(ProcessInfo->FileType[25], L".xlt");
			wcscpy(ProcessInfo->FileType[26], L".mid");
			wcscpy(ProcessInfo->FileType[27], L".rmi");
			wcscpy(ProcessInfo->FileType[28], L".midi");
			wcscpy(ProcessInfo->FileType[29], L".asf");
			wcscpy(ProcessInfo->FileType[30], L".wm");
			wcscpy(ProcessInfo->FileType[31], L".wma");
			wcscpy(ProcessInfo->FileType[32], L".wmv");
			wcscpy(ProcessInfo->FileType[33], L".avi");
			wcscpy(ProcessInfo->FileType[34], L".wav");
			wcscpy(ProcessInfo->FileType[35], L".mpg");
			wcscpy(ProcessInfo->FileType[36], L".mpeg");

			wcscpy(ProcessInfo->FileType[37], L".tmp");
			wcscpy(ProcessInfo->FileType[38], L".dot");
		}

		ProcessInfo->IsMonitor = IsMonitor;
		RtlCopyMemory(ProcessInfo->ImageFileName, ImageFileName, strlen((const char*)ImageFileName)); //Test 
		ExInterlockedInsertTailList(&__ListHeadOfMonitorProcess, &ProcessInfo->ListEntry, &__SpinLockOfListEntry);
	}
	__finally 
	{
	}

	return Status;
}

BOOLEAN SearchForSpecifiedProcess(PUCHAR ImageFileName, BOOLEAN IsRemove)
{
	BOOLEAN IsOk = TRUE;
	__try 
	{
		PLIST_ENTRY TempListEntry;
		TempListEntry = __ListHeadOfMonitorProcess.Flink;

		PPROCESS_INFORMATION ProcessInfo;
		KIRQL Irql;
		while (&__ListHeadOfMonitorProcess != TempListEntry)
		{
			ProcessInfo = CONTAINING_RECORD(TempListEntry, PROCESS_INFORMATION, ListEntry);

			if (!_strnicmp(ProcessInfo->ImageFileName, (const char*)ImageFileName, strlen((const char*)ImageFileName)))  //字符串相同返回0
			{
				//查找了目标字符串
				IsOk = TRUE;
				if (IsRemove)
				{
					KeAcquireSpinLock(&__SpinLockOfListEntry, &Irql);
					RemoveEntryList(&ProcessInfo->ListEntry);
					KeReleaseSpinLock(&__SpinLockOfListEntry, Irql);
					ExFreePool(ProcessInfo);
					ProcessInfo = NULL;
				}
				__leave;
			}
			TempListEntry = TempListEntry->Flink;
		}
		IsOk = FALSE;
	}
	__finally 
	{
	}

	return IsOk;
}

BOOLEAN IsCurrentProcessMonitored(WCHAR* FileFullPath, ULONG FileFullPathLength, BOOLEAN* IsSpecialProcess, BOOLEAN* IsPPTFile)
{
	BOOLEAN IsOk;
	__try 
	{
		//当前进程EProcess中获取ImageFileName
		UCHAR ImageFileName[25] = { 0 };
		GetImageFileName(ImageFileName, NULL);   //谁 Notepad.exe

		WCHAR TempFileFullPath[MAX_PATH];
		RtlCopyMemory(TempFileFullPath, FileFullPath, FileFullPathLength * sizeof(WCHAR));//操作谁     1.txt

		if (IsSpecialProcess != NULL)
		{
			if ((strlen(ImageFileName) == strlen("explorer.exe")) && !_strnicmp(ImageFileName, "explorer.exe", strlen(ImageFileName)))
			{
				*IsSpecialProcess = EXPLORER_PROCESS;
			}
			else
			{
				*IsSpecialProcess = NORMAL_PROCESS;
				if ((strlen(ImageFileName) == strlen("excel.exe")) && !_strnicmp(ImageFileName, "excel.exe", strlen(ImageFileName)))
				{
					*IsSpecialProcess = EXCEL_PROCESS;
				}
				if ((strlen(ImageFileName) == strlen("powerpnt.exe")) && !_strnicmp(ImageFileName, "powerpnt.exe", strlen(ImageFileName)))
				{
					*IsSpecialProcess = POWERPNT_PROCESS;
				}
			}
		}
		//将字符串转换为小写形式
		_wcslwr(TempFileFullPath);
		if (wcsstr(TempFileFullPath, L"\\local settings\\temp\\~wrd"))
		{
			IsOk = TRUE;
		}

		//定位到文件路径的末尾
		PWCHAR FileEnd = TempFileFullPath + FileFullPathLength - 1;

		if (FileFullPath[FileFullPathLength - 1] == L'\\')
		{
			IsOk = FALSE;
			__leave;
		}

		while (((TempFileFullPath != FileEnd) && (*FileEnd != L'\\')) && ((*FileEnd) != L'.'))
		{
			FileEnd--;    //获取文件类型
		}

		if ((FileEnd == TempFileFullPath) || (*FileEnd == L'\\'))
		{
			FileEnd[0] = L'.';
			FileEnd[1] = L'\0';
		}

		if ((IsPPTFile != NULL) && !_wcsnicmp(FileEnd, L".ppt", wcslen(L".ppt")))
		{
			*IsPPTFile = TRUE;
		}

		PLIST_ENTRY ListEntry = __ListHeadOfMonitorProcess.Flink;
		PPROCESS_INFORMATION ProcessInfo;
		while (&__ListHeadOfMonitorProcess != ListEntry)
		{
			ProcessInfo = CONTAINING_RECORD(ListEntry, PROCESS_INFORMATION, ListEntry);
			if (!_strnicmp(ProcessInfo->ImageFileName, ImageFileName, strlen(ImageFileName)))
			{
				int Index = 0;
				if (ProcessInfo->FileType[0][0] == L'\0')
				{
					IsOk = ProcessInfo->IsMonitor;
					leave;
				}
				while (TRUE)
				{
					if (ProcessInfo->FileType[Index][0] == L'\0')
					{
						IsOk = FALSE; // 当前请求的发起者在监控列表中,但是操作的文件类型不在监控列表中
						break;
					}
					else if ((wcslen(FileEnd) == wcslen(ProcessInfo->FileType[Index])) && !_wcsnicmp(FileEnd, ProcessInfo->FileType[Index], wcslen(FileEnd)))
					{
						IsOk = ProcessInfo->IsMonitor;  //获取文件类型是否属于监控范围内
						break;
					}
					Index++;
				}
				__leave;
			}
			ListEntry = ListEntry->Flink;
		}
		IsOk = FALSE;    //当前请求的发起者不在监控列表中
	}
	__finally 
	{

	}
	return IsOk;
}
