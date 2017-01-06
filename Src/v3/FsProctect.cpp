#include "FsProctect.h"


const FLT_OPERATION_REGISTRATION Callbacks[] = 
{
	{ IRP_MJ_CREATE, 0, MiniPreCreateNotify,	MiniPostCreateNotify},
	{ IRP_MJ_WRITE,  0, MiniPreWriteNotify,		MiniPostWriteNotify},
	{ IRP_MJ_READ,	 0, MiniPreReadNotify,		MiniPostReadNotify},
	{ IRP_MJ_OPERATION_END }
};

const FLT_CONTEXT_REGISTRATION Contextbacks[] = 
{
	{ FLT_FILE_CONTEXT, 0, NULL,	sizeof(FILE_CONTEXT),	'FLCT'},
	{ FLT_CONTEXT_END } 
};

FLT_REGISTRATION FilterRegistration = 
{
	sizeof( FLT_REGISTRATION ),														//  Size
	FLT_REGISTRATION_VERSION,														//  Version
	FLTFL_REGISTRATION_DO_NOT_SUPPORT_SERVICE_STOP,												//  Flags
	Contextbacks,																	//  Context
	Callbacks,																//  Operation Callbacks
	OnUnload,																//  MiniFilterUnload
	InstanceSetupCallback,															//  InstanceSetup
	NULL,																	//  InstanceQueryTeardown
	NULL,																	//  InstanceTeardownStart
	InstanceTeardownCallback,														//  InstanceTeardownComplete
	NULL,																	//  GenerateFileName
	NULL,																	//  GenerateDestinationFileName
	NULL																	//  NormalizeNameComponent
};

VOLUME_CONTEXT	Volume[]	=	
{
	{ E_PAN, NULL, IoModifyAccess },
	{ F_PAN, NULL, IoReadAccess }
};


PFLT_FILTER		FilterHandle					= NULL;
PFLT_VOLUME		KeyVolumeHandle					= NULL;
UCHAR			_KEY [32]						= {0};
extern	HANDLE		TRUST_ID;
extern	HANDLE		ProtectedProcess;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*                                                    微文件系统过滤回调                                                           */
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLT_PREOP_CALLBACK_STATUS
MiniPreCreateNotify (
    PFLT_CALLBACK_DATA       Data,
    PCFLT_RELATED_OBJECTS    FltObjects,
    PVOID                    *CompletionContext
    )
{
	FLT_PREOP_CALLBACK_STATUS		ntStatus	=	FLT_PREOP_SUCCESS_NO_CALLBACK;


	switch(Fs_GetVolumeid(FltObjects->Volume))
	{
		case E_PAN:
		{
			/*	只读属性	*/

			if ((Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & DELETE) || (Data->Iopb->Parameters.Create.Options	>>	24)	==	FILE_SUPERSEDE	)
			{
				FILE_ATTRIBUTE_TAG_INFORMATION	FileAttribute;
				HANDLE							hFile;
				PFILE_OBJECT					FileObject	=	NULL;


				hFile	=	Fs_CreateFile(FltObjects->Volume, FltObjects->FileObject->FileName.Buffer, 0);

				ObReferenceObjectByHandle(
									hFile,
									0,
									*IoFileObjectType,
									KernelMode,
									(void**)&FileObject,
									NULL
									);

				ZwClose(hFile);

				if (FileObject)
				{
					if (NT_SUCCESS(FltQueryInformationFile(FltObjects->Instance, FileObject, &FileAttribute, sizeof(FileAttribute), FileAttributeTagInformation, NULL)))
					{
						if (FileAttribute.FileAttributes & FILE_ATTRIBUTE_READONLY)	

							goto	Failed;
					}

					ObDereferenceObject(FileObject);
				}
			}
			
			break;

		}

		case F_PAN:
		{
			/*	检测进程是否已授权	*/

			UCHAR					CreateOption;
			HANDLE					Requestor	=	(HANDLE)FltGetRequestorProcessId(Data);


			if (Requestor != TRUST_ID && PsGetProcessInheritedFromUniqueProcessId(FltGetRequestorProcess(Data)) != TRUST_ID)
				
				goto	Failed;

			/*	检测操作行为 */

			CreateOption = Data->Iopb->Parameters.Create.Options >> 24;

			/*	禁止删除 */

			if (Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess	&	DELETE)

				goto	Failed;

			/*	禁止覆盖 */

			if ( CreateOption	==	FILE_SUPERSEDE ||  CreateOption	&	(FILE_OVERWRITE	& FILE_OVERWRITE_IF) )
			
				goto	Failed;

			if ( (CreateOption & FILE_OPEN_IF)	==	FILE_OPEN_IF )
			{
				HANDLE	hFile;
				
				
				hFile	=	Fs_CreateFile(FltObjects->Volume, FltObjects->FileObject->FileName.Buffer);

				if (!hFile)
				{
					Data->Iopb->Parameters.Create.Options	&=	~(FILE_OPEN << 24);
					CreateOption	=	FILE_CREATE;
				}
				else
				{
					Data->Iopb->Parameters.Create.Options	&=	~(FILE_CREATE << 24);
					CreateOption	=	FILE_OPEN;
				}

				FltSetCallbackDataDirty(Data);
				ZwClose(hFile);
			}

			if ( CreateOption & FILE_CREATE  )
			{
				/*	非EXPLORER禁止创建	*/

				if (Requestor != TRUST_ID)

					goto	Failed;
			}
			else
			{
				/*	白名单放行 */

				if (RtlEqualUnicodeString(&FltObjects->FileObject->FileName, WhiteList, wsize(WhiteList), TRUE))
			
					break;

				/*	禁止写入 */

				Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess &= ~FileModfityAccess;
				Data->Iopb->Parameters.Create.SecurityContext->AccessState->RemainingDesiredAccess &= ~FileModfityAccess;
			}

			/*	重要文件保护 */

			if (RtlPrefixUnicodeString(&FltObjects->FileObject->FileName, ProcDirctory, wsize(ProcDirctory) + (FltObjects->FileObject->FileName.Length < wsize(ProcDirctory) ? -2 : 0), FALSE))
			{
				if (IsProtectEnabled)
						
					goto Failed;

				ntStatus	=	FLT_PREOP_SUCCESS_WITH_CALLBACK;
			}

			break;

		}

		default:	break;
	}	
	
End:

	return ntStatus;

Failed:
	Data->IoStatus.Status = STATUS_MEDIA_WRITE_PROTECTED;
	Data->IoStatus.Information = 0;
	ntStatus = FLT_PREOP_COMPLETE;
	goto End;
}



