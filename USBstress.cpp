//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// This source code is licensed under Microsoft Shared Source License
// Version 1.0 for Windows CE.
// For a copy of the license visit http://go.microsoft.com/fwlink/?LinkId=3223.
//

#include "CRC32.hpp"
#include <stressutils.h>
#include "usb100.h"
///////////////////////////////////////////////////////////////////////////////
//
// @doc SAMPLESTRESSDLL
//
//
// @topic Dll Modules for CE Stress |
//
//	The simplest way to implement a CE Stress module is by creating a DLL
//  that can be managed by the stress harness.  Each stress DLL is loaded and 
//	run by a unique instance of the container: stressmod.exe.  The container
//  manages the duration of your module, collects and reports results to the
//  harness, and manages multiple test threads.
//
//	Your moudle is required to export an initialization function (<f InitializeStressModule>)
//	and a termination function (<f TerminateStressModule>).  Between these calls 
//  your module's main test function (<f DoStressIteration>) will be called in a 
//  loop for the duration of the module's run.  The harness will aggragate and report
//  the results of your module's run based on this function's return values.
//
//	You may wish to run several concurrent threads in your module.  <f DoStressIteration>
//	will be called in a loop on each thread.  In addition, you may implement per-thread
//  initialization and cleanup functions (<f InitializeTestThread> and <f CleanupTestThread>).
//	  
//	<nl>
//	Required functions:
//
//    <f InitializeStressModule> <nl>
//    <f DoStressIteration> <nl>
//    <f TerminateStressModule>
//
//  Optional functions:
//
//    <f InitializeTestThread> <nl>
//    <f CleanupTestThread>
//

//
// @topic Stress utilities |
//
//	Documentation for the utility functions in StressUtils.Dll can be
//  found on:
//
//     \\\\cestress\\docs\\stresss <nl>
//     %_WINCEROOT%\\private\\test\\stress\\stress\\docs
//
//
// @topic Sample code |
//
//	Sample code can be found at: 
//       <nl>\t%_WINCEROOT%\\private\\test\\stress\\stress\\samples <nl>
//
//	Actual module examples can be found at: 
//       <nl>%_WINCEROOT%\\private\\test\\stress\\stress\\modules
//
 enum {
SECOND0P5S =0,
SECOND1S,
SECOND2S,
SECOND3S,
SECOND4S,
SECOND5S,
SECOND10S,
SECOND_NUM
};
#define IOCTL_USBOTG_HCD_GET_NODEINFO   \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x080C, METHOD_BUFFERED, FILE_ANY_ACCESS)
typedef struct USB_NODE_COORDINATE{
	BOOL  m_bConnectionStatus;
	DWORD m_dwRootHub;
	DWORD m_dwTier;
	DWORD m_dwPort;
}USB_NODE_COORDINATE;
typedef struct USBNodeInfo{
	DWORD m_dwRootHub;
	DWORD m_dwTier;
	DWORD m_dwPort;

	DWORD m_dwConnectionStatus; // 0 - disconnected, 1 - connected, 2 - suspended, 3 - Error State
	DWORD m_dwCurrentConfigValue;
	DWORD m_dwDeviceBusSpeed; // 0 - LS, 1 - FS, 2 - HS
	DWORD m_dwDeviceAddress;
	DWORD m_dwOpenPipes;

	TCHAR m_sManufacturer[64];
	TCHAR m_sProduct[64];
	TCHAR m_sSerialNumber[64];
	TCHAR m_sDriverFileName[64];

	DWORD m_dwEndpointsSupportedMax; // Roothub specific feature
	DWORD m_dwUSBError; // Roothub specific feature
	TCHAR m_sUSBErrorDescription[64];// Roothub specific feature
	TCHAR m_sUSBErrorLog[256];// Roothub specific feature

	USB_DEVICE_DESCRIPTOR					m_DeviceDescriptor;
	USB_CONFIGURATION_DESCRIPTOR			m_ConfigDescriptor;
	USB_INTERFACE_DESCRIPTOR				m_InterfaceDescriptor;
	USB_ENDPOINT_DESCRIPTOR 				m_EndpointDescriptor[10];
	USB_HUB_DESCRIPTOR						m_HubDescriptor;
	USB_NODE_COORDINATE 					m_Node[10];
}USBNodeInfo;
//static LPCTSTR szFolderName = TEXT("CESTRESS");
static LPCTSTR szBaseFileExt = TEXT("usbstress.sdt");
typedef struct _fileHeader
{
    DWORD dwFileSize;
    DWORD dwRandSeed;
    DWORD dwCRC;
} FILEHDR, *PFILEHDR;

