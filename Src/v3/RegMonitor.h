#include "xOverloading.h"

#define Machine		L"\\Registry\\Machine\\"
#define User		L"\\Registry\\User\\"
#define x32Run		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"
#define x64Run		L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run"
#define System1		L"SYSTEM\\CurrentControlSet\\Services\\"
#define System2_1	L"SYSTEM\\ControlSet00"
#define System2_2	L"\\Services\\"
#define	StartName	L"Start"
#define	TypeName	L"Type"

#define	ManualStart	3


NTSTATUS	CmpRegCallBack(IN PVOID CallbackContext, IN PVOID Argument1, IN PVOID Argument2);
NTSTATUS	InstallRegMonitor();
VOID		UnInstallRegMonitor();
