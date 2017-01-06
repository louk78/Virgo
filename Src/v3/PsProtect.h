#define PROCESS_VM_OPERATION               (0x0008)  
#define PROCESS_VM_READ                    (0x0010)  
#define PROCESS_VM_WRITE                   (0x0020)  
#define	PROCESS_NOT_ALLOWED_PRIVILEGE	(PROCESS_VM_OPERATION | PROCESS_VM_READ	| PROCESS_VM_WRITE)


OB_PREOP_CALLBACK_STATUS	ObjectRegisterCallBack(IN PVOID RegistrationContext, IN POB_PRE_OPERATION_INFORMATION OperationInformation);
NTSTATUS					InstallProcessProtect();
VOID						UnInstallProcessProtect();