FLT_POSTOP_CALLBACK_STATUS
MiniPostCreateNotify (
    PFLT_CALLBACK_DATA       Data,
    PCFLT_RELATED_OBJECTS    FltObjects,
    PVOID                    CompletionContext,
    FLT_POST_OPERATION_FLAGS Flags
    )
{
		if (!NT_SUCCESS(Data->IoStatus.Status) ||  Data->IoStatus.Status ==	STATUS_REPARSE)

			return FLT_POSTOP_FINISHED_PROCESSING;

		
		PFILE_CONTEXT					thisObject	=	NULL;


		FltGetFileContext(FltObjects->Instance, FltObjects->FileObject, (PFLT_CONTEXT*)&thisObject);

		if (!thisObject)
		{
			FltAllocateContext(FltObjects->Filter, FLT_FILE_CONTEXT, sizeof(FILE_CONTEXT), NonPagedPool, (PFLT_CONTEXT*)&thisObject);

			if (!thisObject)

				goto	Failed;

			FltSetFileContext(FltObjects->Instance, FltObjects->FileObject, FLT_SET_CONTEXT_REPLACE_IF_EXISTS, thisObject, NULL);

			thisObject->Policy	=	0;
		}

		thisObject->Policy	|=	IsEncodedFile;

		FltReleaseContext(thisObject);

		if (NT_SUCCESS(PsLookupProcessByProcessId(TRUST_ID, (PEPROCESS*)&ProtectedProcess)))

			ObDereferenceObject(ProtectedProcess);

End:
		return FLT_POSTOP_FINISHED_PROCESSING;

Failed:
		FltCancelFileOpen(FltObjects->Instance, FltObjects->FileObject);
		Data->IoStatus.Information	=	0;
		Data->IoStatus.Status	=	STATUS_UNSUCCESSFUL;
		goto	End;
}



