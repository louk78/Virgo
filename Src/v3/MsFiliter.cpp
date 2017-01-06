#include "MsFiliter.h"

__int32			TRUST_HASH	=	0;
HANDLE			TRUST_ID	=	NULL;
UCHAR           TRUST_MODULE [256];

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*                                                        进程加载监控                                                             */
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
imCreateProcessNotify(
	HANDLE                 ParentId, 
	HANDLE                 ProcessId,
	PPS_CREATE_NOTIFY_INFO Create
	)
{
	PEPROCESS              KPEPROCESS;
	KAPC_STATE             APC_STATE;
	PIMAGE_NT_HEADERS      HeaderBased;
	__int32                Cache;


	if(Create)
	{
		if (NT_SUCCESS(PsLookupProcessByProcessId(ProcessId, &KPEPROCESS)))
		{
			KeStackAttachProcess (KPEPROCESS, &APC_STATE);

			__try
			{
				HeaderBased = (PIMAGE_NT_HEADERS)RtlImageNtHeader(PsGetProcessSectionBaseAddress(KPEPROCESS));

				/*	授权进程列表	*/
				
				if(!TRUST_ID)
				{
					if (RtlEqualMemory(PsGetProcessImageFileName(KPEPROCESS), TrustProc, csize(TrustProc)))
					{
						if (HeaderBased->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
						{
							Cache = HeaderBased->OptionalHeader.CheckSum;

							if (!TRUST_HASH)
							{
								TRUST_HASH = Cache;
								Fs_ReadWriteFile(WriteFile, Fs_CreateFile(F_PAN, ImageHash, FILE_WRITE_DATA, FILE_OPEN_IF), &TRUST_HASH, sizeof(TRUST_HASH));
							}

							if (Cache && Cache == TRUST_HASH)
						
								TRUST_ID = ProcessId;

							else
						
								Create->CreationStatus = STATUS_ACCESS_DENIED;

						}
					}
				}

				/*	签名检测	*/
			
				

			}
			__finally
			{
				KeUnstackDetachProcess (&APC_STATE);
				ObDereferenceObject (KPEPROCESS);
			}
		}
	}
	else
	{
		if (TRUST_ID == ProcessId)
			
			TRUST_ID = NULL;
	}
}
		

			
VOID UnInstallModuleFiliter()
{
	PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)imCreateProcessNotify, TRUE);
}



NTSTATUS InstallModuleFiliter()
{
	NTSTATUS		ntStatus;

	
	Fs_ReadWriteFile(ReadFile, Fs_CreateFile(F_PAN, WhiteList), &TRUST_MODULE, sizeof(TRUST_MODULE));	
	Fs_ReadWriteFile(ReadFile, Fs_CreateFile(F_PAN, ImageHash), &TRUST_HASH, sizeof(TRUST_HASH));

	ntStatus = PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)imCreateProcessNotify, FALSE);
	

	return ntStatus;
} 