// globals

HANDLE  g_hInst                         = NULL;
DWORD		g_cycles ;

HANDLE m_hWatchDogAttachEvent= NULL;
HANDLE m_hWatchDogDetachEvent= NULL;
HANDLE m_hstartwriteEvent = NULL;
HANDLE m_hstopwriteEvent = NULL;
static BOOL g_bExitVerifyThread;
static CCrc32 crc;
TCHAR g_szFilename[MAX_PATH];
BOOL g_bStopWrite = FALSE;


USHORT g_VendorID;
USHORT g_ProductID;
TCHAR      g_szTargetStore[MAX_PATH];
PVOID g_pMappedAddress = NULL;
const unsigned BUFFERSIZE = 4096 * 32;
UINT USBWriteThread();
PTCHAR GetTargetStoreName();
static void DeleteReadOnlyFile(  PTCHAR  );
UINT fnFSStressVerify();

UINT fnWriteFileToUSB();
///////////////////////////////////////////////////////////////////////////////
//
// @func	(Required) BOOL | InitializeStressModule |
//			Called by the harness after your DLL is loaded.
// 
// @rdesc	Return TRUE if successful.  If you return FALSE, CEStress will 
//			terminate your module.
// 
// @parm	[in] <t MODULE_PARAMS>* | pmp | Pointer to the module params info 
//			passed in by the stress harness.  Most of these params are handled 
//			by the harness, but you can retrieve the module's user command
//          line from this structure.
// 
// @parm	[out] UINT* | pnThreads | Set the value pointed to by this param 
//			to the number of test threads you want your module to run.  The 
//			module container that loads this DLL will manage the life-time of 
//			these threads.  Each thread will call your <f DoStressIteration>
//			function in a loop.
// 		
// @comm    You must call InitializeStressUtils( ) (see \\\\cestress\\docs\\stress\\stressutils.hlp) and 
//			pass it the <t MODULE_PARAMS>* that was passed in to this function by
//			the harness.  You may also do any test-specific initialization here.
//
//
//

