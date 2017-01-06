#include "RegMonitor.h"

LARGE_INTEGER  Cookie;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*														    注册表监控                                                             */
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS CmpRegCallBack(
					IN PVOID               CallbackContext, 
					IN PVOID               REQ_TYPE_ENUM, 
					IN PVOID               Context
					)
{
	NTSTATUS						ntStatus = STATUS_SUCCESS;
	PUNICODE_STRING					KeyName;


	switch((REG_NOTIFY_CLASS)(ULONG)REQ_TYPE_ENUM) 
	{
		case RegNtSetValueKey:
		{
			PREG_SET_VALUE_KEY_INFORMATION	&SetKeyContext	=	(PREG_SET_VALUE_KEY_INFORMATION &)Context;
			USHORT							KeyNameLength;


			if (!NT_SUCCESS(CmCallbackGetKeyObjectID(&Cookie,SetKeyContext->Object, NULL, (PCUNICODE_STRING *)&KeyName)))
				
					break;


			switch(KeyNameLength	=	KeyName->Length)
			{
				/*启动项拦截*/

				case wsize(x32Run) + wsize(Machine):
				case wsize(x32Run) + wsize(User):

				{
					if (ho_memcmp(KeyName->Buffer, x32Run, KeyNameLength, wsize(x32Run)) == wsize(x32Run))
			
						goto Failed;
			
					break;
				}

				case wsize(x64Run) + wsize(Machine):

				{
					if(ho_memcmp(KeyName->Buffer, x64Run, KeyNameLength, wsize(x64Run)) == wsize(x64Run))

						goto Failed;

					break;
				}
		
				/*服务项拦截*/

				default:
					
					BOOLEAN		ServicePath	=	FALSE;

					if (KeyNameLength > wsize(System1) + wsize(Machine))
					{
						if	(hi_memcmp(&KeyName->Buffer[wsize(Machine)/2], System1, wsize(System1)) == wsize(System1))
						{
							ServicePath	=	TRUE;	
						}
						else
						{
							if	(hi_memcmp(&KeyName->Buffer[wsize(Machine)/2], System2_1, wsize(System2_1)) == wsize(System2_1))
							{
								if (hi_memcmp(&KeyName->Buffer[wsize(Machine)/2 + wsize(System2_1)/2 + 1], System2_2, wsize(System2_2)) == wsize(System2_2))

									ServicePath	=	TRUE;
							}
						}
					}
					if (!ServicePath || !SetKeyContext->ValueName)
					
						break;
					

					switch(SetKeyContext->Type) 
					{
						case REG_DWORD:

							/*启动类型过滤*/

							if (SetKeyContext->ValueName->Length == wsize(StartName) && hi_memcmp(SetKeyContext->ValueName->Buffer, StartName, wsize(StartName)) == wsize(StartName))
							{
								__try
								{
									if (*(__int32 *)SetKeyContext->Data < ManualStart)
										
										*(__int32 *)SetKeyContext->Data	=	ManualStart;
										
								}
								__except(EXCEPTION_EXECUTE_HANDLER)
								{
									goto Failed;
								}
							}	
							/*
								//驱动过滤//	
								
								if(SetKeyContext->Type == REG_DWORD && SetKeyContext->ValueName->Length == 8 && hi_memcmp(SetKeyContext->ValueName->Buffer, L"Type", 8) == 8 )
								
									goto	Failed;
							*/
							break;
					}
			}

			break;
		}
	}

End:
		return ntStatus;
Failed:
		ntStatus = STATUS_ACCESS_DENIED;
		goto End;
}


VOID UnInstallRegMonitor()
{
	CmUnRegisterCallback(Cookie);
}

NTSTATUS InstallRegMonitor()
{
	NTSTATUS	ntStatus;

	ntStatus = CmRegisterCallback((PEX_CALLBACK_FUNCTION)CmpRegCallBack, NULL, &Cookie);
	

	return ntStatus;
}