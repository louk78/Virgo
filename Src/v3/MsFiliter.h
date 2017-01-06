#include <ntimage.h>
#include "xOverloading.h"

#define TrustProc					"explorer.exe"
#define WhiteList					L"\\white_list"
#define ImageHash					L"\\no_delete"


extern "C"	NTSTATUS KeUserModeCallback(IN ULONG ApiNumber, IN PVOID InputBuffer, IN ULONG InputLength, OUT PVOID *OutputBuffer, IN PULONG OutputLength);
extern "C"	PVOID	 PsGetProcessSectionBaseAddress(IN PEPROCESS Process);
extern "C"	PUCHAR	 PsGetProcessImageFileName(IN PEPROCESS Process);
extern "C"	PVOID	 RtlImageNtHeader(IN PVOID BaseAdress);

VOID        imCreateProcessNotify	(IN HANDLE ParentId,               IN HANDLE ProcessId, IN OUT PPS_CREATE_NOTIFY_INFO Create);
VOID        imLoadImageNotify		(IN PUNICODE_STRING FullImageName, IN HANDLE ProcessId, IN PIMAGE_INFO ImageInfo);
NTSTATUS	InstallModuleFiliter	();
VOID		UnInstallModuleFiliter	();