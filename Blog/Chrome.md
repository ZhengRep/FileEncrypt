# Chrome

- PROCESS_INFORMATION

```C++
typedef struct _PROCESS_INFORMATION {
  HANDLE hProcess;
  HANDLE hThread;
  DWORD  dwProcessId;
  DWORD  dwThreadId;
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;
```

Contains information about a newly created process and its primary thread.

- FLT_RELATED_CONTEXTS

```C++
typedef void* FLT_CONTEXT;
typedef struct _FLT_RELATED_CONTEXTS {
  PFLT_CONTEXT VolumeContext; //卷上下文
  PFLT_CONTEXT InstanceContext; //实例上下文
  PFLT_CONTEXT FileContext; //文件上下文
  PFLT_CONTEXT StreamContext;  //文件流上下文
  PFLT_CONTEXT StreamHandleContext; //文件流处理上下文
  PFLT_CONTEXT TransactionContext; //事务上下文
} FLT_RELATED_CONTEXTS, *PFLT_RELATED_CONTEXTS;
```

Contains a minifilter driver's contexts for objects associated with an I/O operation or an instance setup or teardown operation.

> FltGetContexts
>
> FltReleaseContexts
>
> FltAllocateContext

- FltAllocateContext

```c++
NTSTATUS FLTAPI FltAllocateContext(
  [in]  PFLT_FILTER      Filter,
  [in]  FLT_CONTEXT_TYPE ContextType,
  [in]  SIZE_T           ContextSize,
  [in]  POOL_TYPE        PoolType,
  [out] PFLT_CONTEXT     *ReturnedContext
);
//Context Type
FLT_FILE_CONTEXT (starting with Windows Vista)
FLT_INSTANCE_CONTEXT
FLT_SECTION_CONTEXT (starting with Windows 8)
FLT_STREAM_CONTEXT
FLT_STREAMHANDLE_CONTEXT
FLT_TRANSACTION_CONTEXT (starting with Windows Vista)
FLT_VOLUME_CONTEXT   
```

Allocates a context structure for a specified context type.

- FltSetCallbackDataDirty

Indicates that it has modified the contexts of the callback data structure.

```c++
VOID FLTAPI FltSetCallbackDataDirty(
  [in, out] PFLT_CALLBACK_DATA Data
);
```

> FltClearCallbackDataDirty

- FltDoCompletionProcessingWhenSafe

If it is safe to do so, the FltDoCompletionProcessingWhenSafe function executes a minifilter driver's post-callback routine.

```c++
BOOLEAN FLTAPI FltDoCompletionProcessingWhenSafe(
  [in]           PFLT_CALLBACK_DATA           Data, 
  [in]           PCFLT_RELATED_OBJECTS        FltObjects,
  [in, optional] PVOID                        CompletionContext,
  [in]           FLT_POST_OPERATION_FLAGS     Flags,
  [in]           PFLT_POST_OPERATION_CALLBACK SafePostCallback,
  [out]          PFLT_POSTOP_CALLBACK_STATUS  RetPostOperationStatus
);
```

- IoGetDeviceAttachmentBaseRef

Returns a pointer to the lowest-level device in a file system or device driver stack.

- FltQueryInformationFile

Retrieves the information for a given file.

```c++
NTSTATUS FLTAPI FltQueryInformationFile(
  [in]            PFLT_INSTANCE          Instance, 
  [in]            PFILE_OBJECT           FileObject,
  [out]           PVOID                  FileInformation,
  [in]            ULONG                  Length,
  [in]            FILE_INFORMATION_CLASS FileInformationClass,
  [out, optional] PULONG                 LengthReturned
);
```

- FILE_STANDARD_INFORMATION

Is used as an argument to routines that query or set file information.

```c++
typedef struct _FILE_STANDARD_INFORMATION {
  LARGE_INTEGER AllocationSize;
  LARGE_INTEGER EndOfFile;
  ULONG         NumberOfLinks;
  BOOLEAN       DeletePending;
  BOOLEAN       Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;
```

- FILE_POSITION_INFORMATION

```c++
typedef struct _FILE_POSITION_INFORMATION {
  LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;
```

- FLT_PREOP_CALLBACK_STATUS