FLT_PREOP_CALLBACK_STATUS
MiniPreWriteNotify (
    PFLT_CALLBACK_DATA       Data,
    PCFLT_RELATED_OBJECTS    FltObjects,
    PVOID                    *CompletionContext
    )
{
		FLT_PREOP_CALLBACK_STATUS		ntStatus	=	FLT_PREOP_SUCCESS_NO_CALLBACK;
		PFILE_CONTEXT				thisObject	=	NULL;


		FltGetFileContext(FltObjects->Instance, FltObjects->FileObject, (PFLT_CONTEXT*)&thisObject);

		if (!thisObject)
		
			return	ntStatus;
		
		/* 文件加密 */
				
		ULONG							length;
		PVOID							origBuf;
		PVOID							newBuf	=	NULL;
		PMDL							newMdl	=	NULL;

		
		if (thisObject->Policy & IsEncodedFile)
		{
			if ((Data->Iopb->IrpFlags & CacheManagerOperation)	==	FALSE)
			{
				length = Data->Iopb->Parameters.Write.Length;
			
				newBuf	= ExAllocatePoolWithTag(NonPagedPool, length, 'SWAP');

				if (newBuf	==	NULL)
				
					goto	Failed;

				if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_IRP_OPERATION)) 
				{ 
					newMdl = IoAllocateMdl( 
										newBuf,	
										length,  
										FALSE,  
										FALSE,  
										NULL 
										);  

					if (newMdl	==	NULL) 
	
						goto	Failed;

					MmBuildMdlForNonPagedPool( newMdl );
				}

				if (Data->Iopb->Parameters.Write.MdlAddress != NULL)
				{  
					origBuf = MmGetSystemAddressForMdlSafe( Data->Iopb->Parameters.Write.MdlAddress,  NormalPagePriority );  
  
					if (origBuf	==	NULL) 

						goto	Failed;
				} 
				else 
				
					origBuf = Data->Iopb->Parameters.Write.WriteBuffer;  
				

  
				if (!NT_SUCCESS(XOR_Encode_Decode(
											origBuf, 
											newBuf, 
											length, 
											_KEY, 
											Data->Iopb->Parameters.Write.ByteOffset.QuadPart
											)))
											
					goto	Failed;

			
				Data->Iopb->Parameters.Write.WriteBuffer = newBuf;
				Data->Iopb->Parameters.Write.MdlAddress = newMdl;
				*CompletionContext = newBuf;
				FltSetCallbackDataDirty( Data );

				ntStatus	=	FLT_PREOP_SUCCESS_WITH_CALLBACK;
			}
		}

End:
		FltReleaseContext(thisObject);

		return ntStatus;

Failed:
		if (newBuf != NULL)
  
			ExFreePool( newBuf );  
  
		if (newMdl != NULL)

			IoFreeMdl( newMdl );  

		Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
		Data->IoStatus.Information = 0;
		ntStatus = FLT_PREOP_COMPLETE;
		goto	End;
}



FLT_POSTOP_CALLBACK_STATUS
MiniPostWriteNotify (
    PFLT_CALLBACK_DATA       Data,
    PCFLT_RELATED_OBJECTS    FltObjects,
    PVOID                    CompletionContext,
    FLT_POST_OPERATION_FLAGS Flags
    )
{
	ExFreePool(CompletionContext);

	return FLT_POSTOP_FINISHED_PROCESSING;
}



FLT_PREOP_CALLBACK_STATUS
MiniPreReadNotify (
    PFLT_CALLBACK_DATA       Data,
    PCFLT_RELATED_OBJECTS    FltObjects,
    PVOID                    *CompletionContext
    )
{
		FLT_PREOP_CALLBACK_STATUS		ntStatus	=	FLT_PREOP_SUCCESS_NO_CALLBACK;
		PFILE_CONTEXT				thisObject	=	NULL;

		
		FltGetFileContext(FltObjects->Instance, FltObjects->FileObject, (PFLT_CONTEXT*)&thisObject);

		if (!thisObject)

			return	ntStatus;
		
		/* 文件解密 */
		
		ULONG							length;
		PVOID							newBuf	=	NULL;
		PMDL							newMdl	=	NULL;

		if (thisObject->Policy & IsEncodedFile)
		{
			if ((Data->Iopb->IrpFlags & CacheManagerOperation)	==	FALSE)
			{
				length = Data->Iopb->Parameters.Read.Length;
			
				newBuf	= ExAllocatePoolWithTag(NonPagedPool, length, 'SRAP');

				if (newBuf	==	NULL)
				
					goto	Failed;

				if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_IRP_OPERATION)) 
				{ 
					newMdl = IoAllocateMdl( 
										newBuf,	
										length,  
										FALSE,  
										FALSE,  
										NULL 
										);  

					if (newMdl	==	NULL) 
	
						goto	Failed;

					MmBuildMdlForNonPagedPool( newMdl );
				}
			
				Data->Iopb->Parameters.Read.ReadBuffer = newBuf;
				Data->Iopb->Parameters.Read.MdlAddress = newMdl;
				*CompletionContext = newBuf;
				FltSetCallbackDataDirty( Data );

				ntStatus	=	FLT_PREOP_SUCCESS_WITH_CALLBACK;
			}
		}
		
