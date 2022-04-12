#Chrome

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
