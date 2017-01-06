#include <fltKernel.h>
#include "xCryptgraphy.h"
#include "xOverloading.h"

#pragma comment(lib,"FltMgr.lib")

#define	IsProtectEnabled			(!KeyVolumeHandle)
#define IsEncodedFile				0x10000000
#define	FileModfityAccess			(FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA | FILE_APPEND_DATA | WRITE_DAC | WRITE_OWNER | DELETE | GENERIC_WRITE)
#define	CacheManagerOperation		(IRP_NOCACHE | IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO)

#define	NODELETE					0
#define	DATAENCODE					1
#define	ReadFile					0
#define	WriteFile					1

#define WhiteList					L"\\white_list"
#define ProcDirctory				L"\\±£»¤Ä¿Â¼\\"
#define KeyFile						L"\\DiskGuard.key"


typedef	enum	VOLUMEID { NOPAN, A_PAN, B_PAN, C_PAN, D_PAN, E_PAN, F_PAN };

typedef	struct _FILE_CONTEXT
{
	PKTHREAD		TrustThread;
	ULONG			Policy;
}FILE_CONTEXT, *PFILE_CONTEXT;

typedef	struct _VOLUME_CONTEXT
{
	VOLUMEID			id;
	PFLT_VOLUME		Volume;
	ULONG			Policy;
}VOLUME_CONTEXT, *PVOLUME_CONTEXT;



extern "C"	HANDLE	PsGetProcessInheritedFromUniqueProcessId( IN PEPROCESS Process ); 

FLT_PREOP_CALLBACK_STATUS	MiniPreCreateNotify			(IN OUT PFLT_CALLBACK_DATA Data, IN PCFLT_RELATED_OBJECTS FltObjects, IN OPTIONAL PVOID *CompletionContext);
FLT_POSTOP_CALLBACK_STATUS	MiniPostCreateNotify		(IN OUT PFLT_CALLBACK_DATA Data, IN PCFLT_RELATED_OBJECTS FltObjects, IN OPTIONAL PVOID CompletionContext, IN FLT_POST_OPERATION_FLAGS Flags);
FLT_PREOP_CALLBACK_STATUS	MiniPreWriteNotify			(IN OUT PFLT_CALLBACK_DATA Data, IN PCFLT_RELATED_OBJECTS FltObjects, IN OPTIONAL PVOID *CompletionContext);
FLT_POSTOP_CALLBACK_STATUS	MiniPostWriteNotify			(IN OUT PFLT_CALLBACK_DATA Data, IN PCFLT_RELATED_OBJECTS FltObjects, IN OPTIONAL PVOID CompletionContext, IN FLT_POST_OPERATION_FLAGS Flags);
FLT_PREOP_CALLBACK_STATUS	MiniPreReadNotify			(IN OUT PFLT_CALLBACK_DATA Data, IN PCFLT_RELATED_OBJECTS FltObjects, IN OPTIONAL PVOID *CompletionContext);
FLT_POSTOP_CALLBACK_STATUS	MiniPostReadNotify			(IN OUT PFLT_CALLBACK_DATA Data, IN PCFLT_RELATED_OBJECTS FltObjects, IN OPTIONAL PVOID CompletionContext, IN FLT_POST_OPERATION_FLAGS Flags);
FLT_POSTOP_CALLBACK_STATUS	MiniPostReadNotifyWhenSafe	(IN OUT PFLT_CALLBACK_DATA Data, IN PCFLT_RELATED_OBJECTS FltObjects, IN OPTIONAL PVOID CompletionContext, IN FLT_POST_OPERATION_FLAGS Flags);
VOID						InstanceTeardownCallback	(IN PCFLT_RELATED_OBJECTS FltObjects, IN FLT_INSTANCE_TEARDOWN_FLAGS Reason);
NTSTATUS					InstanceSetupCallback		(IN PCFLT_RELATED_OBJECTS FltObjects, IN FLT_INSTANCE_SETUP_FLAGS Flags, IN DEVICE_TYPE VolumeDeviceType, IN FLT_FILESYSTEM_TYPE VolumeFilesystemType);
NTSTATUS					OnUnload					(IN FLT_FILTER_UNLOAD_FLAGS Flags);
NTSTATUS					InstallDiskProtect			(IN PDRIVER_OBJECT DriverObject);
VOID						UnInstallDiskProtect		();
HANDLE						Fs_CreateFile				(IN	VOLUMEID	Path,		IN LPCWSTR Name,	IN	ULONG	DesiredAccess	=	FILE_READ_DATA | FILE_WRITE_DATA,	IN ULONG Disposition	= FILE_OPEN);
HANDLE						Fs_CreateFile				(IN	PFLT_VOLUME Volume,		IN LPCWSTR Name,	IN	ULONG	DesiredAccess	=	FILE_READ_DATA | FILE_WRITE_DATA,	IN ULONG Disposition	= FILE_OPEN);
NTSTATUS					Fs_ReadWriteFile			(IN BOOLEAN		Operation,	IN HANDLE  Volume,	IN PVOID lpBuffer, IN ULONG Length);
NTSTATUS					Fs_SetVolumeGuid			(IN VOLUMEID	id, IN PFLT_VOLUME Vol);
PFLT_VOLUME					Fs_GetVolumeGuid			(IN VOLUMEID	id);
VOLUMEID					Fs_GetVolumeid				(IN PFLT_VOLUME	Vol);