| Return Status | Intor |
| ----- | ----- |
| FLT_PREOP_COMPLETE | 本层过滤驱动已经完成PreOperation，告诉过滤驱动管理器，不用再调用下层过滤驱动，直接调用上层的PostOperation |
| FLT_PREOP_PENDING | 告诉Filter Manager，本次I/O操作被挂起。之后Filter Manager不再处理I/O操作，直到调用FltCompletePendedPreOperation。 |
| FLT_PREOP_SUCCESS_NO_CALLBACK | 通知Filter Manager已完成本次I/O操作，可以继续处理其他I/O操作，但是不要调用Filter Driver的PostOperation。 |
| FLT_PREOP_SUCCESS_WITH_CALLBACK | 与上类似，只是调用PostOperation。 |
| FLT_PREOP_SYNCHRONIS | The minifilter driver is returning the I/O operation to the filter manager for further processing, but it is not completing the operation. In this case, the filter manager calls the minifilter's post-operation callback in the context of the current thread at IRQL <= APC_LEVEL. |

- FLT_POSTOP_CALLBACK_STATUS

| Return Code                    | Description                                                  |
| ------------------------------ | ------------------------------------------------------------ |
| FLT_POSTOP_FINISHED_PROCESSING | The miniflter driver has finished completion processing and is returning control of I/O operation to filter manager.After the post-operation returns this status value, the filter manager continuous completion processing of the /O operation. |

- PFILE_ALL_INFORMATION

Is container for several FILE_XXX_INFORMATION structures.

```c++
typedef struct _FILE_ALL_INFORMATION {
  FILE_BASIC_INFORMATION     BasicInformation; //文件基本信息
  FILE_STANDARD_INFORMATION  StandardInformation; //标准信息
  FILE_INTERNAL_INFORMATION  InternalInformation; 
  FILE_EA_INFORMATION        EaInformation;
  FILE_ACCESS_INFORMATION    AccessInformation; //访问信息
  FILE_POSITION_INFORMATION  PositionInformation;
  FILE_MODE_INFORMATION      ModeInformation;
  FILE_ALIGNMENT_INFORMATION AlignmentInformation;
  FILE_NAME_INFORMATION      NameInformation;
} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;

typedef struct _FILE_BASIC_INFORMATION {
  LARGE_INTEGER CreationTime; //创建时间
  LARGE_INTEGER LastAccessTime; //最后一次访问时间
  LARGE_INTEGER LastWriteTime; //最后一次写时间
  LARGE_INTEGER ChangeTime;
  ULONG         FileAttributes; //文件属性
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION {
  LARGE_INTEGER AllocationSize; //The multiple of section or luster
  LARGE_INTEGER EndOfFile; //文件尾偏移
  ULONG         NumberOfLinks; //The number of hard links to file.
  BOOLEAN       DeletePending; //TRUE indicates that a file deletion has been requested.
  BOOLEAN       Directory; //TRUE indicates the file object represent the directory.
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct _FILE_POSITION_INFORMATION {
  LARGE_INTEGER CurrentByteOffset; 
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

typedef struct _FILE_NAME_INFORMATION {
  ULONG FileNameLength;
  WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;
```

- FILE_ATTRIBUTES

File attributes are metadata values stored by the file system on disk and are used by the system and are available to developers via various file I/O APIs.

| Constant/value                                    |
| :------------------------------------------------ |
| FILE_ATTRIBUTE_NORMAL  128 (0x80)                 |
| FILE_ATTRIBUTE_ARCHIVE  32 (0x20)                 |
| FILE_ATTRIBUTE_READONLY  1 (0x1)                  |
| FILE_ATTRIBUTE_COMPRESSED  2048 (0x800)           |
| FILE_ATTRIBUTE_DEVICE  64 (0x40)                  |
| FILE_ATTRIBUTE_DIRECTORY  16 (0x10)               |
| FILE_ATTRIBUTE_ENCRYPTED   16384 (0x4000)         |
| FILE_ATTRIBUTE_HIDDEN  2 (0x2)                    |
| FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  8192 (0x2000) |

- FLT_FILE_NAME_INFORMATION

Contains file name information.

```c++
typedef struct _FLT_FILE_NAME_INFORMATION {
  USHORT                     Size;
  FLT_FILE_NAME_PARSED_FLAGS NamesParsed;
  FLT_FILE_NAME_OPTIONS      Format;
  UNICODE_STRING             Name;  //文件名
  UNICODE_STRING             Volume; /
  UNICODE_STRING             Share;
  UNICODE_STRING             Extension;
  UNICODE_STRING             Stream;
  UNICODE_STRING             FinalComponent;
  UNICODE_STRING             ParentDir;
} FLT_FILE_NAME_INFORMATION, *PFLT_FILE_NAME_INFORMATION;
```

