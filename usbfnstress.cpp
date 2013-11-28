//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// This source code is licensed under Microsoft Shared Source License
// Version 1.0 for Windows CE.
// For a copy of the license visit http://go.microsoft.com/fwlink/?LinkId=3223.
//
#include "usbfnstress.h"
#include <windows.h>
#include <Usbfnioctl.h>
#include <regext.h>
#include <snapi.h>
#include <msgqueue.h>
#include <pnp.h>

// globals
// Sample global variables.

 UINT g_cycles = 0;
 const GUID FFSGUID;
 // Guid for the USB BUS
#define USBBUS_GUID  L"{E2BDC372-598F-4619-BC50-54B3F7848D35}"

 static const TCHAR c_szDriverRegKey[] =	_T("Drivers\\USB\\FunctionDrivers");
 static const TCHAR c_szFunctionDriver[] =	_T("DefaultClientDriver");
 static const TCHAR c_szRndisFnName[] = 	_T("RNDIS");
 static const TCHAR c_szUsbSerialFnName[] = _T("Serial_Class");
 static const TCHAR c_szUsbMassStorageFnName[] = _T("Mass_Storage_Class");
#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))
 LPCTSTR szOldDriverName = NULL;

 typedef union {
   DEVDETAIL d;
   char pad[sizeof(DEVDETAIL)+MAX_DEVCLASS_NAMELEN];
 } MYDEV;


 HRESULT LoadFnDriver(LPCTSTR );
 HRESULT GetUfnController(__out HANDLE*);
 UINT fnUSBRNDIStest();
 UINT fnUSBserialtest();

UINT fnUSBMassStoretest();
BOOL IsActiveSyncConnected();
BOOL IsMassStoreMapped();


/////////////////////////////////////////////////////////////////////////////////////
BOOL InitializeStressModule (
                            /*[in]*/ MODULE_PARAMS* pmp, 
                            /*[out]*/ UINT* pnThreads
                            )
{
    // save off the command line for this module
   // g_pszCmdLine = pmp->tszUser;
    BOOL bRet = TRUE;
    HRESULT hr = S_OK;
    TCHAR szDriverName[MAX_PATH];

    InitializeStressUtils (
                            _T("USBFNSTRESS"),                               // Module name to be used in logging
                            LOGZONE(SLOG_SPACE_FILESYS, SLOG_DEFAULT),    // Logging zones used by default
                            pmp                                         // Forward the Module params passed on the cmd line
                            );
	//
	// Get current driver from the registry
	//
	hr = RegistryGetString(HKEY_LOCAL_MACHINE, c_szDriverRegKey, c_szFunctionDriver, szDriverName, ARRAYSIZE(szDriverName));
	if(FAILED(hr)){
		bRet = FALSE;
        goto EXIT;
	}
    if (_tcscmp(szDriverName, c_szRndisFnName) == 0)
    {
        szOldDriverName = c_szRndisFnName;
    }
    else if (_tcscmp(szDriverName, c_szUsbSerialFnName) == 0)
    {
        szOldDriverName = c_szUsbSerialFnName;
    }
	else if (_tcscmp(szDriverName, c_szUsbMassStorageFnName) == 0)
    {
        szOldDriverName = c_szUsbMassStorageFnName;
    }
	else
	{
     szOldDriverName = c_szRndisFnName;
	}
EXIT:
    return bRet;
	
}
/////////////////////////////////////////////////////////////////////////////////////
UINT DoStressIteration (
                        /*[in]*/ HANDLE hThread, 
                        /*[in]*/ DWORD dwThreadId, 
                        /*[in,out]*/ LPVOID pv /*unused*/)
{
	ITERATION_INFO* pIter = (ITERATION_INFO*) pv;
	UINT uResult = CESTRESS_PASS;
	RETAILMSG(1,(_T("Thread %i, iteration %i\r\n"), pIter->index, pIter->iteration));

	LogVerbose(_T("Thread %i, iteration %i"), pIter->index, pIter->iteration);


	// TODO:  Do your actual testing here.
	//
	//        You can do whatever you want here, including calling other functions.
	//        Just keep in mind that this is supposed to represent one iteration
	//        of a stress test (with a single result), so try to do one discrete 
	//        operation each time this function is called. 
	switch (Random() % USB_PROFILE_TOTAL)
	{
      case USB_PROFILE_RNDIS:
	  	RETAILMSG(1,(_T("fnUSBRNDIStest\r\n")));
	  	CHCE( fnUSBRNDIStest());
	  	break;
	  case USB_PROFILE_SERIAL_CLASS:
	  	RETAILMSG(1,(_T("fnUSBserialtest\r\n")));
	  	CHCE(fnUSBserialtest());
	  	break;
	  case USB_PROFILE_MASSSTORE_CLASS:
	  	RETAILMSG(1,(_T("fnUSBMassStoretest\r\n")));
		CHCE(fnUSBMassStoretest());
		break;
	 default:
	 	break;
	}
    
	CleanUp:
	// You must return one of these values:
	if(uResult == CESTRESS_PASS)
	{
      RETAILMSG(1,(_T("Iteration[%d] pass!\r\n"),  g_cycles));
	}
	else
	{
	  RETAILMSG(1,(_T("Iteration[%d] fail!\r\n"), g_cycles));
	}
	g_cycles++;
    return  uResult;

}
/////////////////////////////////////////////////////////////////////////////////////
DWORD TerminateStressModule (void)
{
    RegistrySetString(HKEY_LOCAL_MACHINE, c_szDriverRegKey, c_szFunctionDriver, szOldDriverName);
    LoadFnDriver(szOldDriverName);
    Sleep(2000);
    return ((DWORD) -1);
}
///////////////////////////////////////////////////////////
BOOL WINAPI DllMain(
                    HANDLE hInstance, 
                    ULONG dwReason, 
                    LPVOID lpReserved
                    )
{
	LogVerbose(TEXT("DllMain\r\n"));
    return TRUE;
}
UINT fnUSBRNDIStest()
{
HRESULT hr = S_OK;
UINT uResult = CESTRESS_PASS;

  LPCTSTR szNewDriverName = NULL;
  szNewDriverName = c_szRndisFnName;
 // Set a serial driver in the registry
 //
 hr = RegistrySetString(HKEY_LOCAL_MACHINE, c_szDriverRegKey, c_szFunctionDriver, szNewDriverName);
 CHK(hr);

 //
 // Load the driver
 //
 hr = LoadFnDriver(szNewDriverName);
CHK(hr);
Sleep(15000);
if(IsActiveSyncConnected()){
	uResult = CESTRESS_PASS;
	RETAILMSG(1,(_T("fnUSBRNDIStest PASS\r\n")));
}
else {
	uResult = CESTRESS_FAIL;
	RETAILMSG(1,(_T("fnUSBRNDIStest FAIL\r\n")));
}
CleanUp:
if(FAILED(hr))
	uResult = CESTRESS_FAIL;
return uResult;
}

