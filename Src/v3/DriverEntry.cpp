#include "DriverEntry.h"
#include "FsProctect.cpp"
#include "PsProtect.cpp"
#include "MsFiliter.cpp"
#include "RegMonitor.cpp"

/************************************************************************************************************************************
		 
															Author: Jin Liang
				
		1.文件读写过滤

		 HarddiskVolume2 -> E盘: 禁止只读属性文件删除操作

		 HarddiskVolume3 -> F盘: 拒绝对 "\保护目录" 的访问
								 禁止非Explorer进程树所有操作
								 禁止非Explorer进程创建操作
								 禁止所有修改操作
								 保护目录透明加密
					   
		 2.模块加载拦截
  
			执行文件: 黑名单签名：(Tencent, Baidu, Ava)		拦截
            白名单模块：(WebChat, QQ)						放行
			驱动文件: (转移到注册表监控)

		3.注册表监控
   
			启动项: 
				禁止创建:
						1.HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run
						2.HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Run
						3.HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Run

			服务项:
				拦截:
					1.HKEY_LOCAL_MACHINE\SYSTEM\ControlSet\services\;	Boot Type: Auto

		Minifiliter:	很惭愧，我只是做了一点微小的工作。

************************************************************************************************************************************/


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
   	NTSTATUS ntStatus = STATUS_SUCCESS;


	#ifdef _DEBUG
	DriverObject->DriverUnload = DriverUnload;
	#endif

	/*开启磁盘保护*/

	InstallDiskProtect(DriverObject);
	
	DbgPrint("[Anti-BTA] fsProtect Installed!\n");

	/*开启模块信任检查*/

	InstallModuleFiliter();
	
	DbgPrint("[Anti-BTA] PsMonitor Installed!\n");

	/*开启注册表监控*/

	InstallRegMonitor(DriverObject);

	DbgPrint("[Anti-BTA] CmpRegister Installed!\n");

	/*开启进程保护*/

	InstallProcessProtect();

	DbgPrint("[Anti-BTA] PsProcect Enabled!\n");
	

	DbgPrint("[Anti-BTA] Passed!\n");

	return ntStatus;
}



VOID
DriverUnload (PDRIVER_OBJECT DriverObject)
{
	UnInstallModuleFiliter();

	UnInstallRegMonitor();

	UnInstallProcessProtect();

	DbgPrint("[Anti-BTA] Excited!\n");
}
