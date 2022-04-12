#pragma once
#include "FileHelper.h"
#include "CommonSetting.h"

//status define
#define STATUS_SUCCESS			0x00000000
#define STATUS_EXIST			0x00000001
#define STATUS_ERROR			0x00000002

//Global Vars
LIST_ENTRY					__ListHeadOfMonitorProcess;
KSPIN_LOCK					__SpinLockOfListEntry;
extern PENCRYPT_HEADER		__EncryptHeader = NULL;