UINT fnUSBserialtest()
{
HRESULT hr = S_OK;
UINT uResult = CESTRESS_PASS;
	 LPCTSTR szNewDriverName = NULL;
	 szNewDriverName = c_szUsbSerialFnName;
	// Set a serial driver in the registry
	//
	hr = RegistrySetString(HKEY_LOCAL_MACHINE, c_szDriverRegKey, c_szFunctionDriver, szNewDriverName);
    CHK(hr);
	//
	// Load the driver
	//
	hr = LoadFnDriver(szNewDriverName);
    CHK(hr);
	Sleep(15000);
if(IsActiveSyncConnected()){
	uResult = CESTRESS_PASS;
	RETAILMSG(1,(_T("fnUSBserialtest PASS\r\n")));
}
else {
	uResult = CESTRESS_FAIL;
	RETAILMSG(1,(_T("fnUSBserialtest FAIL\r\n")));
}
CleanUp:
if(FAILED(hr))
	uResult = CESTRESS_FAIL;
return uResult;
}

UINT fnUSBMassStoretest()
{
	HRESULT hr = S_OK;
	UINT uResult = CESTRESS_PASS;
	 LPCTSTR szNewDriverName = NULL;
	 szNewDriverName = c_szUsbMassStorageFnName;
	// Set a serial driver in the registry
	//
	hr = RegistrySetString(HKEY_LOCAL_MACHINE, c_szDriverRegKey, c_szFunctionDriver, szNewDriverName);
    CHK(hr);
	//
	// Load the driver
	//
	hr = LoadFnDriver(szNewDriverName);
    CHK(hr);
	Sleep(15000);
	if(IsMassStoreMapped()){
		uResult = CESTRESS_PASS;
		RETAILMSG(1,(_T("fnUSBMassStoretest PASS\r\n")));
	}
	else {
		uResult = CESTRESS_FAIL;
		RETAILMSG(1,(_T("fnUSBMassStoretest FAIL\r\n")));
	}
	CleanUp:
	if(FAILED(hr))
		uResult = CESTRESS_FAIL;
	return uResult;
}
HRESULT LoadFnDriver(LPCTSTR pszNewDriver)
{
    HRESULT hr = E_FAIL;
    HANDLE hUfn = INVALID_HANDLE_VALUE;
    UFN_CLIENT_NAME ucn = { 0 };
    // Get the controller handle
    hr = GetUfnController(&hUfn);
    CHK(hr);
    // Change to the new client driver(function driver)
    hr = StringCchCopy(ucn.szName, ARRAYSIZE(ucn.szName), pszNewDriver);
    CHK(hr);
    BOOL fRes = DeviceIoControl(hUfn, IOCTL_UFN_CHANGE_CURRENT_CLIENT, &ucn, sizeof(ucn), NULL, 0, NULL, NULL);
   if(!fRes)
   	{
    hr = E_FAIL;
	goto CleanUp;
   }

CleanUp:
    if (INVALID_HANDLE_VALUE != hUfn) 
    {
        CloseHandle(hUfn);
    }
    return hr;
}
//
// Returns a handle to USB bus controller
//
HRESULT GetUfnController(__out HANDLE* phUfn)
{
    HRESULT hr = S_OK;
    GUID    guidBus;
    HANDLE  hFind = INVALID_HANDLE_VALUE;
    DEVMGR_DEVICE_INFORMATION di = { 0 };
    // Parse the GUID
    hr = CLSIDFromString(USBBUS_GUID, &guidBus);
    CHK(hr);
    // Get a handle to the bus driver
    di.dwSize = sizeof(di);
    hFind = FindFirstDevice(DeviceSearchByGuid, (LPVOID)&guidBus, &di);
    CHH(hFind);
    if(!FindClose(hFind))
   	{
       hr = E_FAIL;
	   goto CleanUp;
	}
    *phUfn = CreateFile(di.szBusName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    CHH(*phUfn);
CleanUp:
    return hr;
}

 BOOL IsActiveSyncConnected()
{
	HKEY  hKey;
	DWORD dwCount;
    DWORD dwValueSize;
    DWORD dwRegType;
	BOOL  bAsync=FALSE;
    RETAILMSG(1,(_T("=======IsActiveSyncConnected?\r\n")));
    Sleep(500);
	if(ERROR_SUCCESS==RegOpenKeyEx(SN_CONNECTIONSDESKTOPCOUNT_ROOT,SN_CONNECTIONSDESKTOPCOUNT_PATH,0,KEY_ALL_ACCESS,&hKey))
	{
		if(ERROR_SUCCESS==RegQueryValueEx(hKey,SN_CONNECTIONSDESKTOPCOUNT_VALUE, NULL, &dwRegType, (BYTE *)&dwCount, &dwValueSize))
		{
			if(dwCount==0){        
                RETAILMSG(1,(_T("=======bAsync = FALSE\r\n")));
				bAsync = FALSE;
           }
			else{     
                RETAILMSG(1,(_T("=======bAsync = TRUE\r\n")));
				bAsync = TRUE;
           }
		}
        else{
               RETAILMSG(1,(_T("=======bAsync Query error\r\n")));   
	    }
	RegCloseKey(hKey);
	}
	return bAsync;
}
BOOL IsMassStoreMapped()
{
BOOL bRet = FALSE;
DWORD dwWait;
DWORD dwTimeout = 15000;
GUID guid = {0x8DD679CE,0x8AB4,0x43c8,0xA1,0x4A,0xEA,0x49,0x63,0xFA,0xA7,0x15};
HANDLE hq, hn;
MSGQUEUEOPTIONS msgopts = {0};
msgopts.dwSize=sizeof(msgopts); 
msgopts.dwFlags=MSGQUEUE_NOPRECOMMIT;
msgopts.dwMaxMessages=0;
msgopts.cbMaxMessage=1024;
msgopts.bReadAccess = TRUE;
hq = CreateMsgQueue(NULL, &msgopts);
RETAILMSG(1,(_T("Created message queue,h = %08x,error = %d\r\n"),hq,GetLastError()));
if(hq == 0) return FALSE;
hn = RequestDeviceNotifications(&guid, hq, TRUE);
RETAILMSG(1,(_T("Registered for notifications, h = %08X\r\n"),hn));
	dwWait = WaitForSingleObject(hq, dwTimeout);
	if(dwWait== WAIT_OBJECT_0){
	RETAILMSG(1,(_T("Get notification signal!\r\n"),hn));
	bRet = TRUE;
	}
	else if(dwWait== WAIT_TIMEOUT){
	RETAILMSG(1,(_T("Get notification Timeout!\r\n"),hn));
    bRet = FALSE;
	}
StopDeviceNotifications(hn);
CloseMsgQueue(hq);
RETAILMSG(1,(_T("IsMassStoreMapped return %d\r\n"),bRet));
return bRet;
}