End:	FltReleaseContext(thisObject);

		return ntStatus;

Failed:
		if (newBuf != NULL)
  
			ExFreePool( newBuf );  
  
		if (newMdl != NULL)

			IoFreeMdl( newMdl );  

		Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
		Data->IoStatus.Information = 0;
		ntStatus = FLT_PREOP_COMPLETE;
		goto	End;
}



FLT_POSTOP_CALLBACK_STATUS
MiniPostReadNotify (
    PFLT_CALLBACK_DATA       Data,
    PCFLT_RELATED_OBJECTS    FltObjects,
    PVOID                    CompletionContext,
    FLT_POST_OPERATION_FLAGS Flags
    )
{
		FLT_POSTOP_CALLBACK_STATUS	ReturnedStatus;

  
		if (!NT_SUCCESS(Data->IoStatus.Status) ||  Data->IoStatus.Status ==	STATUS_REPARSE) 

			goto leave;
  
		FltDoCompletionProcessingWhenSafe( 
										Data,
                                        FltObjects,  
                                        CompletionContext,  
                                        Flags,
                                        (PFLT_POST_OPERATION_CALLBACK)MiniPostReadNotifyWhenSafe,
                                        &ReturnedStatus
										);

leave:
		ExFreePool(CompletionContext);

		return FLT_POSTOP_FINISHED_PROCESSING;
}



FLT_POSTOP_CALLBACK_STATUS
MiniPostReadNotifyWhenSafe (
    PFLT_CALLBACK_DATA       Data,
    PCFLT_RELATED_OBJECTS    FltObjects,
    PVOID                    CompletionContext,
    FLT_POST_OPERATION_FLAGS Flags
    )
{
		PVOID				origBuf;


		if (Data->Iopb->Parameters.Read.MdlAddress != NULL)
		{
			origBuf = MmGetSystemAddressForMdlSafe( Data->Iopb->Parameters.Read.MdlAddress,  NormalPagePriority );  
  
			if (origBuf == NULL)
  
				goto	Failed;
		} 
		else 
		{
			if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) ||  FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_FAST_IO_OPERATION))
			
				origBuf = Data->Iopb->Parameters.Read.ReadBuffer;  
			
			else 
			{  
				FltLockUserBuffer( Data );

				origBuf = MmGetSystemAddressForMdlSafe( Data->Iopb->Parameters.Read.MdlAddress,  NormalPagePriority );  
  
				if (origBuf == NULL)
  
					goto	Failed;
			}
		}
 
		if (!NT_SUCCESS(XOR_Encode_Decode(
										CompletionContext, 
										origBuf, 
										(ULONG)Data->IoStatus.Information, 
										_KEY, 
										Data->Iopb->Parameters.Read.ByteOffset.QuadPart
										)))
										
			goto	Failed;

End:
	
		return	FLT_POSTOP_FINISHED_PROCESSING;
		
Failed:
		Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
		Data->IoStatus.Information = 0;
		goto	End;
}