BOOL InitializeStressModule (
							/*[in]*/ MODULE_PARAMS* pmp, 
							/*[out]*/ UINT* pnThreads
							)
{
	// save off the command line for this module
	g_pszCmdLine = pmp->tszUser;

	*pnThreads = 1;
	
	// !! You must call this before using any stress utils !!
	DWORD dwIOCTL = IOCTL_USBOTG_HCD_GET_NODEINFO;
		USBNodeInfo USBNodeInfoIn;
		USBNodeInfo* pUSBNodeInfoOut = (USBNodeInfo*)malloc(sizeof(USBNodeInfo));
	
		memset(&USBNodeInfoIn, 0, sizeof(USBNodeInfo));
		memset(pUSBNodeInfoOut, 0, sizeof(USBNodeInfo));
		USBNodeInfoIn.m_dwRootHub = 0;
		USBNodeInfoIn.m_dwTier = 0;
		USBNodeInfoIn.m_dwPort = 1;

   RETAILMSG(1,(_T("USBStressTest InitializeStressModule\r\n")));
	InitializeStressUtils (
							_T("USBStressTest"),									// Module name to be used in logging
							LOGZONE(SLOG_SPACE_GDI, SLOG_FONT | SLOG_RGN),	// Logging zones used by default
							pmp												// Forward the Module params passed on the cmd line
							);

	// Note on Logging Zones: 
	//
	// Logging is filtered at two different levels: by "logging space" and
	// by "logging zones".  The 16 logging spaces roughly corresponds to the major
	// feature areas in the system (including apps).  Each space has 16 sub-zones
	// for a finer filtering granularity.
	//
	// Use the LOGZONE(space, zones) macro to indicate the logging space
	// that your module will log under (only one allowed) and the zones
	// in that space that you will log under when the space is enabled
	// (may be multiple OR'd values).
	//
	// See \test\stress\stress\inc\logging.h for a list of available
	// logging spaces and the zones that are defined under each space.



	// (Optional) 
	// Get the number of modules of this type (i.e. same exe or dll)
	// that are currently running.  This allows you to bail (return FALSE)
	// if your module can not be run with more than a certain number of
	// instances.
	LONG count = GetRunningModuleCount(g_hInst);
	RETAILMSG(1,(_T("Lillian -- Running Modules = %d\r\n"), count));
	LogVerbose(_T("Running Modules = %d"), count);
	// TODO:  Any module-specific initialization here
	g_pMappedAddress = VirtualAlloc( NULL, BUFFERSIZE, MEM_COMMIT, PAGE_READWRITE );
	HANDLE hUSBWriteThread	= CreateThread( 0, 0, (LPTHREAD_START_ROUTINE)USBWriteThread,  NULL, 0, NULL);
	m_hWatchDogAttachEvent = CreateEvent(NULL, FALSE, FALSE, L"ITC_OTG_WATCHDOGATTACH");
	m_hWatchDogDetachEvent = CreateEvent(NULL, FALSE, FALSE, L"ITC_OTG_WATCHDOGDETACH");
	m_hstartwriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hstopwriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	HANDLE hUSB = CreateFile(L"HCD1:", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(hUSB != INVALID_HANDLE_VALUE){
		  	if( DeviceIoControl(hUSB, 
							dwIOCTL, 
							(LPVOID)&USBNodeInfoIn, 
							sizeof(USBNodeInfo), 
							(LPVOID)pUSBNodeInfoOut, 
							sizeof(USBNodeInfo), 
							NULL, 
							NULL)){

			g_VendorID=pUSBNodeInfoOut->m_DeviceDescriptor.idVendor;
			g_ProductID=pUSBNodeInfoOut->m_DeviceDescriptor.idProduct;
			RETAILMSG(1,(_T("g_VendorID=%x\r\n"),g_VendorID));
			RETAILMSG(1,(_T("g_ProductID=%x\r\n"),g_ProductID));
			}
			else{
             RETAILMSG(1,(_T("DeviceIoControl failed!\r\n")));
			 return FALSE;
			}
	  	}
	else{
		RETAILMSG(1,(_T("CreateFile HCD1: failed!\r\n")));
     return FALSE;
	}
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// @func	(Optional) UINT | InitializeTestThread | 
//			Called once at the start of each test thread.  
// 
// @rdesc	Return <t CESTRESS_PASS> if successful.  If your return <t CESTRESS_ABORT>
//			your module will be terminated (although <f TerminateStressModule> 
//			will be called first).
// 
// @parm	[in] HANDLE | hThread | A pseudohandle for the current thread. 
//			A pseudohandle is a special constant that is interpreted as the 
//			current thread handle. The calling thread can use this handle to 
//			specify itself whenever a thread handle is required. 
//
// @parm	[in] DWORD | dwThreadId | This thread's identifier.
//
// @parm	[in] int | index | Zero-based index of this thread.  You can use this
//			for indexing arrays of per-thread data.
// 		
// @comm    There is no required action.  This is provided for test-specific 
//			initialization only.
//
//
//

UINT InitializeTestThread (
						/*[in]*/ HANDLE hThread, 
						/*[in]*/ DWORD dwThreadId, 
						/*[in]*/ int index)
{
	RETAILMSG(1,(_T("Lillian -- InitializeTestThread(), thread handle = 0x%x, Id = %d, index %i\r\n"), hThread,dwThreadId,index));

	LogVerbose(_T("InitializeTestThread(), thread handle = 0x%x, Id = %d, index %i"), 
					hThread, 
					dwThreadId, 
					index 
					);

	return CESTRESS_PASS;
}





///////////////////////////////////////////////////////////////////////////////
//
// @func	(Required) UINT | DoStressIteration | 
//			Called once at the start of each test thread.  
// 
// @rdesc	Return a <t CESTRESS return value>.  
// 
// @parm	[in] HANDLE | hThread | A pseudohandle for the current thread. 
//			A pseudohandle is a special constant that is interpreted as the 
//			current thread handle. The calling thread can use this handle to 
//			specify itself whenever a thread handle is required. 
//
// @parm	[in] DWORD | dwThreadId | This thread's identifier.
//
// @parm	[in] LPVOID | pv | This can be cast to a pointer to an <t ITERATION_INFO>
//			structure.
// 		
// @comm    This is the main worker function for your test.  A test iteration should 
//			begin and end in this function (though you will likely call other test 
//			functions along the way).  The return value represents the result for a  
//			single iteration, or test case.  
//
//
//

// functions

/////////////////////////////////////////////////////////////////////////////////////

PBYTE FillBuffer(  PFILEHDR pFileHdr)
{
    // Randomize the number of bytes to write
    unsigned BytesToWrite = Random() % BUFFERSIZE;
	//RETAILMSG(1,(_T("FillBuffer!!!BytesToWrite = %d\r\n"),BytesToWrite));
    // Randomize the alignment
    PBYTE pBytePtr = (PBYTE)g_pMappedAddress;
    pBytePtr += Random() % (BUFFERSIZE - BytesToWrite);

    pFileHdr->dwFileSize = BytesToWrite;
    pFileHdr->dwRandSeed = GetTickCount();
    srand( pFileHdr->dwRandSeed );
    for ( DWORD i = 0; i < BytesToWrite; i++ )
    {
        *(pBytePtr + i) = rand();
		  // RETAILMSG(1,(_T("*(pBytePtr + %d)=%x\r\n"),i, *(pBytePtr + i)));
    }
    // Calculate the CRC of the buffer
    pFileHdr->dwCRC = crc.CalcCRC( pBytePtr, BytesToWrite );
    return pBytePtr;
}
/////////////////////////////////////////////////////////////////////////////////////
PTCHAR GetTargetStoreName()
{
    // Set the default target mount point
    _tcscpy( g_szTargetStore, TEXT("\\Hard Disk") );

    return g_szTargetStore;
}

UINT USBWriteThread()
{
RETAILMSG(1,(_T("Start USBWriteThread .................\r\n")));
UINT Ret = CESTRESS_FAIL;
//TCHAR szRandomFileName[MAX_PATH];
//FILEHDR fileHdr;
HANDLE hFile = INVALID_HANDLE_VALUE ;
BOOL bStopWrite = FALSE;
BOOL rWriteUSB=FALSE;
        // Get a random but valid FAT filename
	wsprintf(g_szFilename, TEXT("%s\\%s"), GetTargetStoreName(), szBaseFileExt);
        RETAILMSG(1,(_T("g_szFilename = %s\r\n"),g_szFilename));
        HANDLE hEvents[2];
	hEvents[0] = m_hstartwriteEvent;
	hEvents[1] = m_hstopwriteEvent;
        BOOL bSuccess = FALSE;
	DWORD dwWaitObject;
	DWORD dwTimeOut = INFINITE;
		while(1){
	        RETAILMSG(1,(_T("WaitForSingleObjects......\r\n")));
		dwWaitObject =WaitForSingleObject(m_hstartwriteEvent,dwTimeOut);
		switch(dwWaitObject){
		case WAIT_OBJECT_0:
		RETAILMSG(1,(_T("Got m_hstartwriteEvent signaled!\r\n")));
		while(!g_bStopWrite){
			rWriteUSB = fnWriteFileToUSB();
			
		 if ( rWriteUSB )
	        {
	        RETAILMSG(1,(_T("Deltefile in hard disk!\r\n")));
	        DeleteReadOnlyFile( g_szFilename );
	        }
		}
		RETAILMSG(1,(_T("g_bStopWrite is set=%d\r\n"),g_bStopWrite));
        break;
	//	case WAIT_OBJECT_0 + 1:
	//		bStopWrite=TRUE;
	//		RETAILMSG(1,(_T("Got m_hstopwriteEvent signaled!\r\n")));
	//		break;
		case WAIT_TIMEOUT:
			break;
		}
		}

		   RETAILMSG(1,(_T("exit while 1\r\n")));

	    if(hFile!=INVALID_HANDLE_VALUE){
        CloseHandle(hFile);
		}

    return 0;
}
/////////////////////////////////////////////////////////////////////////////////////
UINT fnWriteFileToUSB(){
HANDLE hFile = INVALID_HANDLE_VALUE ;	
FILEHDR fileHdr;
DWORD dwBytesWritten;
BOOL bSuccess = FALSE;
PBYTE pBytePtr = NULL;
			// Create a file
			 hFile= CreateFile(g_szFilename, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			 if ( hFile == INVALID_HANDLE_VALUE )
				{
					DWORD dwError = GetLastError();
				}
			else{		
				// Get random data, size and pointer offset
			    pBytePtr = FillBuffer( &fileHdr);	
	
			   // Write the file header
			   if ( !WriteFile( hFile, &fileHdr, sizeof(fileHdr), &dwBytesWritten, NULL ) )
			   {
					DWORD dwError = GetLastError();
					if ( dwError == ERROR_DISK_FULL )
						{
						 RETAILMSG(1,(_T("FSStressWriteThread: Disk Full!\r\n")));
						 LogFail(TEXT("FSStressWriteThread: Disk Full!\r\n"));
							//	goto EXIT;
						}
					else
						{
							RETAILMSG(1,(_T(" WriteFile header Failure %d\r\n"), dwError));

						}
				}
			   else if ( dwBytesWritten != sizeof(fileHdr) )
			   {
					 RETAILMSG(1,(_T("FS Stress Write File Header Error\r\n")));
					// LogFail(TEXT("FS Stress Write File Header Error\r\n"));		
				}
			   else
			   {
	
				// Write the file body
				//	RETAILMSG(1,(_T("W: Writing %d bytes from %s pointer 0x%08x\r\n"), fileHdr.dwFileSize, szPermissionStrings[permission], pBytePtr));
				//LogVerbose(TEXT("W: Writing %d bytes from %s pointer 0x%08x\r\n"), fileHdr.dwFileSize, szPermissionStrings[permission], pBytePtr);
				// Write the CRC'd data
				   if ( !WriteFile( hFile, pBytePtr, fileHdr.dwFileSize, &dwBytesWritten, NULL ) )
				   {
						DWORD dwError = GetLastError();
						if ( dwError == ERROR_DISK_FULL )
						{
							RETAILMSG(1,(_T(" Disk Full!\r\n")));
							LogFail(TEXT("fnFSStressWrite: Disk Full!\r\n"));
							//goto EXIT;
						}
						else
						{
						  // wsprintf(szRandomFileName, TEXT("FS Stress Write Failure %d"), dwError);
						   RETAILMSG(1,(_T("WriteFile body Failure %d\r\n"), dwError));
						   //LogFail(TEXT("FS Stress Write Failure %d\r\n"), dwError);
						  // goto EXIT;
						}
				   }
				   else if ( dwBytesWritten != fileHdr.dwFileSize)
				   {
				   RETAILMSG(1,(_T("Write File Body Error\r\n")));
					}
				   else
				   {
					RETAILMSG(1,(_T("WriteFile sucess!\r\n")));
					bSuccess = TRUE;
	
					 }
				   }
			}
			if(hFile!=INVALID_HANDLE_VALUE){
			RETAILMSG(1,(_T("CloseHandle hFile.\r\n")));
			CloseHandle(hFile);
			}

return bSuccess;
}



static void DeleteReadOnlyFile(  PTCHAR szFileName )
{
    if ( !DeleteFile( szFileName ) )
    {
        LogFail(_T("DeleteFile failure %d\r\n"), GetLastError());
        // Note: We should check if the delete was on a READ_ONLY file and if so,
        //       a failed DeleteFile is probably the correct response from the FS
        //  Intel claims that it is not; The OS should have enforced...

    }
	else{
     RETAILMSG(1,(_T("DeleteFile %s pass error=%d\r\n"), szFileName,GetLastError()));
	}
}

UINT DoStressIteration (
						/*[in]*/ HANDLE hThread, 
						/*[in]*/ DWORD dwThreadId, 
						/*[in]*/ LPVOID pv)
{
	ITERATION_INFO* pIter = (ITERATION_INFO*) pv;
	RETAILMSG(1,(_T("iteration %d\r\n"),g_cycles));
	g_pMappedAddress = VirtualAlloc( NULL, BUFFERSIZE, MEM_COMMIT, PAGE_READWRITE );
	//CreateDirectory(g_szFilename,NULL);
	HANDLE hFile = INVALID_HANDLE_VALUE;
	BOOL bHardDiskExist = FALSE;
	LogVerbose(_T("Thread %i, iteration %i"), pIter->index, pIter->iteration);
    UINT Results=CESTRESS_PASS;
	DWORD indexSleep =0;
    DWORD dwIOCTL = IOCTL_USBOTG_HCD_GET_NODEINFO;
	USBNodeInfo USBNodeInfoIn;
	USBNodeInfo* pUSBNodeInfoOut = (USBNodeInfo*)malloc(sizeof(USBNodeInfo));
    DWORD time_out_tick_count = 20000;
    DWORD start_tick_count, current_tick_count;
	HANDLE changeHandles = NULL;

	memset(&USBNodeInfoIn, 0, sizeof(USBNodeInfo));
	memset(pUSBNodeInfoOut, 0, sizeof(USBNodeInfo));
	USBNodeInfoIn.m_dwRootHub = 0;
	USBNodeInfoIn.m_dwTier = 0;
	USBNodeInfoIn.m_dwPort = 1;

	// TODO:  Do your actual testing here.
	//
	//        You can do whatever you want here, including calling other functions.
	//        Just keep in mind that this is supposed to represent one iteration
	//        of a stress test (with a single result), so try to do one discrete 
	//        operation each time this function is called. 
	SetEvent(m_hstartwriteEvent);
	Sleep(10000);
	for(int i = 0; i< 20; i++){
		RETAILMSG(1,(_T("%d-th detach/attach\r\n"),i));
     indexSleep  = Random()% SECOND_NUM;
	 RETAILMSG(1,(_T("indexSleep = %d\r\n"),indexSleep));
	SetEvent( m_hWatchDogDetachEvent);
		  switch(indexSleep)
	 		{
	           case SECOND0P5S:
	 		  	Sleep(500);
	 			break;
	 		  case SECOND1S:
	 		  	Sleep(1000);
	 			break;
	 		  case SECOND2S:
	 		  	Sleep(2000);
	 			break;
	 		  case SECOND3S:
	 		  	Sleep(3000);
	 			break;
	 		  case SECOND4S:
	 		  	Sleep(4000);
	 			break;
	 		  case SECOND5S:
	 		  	Sleep(5000);
	 			break;
			  case SECOND10S:
			  	Sleep(10000);
				break;
	 		default:
	 			break;
	 		  }
		  
        	SetEvent(m_hWatchDogAttachEvent);
			indexSleep  = Random()% SECOND_NUM;
			switch(indexSleep)
	 		{
	           case SECOND0P5S:
	 		  	Sleep(500);
	 			break;
	 		  case SECOND1S:
	 		  	Sleep(1000);
	 			break;
	 		  case SECOND2S:
	 		  	Sleep(2000);
	 			break;
	 		  case SECOND3S:
	 		  	Sleep(3000);
	 			break;
	 		  case SECOND4S:
	 		  	Sleep(4000);
	 			break;
	 		  case SECOND5S:
	 		  	Sleep(5000);
	 			break;
			  case SECOND10S:
			  	Sleep(10000);
				break;
	 		default:
	 			break;
	 		  }

	}
	 g_bStopWrite = TRUE;
	SetEvent( m_hWatchDogDetachEvent);
	Sleep(10000);
	SetEvent(m_hWatchDogAttachEvent);
	Sleep(10000);
	HANDLE hUSB = CreateFile(L"HCD1:", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(hUSB != INVALID_HANDLE_VALUE){

		   if( DeviceIoControl(hUSB, 
						   dwIOCTL, 
						   (LPVOID)&USBNodeInfoIn, 
						   sizeof(USBNodeInfo), 
						   (LPVOID)pUSBNodeInfoOut, 
						   sizeof(USBNodeInfo), 
						   NULL, 
						   NULL)){	
						   RETAILMSG(1,(_T("pUSBNodeInfoOut->m_DeviceDescriptor.idVendor=%x\r\n"),pUSBNodeInfoOut->m_DeviceDescriptor.idVendor));
						   RETAILMSG(1,(_T("pUSBNodeInfoOut->m_DeviceDescriptor.idProduct=%x\r\n"),pUSBNodeInfoOut->m_DeviceDescriptor.idProduct));

						   if((pUSBNodeInfoOut->m_DeviceDescriptor.idVendor!= g_VendorID)||(pUSBNodeInfoOut->m_DeviceDescriptor.idProduct!= g_ProductID))	{				
                            RETAILMSG(1,(_T("vendor ID and Product ID don't match prefetch value!\r\n")));
								Results =  CESTRESS_FAIL;
						   	}
						   else{
                              RETAILMSG(1,(_T("vendor ID and Product ID MATCH!\r\n")));
							 start_tick_count = GetTickCount();
							current_tick_count = 0;
							  while( GetFileAttributes(GetTargetStoreName())== 0xffffffff){
							  	RETAILMSG(1,(_T("GetFileAttributes return ffffffff,error=%x.continue to wait...\r\n"),GetLastError()));
                                Sleep(500);
								current_tick_count = GetTickCount() - start_tick_count;
								if(current_tick_count > time_out_tick_count){
									RETAILMSG(1,(_T("GetFileAttributes time out,exit this iteration with failure.\r\n")));
									goto EXIT;
									}
							   } 
						
							RETAILMSG(1,(_T("GetFileAttributes SUCCESS!\r\n")));
							//write file to hard disk again for verify...
							BOOL b= fnWriteFileToUSB();
							if(b){
							RETAILMSG(1,(_T("fnWriteFileToUSB SUCCESS!\r\n")));
							Results = fnFSStressVerify();
							}
							else{
								Results=CESTRESS_FAIL;
								RETAILMSG(1,(_T("FAILED to fnWriteFileToUSB,exit this iteration with failure.\r\n")));
								goto EXIT;							
						   }		
						  }
		   }
		   else{
			RETAILMSG(1,(_T("DeviceIoControl failed!\r\n")));
			Results = CESTRESS_FAIL;
		   }
		   }
	else {
		RETAILMSG(1,(_T("CreateFile HCD1: failed!\r\n")));
       Results=CESTRESS_FAIL;
	}
	EXIT:
		 free(pUSBNodeInfoOut);

	// You must return one of these values:
	if(Results==CESTRESS_PASS)
	RETAILMSG(1,(_T("\r\n\r\n******************************Results[%d]***************************PASS!\r\n\r\n"),g_cycles));
	else if(Results=CESTRESS_FAIL)
	RETAILMSG(1,(_T("\r\n\r\n******************************Results[%d]***************************FAIL!\r\n\r\n"),g_cycles));	
	g_cycles++;
    return Results;
}


/////////////////////////////////////////////////////////////////////////////////////
UINT fnFSStressVerify()
{
RETAILMSG(1,(_T("fnFSStressVerify ENTRY\r\n")));
DWORD dwError;
SHORT i = 0;
UINT Ret = CESTRESS_FAIL;
HANDLE hFile = INVALID_HANDLE_VALUE;
BOOL	bSuccess = FALSE;
DWORD dwoffset = 0;
g_bExitVerifyThread = FALSE;
       
            TCHAR szFullPathName[MAX_PATH] = {0};
           
			wsprintf(szFullPathName, TEXT("%s\\%s"), GetTargetStoreName(), szBaseFileExt);
			RETAILMSG(1,(_T("szFullPathName = %s\r\n"),szFullPathName));
            // Add test here to validate attributes work
            hFile = CreateFile( szFullPathName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		
            if ( hFile == INVALID_HANDLE_VALUE )
            {             
				dwError = GetLastError();
                RETAILMSG(1,(_T("V: Failed to open %s, error = %d\r\n"),szFullPathName, dwError));
		
                goto EXIT;
             }
            if ( hFile != INVALID_HANDLE_VALUE )
            {
                RETAILMSG(1,(_T("CreateFile SUCCESS!\r\n")));
                // Get the header info
                FILEHDR fileHdr;
                DWORD dwBytesRead = 0;
                bSuccess = ReadFile(hFile, &fileHdr, sizeof(fileHdr), &dwBytesRead, NULL);
				RETAILMSG(1,(_T("ReadFile fileHdr return bSuccess =%x\r\n"),bSuccess));
                BYTE buffer[4096];
                bSuccess = ReadFile(hFile, buffer, sizeof(buffer), &dwBytesRead, NULL);
				RETAILMSG(1,(_T("ReadFile buffer return bSuccess =%x\r\n"),bSuccess));
                DWORD dwCalcCrc = 0xFFFFFFFFL;
                while(bSuccess && dwBytesRead)  // While no error and not at EOF
                {
                    for(DWORD j = 0; j < dwBytesRead; j++)
                    {
                        dwCalcCrc = ((dwCalcCrc) >> 8) ^ crc.crcTable[(buffer[j]) ^ ((dwCalcCrc) & 0x000000FF)];
                    }
					RETAILMSG(1,(_T("In while ReadFile buffer before dwCalcCrc =%x\r\n"),dwCalcCrc));
                    bSuccess = ReadFile(hFile, buffer, sizeof(buffer), &dwBytesRead, NULL);
					RETAILMSG(1,(_T("In while ReadFile buffer return bSuccess =%x dwBytesRead=%x\r\n"),bSuccess,dwBytesRead));
                }
				RETAILMSG(1,(_T("exit while crc\r\n")));
                if(!bSuccess)   // If a problem occurred
                {                
                    dwError = GetLastError();
					 RETAILMSG(1,(_T("A problem occurred ERROR=%x\r\n"),dwError));
					goto EXIT;
                }
                CloseHandle(hFile);
                dwCalcCrc ^= 0xFFFFFFFFL; // Invert the value
                 RETAILMSG(1,(_T("dwCalcCrc=%x  fileHdr.dwCRC=%x\r\n"),dwCalcCrc,fileHdr.dwCRC));
                if ( dwCalcCrc == fileHdr.dwCRC )
                {
                   RETAILMSG(1,(_T("CRC's match\r\n")));

                }
                else
                {
                    // Stop the write thread
					RETAILMSG(1,(_T("V:- CRC's do not match!fileHdr.dwCRC=%x\r\n"),fileHdr.dwCRC));
				   goto EXIT;
                }
            }
			
            Ret = CESTRESS_PASS;
EXIT:
	if(hFile!=INVALID_HANDLE_VALUE){
	CloseHandle(hFile);
	}
    DeleteReadOnlyFile( szFullPathName );

    return Ret;
}
///////////////////////////////////////////////////////////////////////////////
//
// @func	(OPTIONAL) UINT | CleanupTestThread | 
//			Called once before each test thread exits.  
// 
// @rdesc	Return a <f CESTRESS return value>.  If you return anything other than
//			<t CESTRESS_PASS> this will count toward total test results (i.e. as
//			a final iteration of your test thread).
// 
// @parm	[in] HANDLE | hThread | A pseudohandle for the current thread. 
//			A pseudohandle is a special constant that is interpreted as the 
//			current thread handle. The calling thread can use this handle to 
//			specify itself whenever a thread handle is required. 
//
// @parm	[in] DWORD | dwThreadId | This thread's identifier.
//
// @parm	[in] int | index | Zero-based index of this thread.  You can use this
//			for indexing arrays of per-thread data.
// 		
// @comm    There is no required action.  This is provided for test-specific 
//			clean-up only.
//
//

UINT CleanupTestThread (
						/*[in]*/ HANDLE hThread, 
						/*[in]*/ DWORD dwThreadId, 
						/*[in]*/ int index)
{
	RETAILMSG(1,(_T("CleanupTestThread(), thread handle = 0x%x, Id = %d, index %i\r\n"), hThread, dwThreadId,index));

	LogComment(_T("CleanupTestThread(), thread handle = 0x%x, Id = %d, index %i"), 
					hThread, 
					dwThreadId, 
					index 
					);

	return CESTRESS_WARN2;
}



///////////////////////////////////////////////////////////////////////////////
//
// @func	(Required) DWORD | TerminateStressModule | 
//			Called once before your module exits.  
// 
// @rdesc	Unused.
// 		
// @comm    There is no required action.  This is provided for test-specific 
//			clean-up only.
//

DWORD TerminateStressModule (void)
{
	// TODO:  Do test-specific clean-up here.


	// This will be used as the process exit code.
	// It is not currently used by the harness.

	return ((DWORD) -1);
}



///////////////////////////////////////////////////////////
BOOL WINAPI DllMain(
					HANDLE hInstance, 
					ULONG dwReason, 
					LPVOID lpReserved
					)
{
	g_hInst = hInstance;
    return TRUE;
}

