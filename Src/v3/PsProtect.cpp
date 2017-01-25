#include "PsProtect.h"


PVOID	RegistrationHandle	=	NULL;
HANDLE	ProtectedProcess	=	NULL;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*															  ½ø³Ì±£»¤                                                             */
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OB_PREOP_CALLBACK_STATUS 
ObjectRegisterCallBack(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION OperationInformation)
{
	if (ProtectedProcess ==	OperationInformation->Object)
	{
		if (PsGetCurrentProcess() != ProtectedProcess)
		{
			if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE)
	
				OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_NOT_ALLOWED_PRIVILEGE;
		}
	}
    
	return OB_PREOP_SUCCESS;
}

NTSTATUS
InstallProcessProtect()
{
	NTSTATUS	ntStatus;


	OB_CALLBACK_REGISTRATION	CallBackRegistration;
    OB_OPERATION_REGISTRATION	OperationRegistration;

	OperationRegistration.ObjectType = PsProcessType;
    OperationRegistration.Operations = OB_OPERATION_HANDLE_CREATE;
    OperationRegistration.PreOperation = ObjectRegisterCallBack;
	OperationRegistration.PostOperation	=	NULL;

	RtlInitUnicodeString(&CallBackRegistration.Altitude, L"140831");
    CallBackRegistration.Version = OB_FLT_REGISTRATION_VERSION;
    CallBackRegistration.OperationRegistrationCount = 1;
    CallBackRegistration.RegistrationContext = NULL;
	CallBackRegistration.OperationRegistration = &OperationRegistration;

	ntStatus	=	ObRegisterCallbacks(&CallBackRegistration, &RegistrationHandle);


	return	ntStatus;
}

VOID
UnInstallProcessProtect()
{
	if (RegistrationHandle)

		ObUnRegisterCallbacks(RegistrationHandle);
}