NTSTATUS 
InstanceSetupCallback(
	   PCFLT_RELATED_OBJECTS    FltObjects,
	   FLT_INSTANCE_SETUP_FLAGS Flags,
	   DEVICE_TYPE              VolumeDeviceType,
	   FLT_FILESYSTEM_TYPE      VolumeFilesystemType
)
{
	NTSTATUS                         		ntStatus	=	STATUS_FLT_DO_NOT_ATTACH;
	PDEVICE_OBJECT					VolumeDeviceObject	=	NULL;
	VOLUMEID						Vid;
	UNICODE_STRING					VolumeName;


	if (VolumeDeviceType ==	FILE_DEVICE_DISK_FILE_SYSTEM)
	{
		FltGetDiskDeviceObject(FltObjects->Volume, &VolumeDeviceObject);

		if (VolumeDeviceObject)
		{
			if (NT_SUCCESS(IoVolumeDeviceToDosName(VolumeDeviceObject, &VolumeName)))
			{	
				Vid	=	(VOLUMEID)((USHORT)VolumeName.Buffer[0] & ~0x60);
			
				ntStatus	=	Fs_SetVolumeGuid(Vid,	FltObjects->Volume);
			

				/*	解除文件保护  */

				if (NT_SUCCESS(Fs_ReadWriteFile(ReadFile, Fs_CreateFile(FltObjects->Volume, KeyFile), _KEY, sizeof(_KEY))))
				{
					RtlMoveMemory(_KEY + sizeof(_KEY)/2, _KEY, sizeof(_KEY)/2);
					KeyVolumeHandle =	FltObjects->Volume;
					ntStatus	=	STATUS_SUCCESS;
				}
				

				ObDereferenceObject(VolumeDeviceObject);
				ExFreePool(VolumeName.Buffer);
			}
		}
	}

	
	return ntStatus;
}



