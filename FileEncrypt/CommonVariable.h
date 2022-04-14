#pragma once
#include "FileHelper.h"
#include "CommonSetting.h"

//status define
#define STATUS_SUCCESS			0x00000000
#define STATUS_EXIST			0x00000001
#define STATUS_ERROR			0x00000002

#define NORMAL_PROCESS			0
#define EXPLORER_PROCESS		1
#define EXCEL_PROCESS			2
#define POWERPNT_PROCESS		3

//Global Vars
extern LIST_ENTRY					__ListHeadOfMonitorProcess;
extern KSPIN_LOCK					__SpinLockOfListEntry;
extern PENCRYPT_HEADER				__EncryptHeader;
extern NPAGED_LOOKASIDE_LIST		__NpagedLookasideListOfPrevious2Post;
extern UCHAR						__Signature[SIGNATURE_LENGTH];