VOID
InstanceTeardownCallback(
		PCFLT_RELATED_OBJECTS       FltObjects,
		FLT_INSTANCE_TEARDOWN_FLAGS Reason
)
{
	/*	文件重新上锁  */

	if (FltObjects->Volume	==	KeyVolumeHandle)
	{
		KeyVolumeHandle = NULL;
		ProtectedProcess	=	NULL;
		RtlZeroMemory(_KEY, sizeof(_KEY));
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*                                                          Fs_子程序                                                             */
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HANDLE
Fs_CreateFile(VOLUMEID	Path,  LPCWSTR	Name, ULONG	DeirsedAccess, ULONG	Disposition)
{
	return	Fs_CreateFile(Fs_GetVolumeGuid(Path), Name, DeirsedAccess, Disposition);
}

HANDLE
Fs_CreateFile(PFLT_VOLUME VolumeGuid,  LPCWSTR	Name, ULONG	DeirsedAccess, ULONG Disposition)
{
/*
		NTSTATUS			ntStatus;
		PFLT_INSTANCE		InstanceHandle	=	NULL;
		HANDLE				hFile	=	NULL;
		OBJECT_ATTRIBUTES	oaName;
		UNICODE_STRING		filename;
		WCHAR				buffer[128];
		IO_STATUS_BLOCK		ioStatus;


		if (!VolumeGuid)
		{
			KdPrint(("Empty Volume Object!\n"));
			return	NULL;
		}

		FltGetBottomInstance(VolumeGuid, &InstanceHandle);

		if (!InstanceHandle)
		{
			KdPrint(("Empty Instance Handle!\n"));
			return	NULL;
		}
		
		RtlInitEmptyUnicodeString(&filename, buffer, sizeof(buffer));
		FltGetVolumeName(VolumeGuid, &filename, NULL);
		RtlAppendUnicodeToString(&filename,	Name);

		InitializeObjectAttributes(
							&oaName,	
							&filename, 
							OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 
							NULL, 
							NULL
							);

		ntStatus = FltCreateFile(
							FilterHandle,
							InstanceHandle,
							&hFile,
							FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE, 
							&oaName,
							&ioStatus,
							NULL, 
							FILE_ATTRIBUTE_NORMAL,
							0, 
							Disposition,
							FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_NO_INTERMEDIATE_BUFFERING,
							NULL,
							0,
							IO_IGNORE_SHARE_ACCESS_CHECK
							);

		FltObjectDereference(InstanceHandle);


		return	hFile;
*/
		NTSTATUS			ntStatus;
		PDEVICE_OBJECT		DeviceObject	=	NULL;
		HANDLE				hFile	=	NULL;
		OBJECT_ATTRIBUTES	oaName;
		UNICODE_STRING		filename;
		WCHAR				buffer[128];
		IO_STATUS_BLOCK		ioStatus;


		if (!VolumeGuid)
		{
			KdPrint(("Empty Volume Object!\n"));
			return	NULL;
		}

		FltGetDeviceObject(VolumeGuid, &DeviceObject);

		if (!DeviceObject)
		{
			KdPrint(("Empty Disk Object!\n"));
			return	NULL;
		}

		ObDereferenceObject(DeviceObject);
		
		DeviceObject	=	IoGetDeviceAttachmentBaseRef(DeviceObject);

		if (!DeviceObject)
		{
			KdPrint(("Empty Device Object!\n"));
			return	NULL;
		}
		
		RtlInitEmptyUnicodeString(&filename, buffer, sizeof(buffer));
		FltGetVolumeName(VolumeGuid, &filename, NULL);
		RtlAppendUnicodeToString(&filename,	Name);

		InitializeObjectAttributes(
							&oaName,	
							&filename, 
							OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 
							NULL, 
							NULL
							);

		ntStatus = IoCreateFileSpecifyDeviceObjectHint(
							&hFile,
							DeirsedAccess | SYNCHRONIZE, 
							&oaName, 
							&ioStatus,
							NULL, 
							FILE_ATTRIBUTE_NORMAL, 
							0, 
							Disposition, 
							FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_NO_INTERMEDIATE_BUFFERING, 
							NULL,
							0,
							CreateFileTypeNone,
							NULL,
							IO_IGNORE_SHARE_ACCESS_CHECK,
							DeviceObject
							);

		ObDereferenceObject(DeviceObject);


		return	hFile;
}



NTSTATUS 
Fs_ReadWriteFile(BOOLEAN Operation, HANDLE hFile, PVOID lpBuffer,  ULONG Length)
{
		NTSTATUS			ntStatus;
		
			
		switch(Operation)
		{
			case ReadFile:
				ntStatus = ZwReadFile(
								hFile,
								NULL,
								NULL,
								NULL,
								NULL,
								lpBuffer,
								Length,
								NULL,
								NULL
								);
				break;

			case WriteFile:
				ntStatus = ZwWriteFile(
								hFile,
								NULL,
								NULL,
								NULL,
								NULL,
								lpBuffer,
								Length,
								NULL,
								NULL
								);
				break;
		}

		ZwClose(hFile);
		

		return ntStatus;
}



VOLUMEID
Fs_GetVolumeid(PFLT_VOLUME	Vol)
{
	for (int i = 0; i < sizeof(Volume)/sizeof(VOLUME_CONTEXT); i++)
	{
		if (Volume[i].Volume == Vol)

			return	Volume[i].id;
	}

	return	NOPAN;
}



PFLT_VOLUME
Fs_GetVolumeGuid(VOLUMEID	id)
{
	for (int i = 0; i < sizeof(Volume)/sizeof(VOLUME_CONTEXT); i++)
	{
		if (Volume[i].id == id)

			return	Volume[i].Volume;
	}

	return	NULL;
}



NTSTATUS	
Fs_SetVolumeGuid(VOLUMEID	id,	PFLT_VOLUME	Vol)
{
	for (int i = 0; i < sizeof(Volume)/sizeof(VOLUME_CONTEXT); i++)
	{
		if (Volume[i].id == id)
		{
			Volume[i].Volume	=	Vol;

			return	STATUS_SUCCESS;
		}
	}

	return	STATUS_FLT_DO_NOT_ATTACH;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*                                                        安装与卸载例程                                                           */
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS 
InstallDiskProtect(PDRIVER_OBJECT DriverObject)
{
	NTSTATUS			 ntStatus;


	if (DriverObject->DriverUnload)
		
		FilterRegistration.Flags = 0;

	ntStatus = FltRegisterFilter(DriverObject, &FilterRegistration, &FilterHandle);

    if (NT_SUCCESS(ntStatus)) 
	{
		ntStatus = FltStartFiltering(FilterHandle);

		if (!NT_SUCCESS(ntStatus))
		
			FltUnregisterFilter(FilterHandle);
	}

	
	return ntStatus;
}


VOID UnInstallDiskProtect()
{
	FltUnregisterFilter(FilterHandle);
		
	FilterHandle	=	NULL;
}


NTSTATUS
OnUnload (FLT_FILTER_UNLOAD_FLAGS Flags)
{
	UnInstallDiskProtect();

    return STATUS_SUCCESS;
}


