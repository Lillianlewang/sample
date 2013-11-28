//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// This source code is licensed under Microsoft Shared Source License
// Version 1.0 for Windows CE.
// For a copy of the license visit http://go.microsoft.com/fwlink/?LinkId=3223.
//

//#include <ws2tcpip.h>


#include <windows.h>
//#include <winbase.h>

#include <initguid.h>

#include <tchar.h>
#include <stressutils.h>
#include <commctrl.h>
#include <wininet.h>
#include <Iphlpapi.h>
#include <wtypes.h>


#include <connmgr.h>
#include <Connmgr_status.h>
#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>

#define STATUS_FREQUENCY		1000		// print out our status every time this number
											// of iterations has been performed
#define DEFAULT_WAITTIME		50			// Default time to wait (in ms) between binds.
											// This is used to lessen the chance that
											// multiple, concurrent bind tests will
											// interfere with one another and to prevent
											// the test from cycling thru the ports too
											// quickly (WSAEADDRINUSE).

#define MIN_PORT				1			// Defines the port range in which to bind
#define MAX_PORT				65535		// sockets

#define MAX_ADDRS				100			// Maximum number of addresses to choose
#define DEFAULT_METRIC_VALUE 4294967295											// from for binding purposes
// Function Prototypes
BOOL Setup(void);
UINT DoStressIteration (void);
BOOL Cleanup(void);
UINT PutInternetFile(LPCTSTR , LPTSTR );
UINT GetInternetFile(LPCTSTR , LPTSTR );

DWORD GetDefaultInterface(TCHAR *, TCHAR *);
void FillDefaults(MIB_IPFORWARDROW* );
void CloseTheHandles();
void ReadRouteValue(TCHAR *, TCHAR *,TCHAR *, TCHAR *,TCHAR *, TCHAR *, TCHAR *);
void WriteResultsToRegistry(STRESS_RESULTS);

// Global Variables
MIB_IPFORWARDROW routeinfo;
HINTERNET	hOpen ;
HINTERNET	hConnect ;
HINTERNET	hRequest ;
UINT i=0;
TCHAR FTPServerEthernetUsername[255];
TCHAR FTPServerEthernetPassword[255];
TCHAR FTPServerWiFiUsername[255];
TCHAR FTPServerWiFiPassword[255];
TCHAR FTPServerWiFiIP[255];
TCHAR FTPServerEthernetIP[255];
TCHAR EthernetCardName[255];


//
//***************************************************************************************
void DottedNotationToDword( TCHAR *abc, DWORD* decvalue) 

{
    char tempstr[16]={0};

#ifdef UNICODE
    // convert from Unicode
    wcstombs(tempstr,abc,sizeof(tempstr));
#else
    strncpy(tempstr,abc,sizeof(tempstr)-1);
#endif

    *decvalue = inet_addr(tempstr);
}

int WINAPI WinMain ( 
					HINSTANCE hInstance,
					HINSTANCE hInstPrev,
#ifdef UNDER_CE
					LPWSTR wszCmdLine,
#else
					LPSTR szCmdLine,
#endif
					int nCmdShow
					)
{
	//DWORD			dwWaitTime = 0;
	DWORD dwStartTime = GetTickCount();
//	DWORD dwIP;
	UINT			ret;        
	LPTSTR			tszCmdLine;
	int				retValue = 0;	// Module return value.
									// You should return 0 unless you had to abort.
	MODULE_PARAMS	mp;				// Cmd line args arranged here.
									// Used to init the stress utils.
	STRESS_RESULTS	results;		// Cumulative test results for this module.
	DWORD			dwIterations;
	ZeroMemory((void*) &mp, sizeof(mp));
	ZeroMemory((void*) &results, sizeof(results));
	RETAILMSG(1,(_T("filetransfer8023.exe WinMain Entry.\r\n")));
	// NOTE:  Modules should be runnable on desktop whenever possible,
	//        so we need to convert to a common cmd line char width:

#ifdef UNDER_NT
	tszCmdLine =  GetCommandLine();
#else
	tszCmdLine  = wszCmdLine;
#endif

	///////////////////////////////////////////////////////
	// Initialization
	//

	// Read the cmd line args into a module params struct.
	// You must call this before InitializeStressUtils().

	// Be sure to use the correct version, depending on your 
	// module entry point.

	if ( !ParseCmdLine_WinMain (tszCmdLine, &mp) )
	{
		LogFail(TEXT("Failure parsing the command line!  exiting ..."));
		return CESTRESS_ABORT; // Use the abort return code
	}
	// You must call this before using any stress utils 
	InitializeStressUtils (
							_T("filetransfer"), // Module name to be used in logging
							LOGZONE(SLOG_SPACE_NET, SLOG_WINSOCK),     // Logging zones used by default
							&mp          // Module params passed on the cmd line
							);

	// TODO: Module-specific parsing of tszUser ...
	InitUserCmdLineUtils(mp.tszUser, NULL);
	//if(!User_GetOptionAsDWORD(_T("ip"), &dwIP))
	//	dwIP = DEFAULT_WAITTIME;
	if(!Setup())
		return CESTRESS_ABORT;
	LogVerbose (TEXT("Starting at tick = %u"), dwStartTime);
RETAILMSG(1,(_T("Starting at tick = %u\r\n"), dwStartTime));
	///////////////////////////////////////////////////////
	// Main test loop
	//
	dwIterations = 0;
	while ( CheckTime(mp.dwDuration, dwStartTime) )
	{
		dwIterations++;
      RETAILMSG(1,(_T("dwIterations = %d\r\n"),dwIterations));
		// Pause if we're supposed to
		//if(dwWaitTime)
		//	Sleep(dwWaitTime);
		ret = DoStressIteration();
		if (ret == CESTRESS_ABORT)
		{
			LogFail (TEXT("Aborting on test iteration %i!"), results.nIterations);
			retValue = CESTRESS_ABORT;
			break;
		}
		if(ret==CESTRESS_PASS){
        RETAILMSG(1,(_T("Ethernet iter[%d] return PASS\r\n"),dwIterations));
		}
		else if(ret==CESTRESS_FAIL){
			RETAILMSG(1,(_T("Ethernet iter[%d] return FAIL\r\n"),dwIterations));
		}
		RecordIterationResults (&results, ret);
		if(dwIterations % STATUS_FREQUENCY == 0)
			LogVerbose(TEXT("completed %u iterations"), dwIterations);
	}

	///////////////////////////////////////////////////////
	// Clean-up, report results, and exit
	//
	Cleanup();
	DWORD dwEnd = GetTickCount();
	LogVerbose (TEXT("Leaving at tick = %d, duration (min) = %d"),
					dwEnd,
					(dwEnd - dwStartTime) / 60000
					);
	RETAILMSG(1,(_T("Ethernet results.nFail=%d.\r\n"),results.nFail));

	// DON'T FORGET:  You must report your results to the harness before exiting
	ReportResults (&results);
	//write resuts to registry.
	//WriteResultsToRegistry(results);
	return retValue;
}

void FillDefaults(MIB_IPFORWARDROW* pRouteInfo)
{
   pRouteInfo->dwForwardPolicy = 0;
   pRouteInfo->dwForwardType = 0;
   pRouteInfo->dwForwardProto = 3;
   pRouteInfo->dwForwardAge = 0;
   pRouteInfo->dwForwardNextHopAS = 0;
   pRouteInfo->dwForwardMetric1 = 0;
   pRouteInfo->dwForwardMetric2 = DEFAULT_METRIC_VALUE;
   pRouteInfo->dwForwardMetric3 = DEFAULT_METRIC_VALUE;
   pRouteInfo->dwForwardMetric4 = DEFAULT_METRIC_VALUE;
   pRouteInfo->dwForwardMetric5 = DEFAULT_METRIC_VALUE;
}

DWORD GetDefaultInterface(TCHAR *szAdapter, TCHAR *gate)
{
   PIP_ADAPTER_INFO pAdapterInfo = NULL, temp = NULL;
   ULONG ulSizeAdapterInfo=0;
   DWORD dwReturnvalueGetAdapterInfo=0;

   dwReturnvalueGetAdapterInfo = GetAdaptersInfo( pAdapterInfo, &ulSizeAdapterInfo );
   if (dwReturnvalueGetAdapterInfo == ERROR_BUFFER_OVERFLOW) {
      if (!(pAdapterInfo=(PIP_ADAPTER_INFO)malloc(ulSizeAdapterInfo)))
        {
       RETAILMSG(1,(_T("Insufficient Memory\r\n")));

          return -1;
      }
      dwReturnvalueGetAdapterInfo = GetAdaptersInfo(pAdapterInfo, &ulSizeAdapterInfo);
   }
   if (dwReturnvalueGetAdapterInfo!=ERROR_SUCCESS)
   {
      dwReturnvalueGetAdapterInfo = dwReturnvalueGetAdapterInfo;
      return -1;
   }
   if ( pAdapterInfo != NULL)
   {
      DWORD ReturnValue=0;

        TCHAR szName[MAX_PATH] = { 0 };
        temp = pAdapterInfo;
        while(temp)
        {
              mbstowcs(szName, temp->AdapterName, strlen(temp->AdapterName));
              if(_tcsicmp(szName, szAdapter) == 0)
              {
                    ReturnValue = temp->Index;
                    mbstowcs(gate, temp->GatewayList.IpAddress.String, strlen(temp->GatewayList.IpAddress.String));
                    break;
              }
              temp = temp->Next;
              memset(szName, 0, sizeof(szName));
        }

      free(pAdapterInfo); 
      return(ReturnValue); 
   }
   else
      return 0;
}

BOOL Setup (void)
{
//HRESULT hRet = E_FAIL;
BOOL bRet = FALSE;
CONNMGR_DESTINATION_INFO networkDestInfo = {0};
HANDLE m_hConnection = NULL;
LPCTSTR pszConnName = NULL;
CONNMGR_CONNECTION_DETAILED_STATUS * pStatusBuffer=NULL;
DWORD  Dest, Mask, Gateway, InterfaceVal = 0;
//MIB_IPFORWARDROW routeinfo;
DWORD returnvalue = 0;
DWORD BufferSize =NULL;
HRESULT hr = S_OK; 
memset(&routeinfo, 0, sizeof(MIB_IPFORWARDNUMBER));
ReadRouteValue(FTPServerEthernetUsername, FTPServerEthernetPassword,FTPServerWiFiUsername,FTPServerWiFiPassword,FTPServerWiFiIP, FTPServerEthernetIP, EthernetCardName);
RETAILMSG(1,(_T("FTPServerEthernetUsername=%s FTPServerEthernetPassword=%s FTPServerWiFiUsername=%s FTPServerWiFiPassword=%s FTPServerWiFiIP=%s FTPServerEthernetIP=%s EthernetCardName=%s\r\n"),FTPServerEthernetUsername,FTPServerEthernetPassword,FTPServerWiFiUsername,FTPServerWiFiPassword,FTPServerWiFiIP,FTPServerEthernetIP,EthernetCardName));
//TCHAR  Address[] = _T("192.168.0.3");
//TCHAR  Address[] = _T("10.200.162.151");
TCHAR * mask = _T("255.255.255.255");
	ConnMgrQueryDetailedStatus(pStatusBuffer,&BufferSize);	//allocate buffer
	pStatusBuffer = (CONNMGR_CONNECTION_DETAILED_STATUS *)new BYTE[BufferSize];
	//Call again
	hr=ConnMgrQueryDetailedStatus(pStatusBuffer,&BufferSize);
	if (hr!=S_OK)
	{
		RETAILMSG(1,(_T("We've got problems ConnMgrQueryDetailedStatus hr=0x%x ....exiting"),hr));
		return FALSE;
	}
	else
	{
	    RETAILMSG(1,(_T("ConnMgrQueryDetailedStatus success!BufferSize=%x\r\n"),BufferSize));
		CONNMGR_CONNECTION_DETAILED_STATUS *temp;
		temp = pStatusBuffer;
		int i=0;
		//Search a Nic connection
		do
		{
			if(temp->szAdapterName!=NULL){			
				    if(0 == wcscmp(temp->szAdapterName,EthernetCardName))
					{
					RETAILMSG(1,(_T("FOUND! temp->szAdapterName=%s\r\n"),(temp->szAdapterName)));
	                  break;
					}
			}
	            RETAILMSG(1,(_T("NOT FOUND! temp->szAdapterName=%s\r\n"),(temp->szAdapterName)));
		}while((temp = temp->pNext) != NULL);
		if(temp!= NULL){
			//Create a routing table.
			RETAILMSG(1,(_T("Create a routing table\r\n")));
			 DottedNotationToDword(FTPServerEthernetIP, &Dest);		
			 routeinfo.dwForwardDest = Dest;
			 DottedNotationToDword(mask, &Mask);
			 routeinfo.dwForwardMask = Mask;
			 TCHAR gate[16] = { 0 };
			 InterfaceVal = GetDefaultInterface(temp->szAdapterName, gate);
			 RETAILMSG(1,(_T("temp->szAdapterName=%s,InterfaceVal = %x,gate=%s\r\n"),temp->szAdapterName,InterfaceVal,gate));
			 DottedNotationToDword(gate, &Gateway);
			 routeinfo.dwForwardNextHop = Gateway;
			 routeinfo.dwForwardIfIndex = InterfaceVal;
			 FillDefaults(&routeinfo);
			 returnvalue = CreateIpForwardEntry(&routeinfo);
			 if (returnvalue == NO_ERROR){
               RETAILMSG(1,(_T("Entry added successfully\r\n")));
			   return TRUE;
			 }
			 else if (returnvalue == ERROR_INVALID_PARAMETER){
				 RETAILMSG(1,(_T("CreateIpForwardEntry failed: Invalid Parameter\r\n")));
				 
			 }
			else if (returnvalue == ERROR_NOT_SUPPORTED)
				  RETAILMSG(1,(_T("CreateIpForwardEntry not supported\r\n")));
			else if (returnvalue != NO_ERROR)
			  {
					 RETAILMSG(1,(_T("CreateRouteTableEntry: Unknown Error occured=%d\r\n"), returnvalue));
			  }
	       }
		return bRet;
  }
}

UINT DoStressIteration (void)
{
	UINT uRet = CESTRESS_PASS;
	//TCHAR  Address[] = _T("192.168.0.3");
	//TCHAR  Address[] = _T("10.200.162.151");
	RETAILMSG(1,(_T("Ethernet DoStressIteration[%d]\r\n"),i++));
	RETAILMSG(1,(_T("*******************Ethernet PutInternetFile to %s*******************\r\n"),FTPServerEthernetIP));
	uRet = PutInternetFile(FTPServerEthernetIP, _T("")); // put our server
	RETAILMSG(1,(_T("*******************Ethernet GetInternetFile from %s*******************\r\n"),FTPServerEthernetIP));
	uRet = GetInternetFile(FTPServerEthernetIP,_T(""));//Get from our server
	return uRet;
}


BOOL Cleanup (void)
{
	BOOL returnvalue = DeleteIpForwardEntry(&routeinfo);
				if (returnvalue==NO_ERROR)
				  RETAILMSG(1,(_T("Entry deleted successfully\r\n")));
			else if (returnvalue==ERROR_INVALID_PARAMETER)
			   RETAILMSG(1,(_T("DeleteIpForwardEntry failed: Invalid Parameter\r\n")));
			else if (returnvalue==ERROR_NOT_SUPPORTED)
				   RETAILMSG(1,(_T("DeleteIpForwardEntry not supported\r\n")));
			else if (returnvalue!=NO_ERROR)
				{
					  RETAILMSG(1,(_T("DeleteRouteTableEntry: Unknown Error occured=%d\r\n"), returnvalue));
				}


	return returnvalue;
}

#define NUM_TYPES				2			// Number of socket types we will be using

UINT PutInternetFile(LPCTSTR lpszServer, LPTSTR lpszProxyServer)
{

	UINT		bReturn = CESTRESS_FAIL;
	//bool		Fast ;
	DWORD		Start , dwTotalBytes;
	DWORD		dwSize = 0, 
				dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE; 	
	LPTSTR		AcceptTypes[2] = {TEXT("*/*"), NULL}; 
	//FileName.Format(TEXT("T%x"),(GetTickCount()));  // don't have the same name for 2 applications on different terminals.									// I know , it could still happen but this is easy.. kg
	//FileName += TEXT("MyTestFile.txt");	
	DWORD	PutFileSize = 0;	
	// the sendstrS is 64 bytes long
	//char sendstrS[101] = "1234567890abcdefghijklmnopqrstuvwxyz12345678901234567890abcdefgh";
	char sendstrS[101] = "LillianLillianLillianLillianLillianLillianLillianLillianLill";
	char sendstr[2050] = "";
	//WCHAR Add[16] = L"136.179.160.31";
	dwTotalBytes = 0 ;
	for (int i=0;i<32;i++){
		strcat(sendstr,sendstrS);
	}
	//sendstr should now be 2048  + the null
	PutFileSize = strlen(sendstr);
	RETAILMSG(1,(_T("Filetransfer8023 PutInternetFile PutFileSize = %d\r\n"),PutFileSize));
	TCHAR PutFileName[]= _T("Lillian8023.TXT");
	//FileName += TEXT(".txt");  // should now be tick count +.txt
	//PutFileName += FileName ;


	//hOpen = InternetOpen (TEXT("FileTransferTestCpp"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, 0, 0);
	hOpen = InternetOpen (TEXT("FileTransferTestCpp"), INTERNET_OPEN_TYPE_DIRECT, NULL, 0, 0);
	if (!hOpen)
	{
	RETAILMSG(1,(_T("Ethernet InternetOpen FAIL Error=%x\r\n"),GetLastError()));
	goto exit;
	}   	
	RETAILMSG(1,(_T("Ethernet InternetOpen success!\r\n")));


	//if (!(hConnect = InternetConnect (hOpen, _T("136.179.160.31"), 60001, _T("ftptest"), _T("Siemens1"), INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE , 0)))
	//if (!(hConnect = InternetConnect (hOpen, lpszServer, 60001, _T("ftptest"), _T("Siemens1"), INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE , 0)))
//	if (!(hConnect = InternetConnect (hOpen, lpszServer, INTERNET_INVALID_PORT_NUMBER, _T("ftptest"), _T("Siemens1"), INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE , 0)))

	if (!(hConnect = InternetConnect (hOpen, lpszServer, INTERNET_DEFAULT_FTP_PORT, FTPServerEthernetUsername, FTPServerEthernetPassword, INTERNET_SERVICE_FTP, 0 , 0)))
	//if (!(hConnect = InternetConnect (hOpen, lpszServer, 21, _T("ftptest2"), _T("Siemens1"), INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE , 0)))
	{

		DWORD foo = GetLastError() ;
		RETAILMSG(1,(_T("Ethernet InternetConnect FAIL Error=%x\r\n"),foo));		
		goto exit;
	}
	RETAILMSG(1,(_T("Ethernet InternetConnect success!\r\n")));

	//if (!(hRequest = FtpOpenFile(hConnect, TEXT("/FTP/testsend.pdf") , GENERIC_WRITE, INTERNET_FLAG_TRANSFER_ASCII , 0)))
	//if (!(hRequest = FtpOpenFile(hConnect, TEXT("/FTP/testsend.pdf") , GENERIC_WRITE, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_NO_CACHE_WRITE, 0)))
	if (!(hRequest = FtpOpenFile(hConnect, PutFileName , GENERIC_WRITE, FTP_TRANSFER_TYPE_BINARY , 0)))
	{
	RETAILMSG(1,(_T("Ethernet FtpOpenFile FAIL Error=%x\r\n"),GetLastError()));
	goto exit;
	}
	RETAILMSG(1,(_T("Ethernet FtpOpenFile success!\r\n")));

	Start = GetTickCount();

	do{
		if(!InternetWriteFile(hRequest,(LPCVOID)&sendstr[0],(sizeof(sendstr)-1),&dwSize))
		{
		RETAILMSG(1,(_T("Ethernet InternetWriteFile FAIL Error=%x\r\n"),GetLastError()));
			goto exit;
		}
			RETAILMSG(1,(_T("Ethernet InternetWriteFile success!dwSize=%x\r\n"),dwSize));
			/*
			for(int i=0;i<dwSize;i++){
	             if(i%16==0){          
				 RETAILMSG(1,(_T("\r\n")));
			     }
			RETAILMSG(1,(_T("sendstr[%d]=%x"),i,sendstr[i]));
			}
			*/
		if (dwSize >= 0){
			dwTotalBytes += dwSize ;
		}
			if (dwSize >= 0)    
			{
				RETAILMSG(1,(_T("Ethernet FTP page sent %d bytes\r\n"), dwTotalBytes));
			} else {
				RETAILMSG(1,(_T("Ethernet FTP page sent 0 bytes -- DONE\r\n")));
			}		
	}while(dwTotalBytes < PutFileSize);

	DWORD End = GetTickCount();
	DWORD Time = End - Start ;
	if (Time < 1000){
		Time = 1000 ;
	}
	DWORD Speed = ( (dwTotalBytes *8) / (Time / 1000) ) ; // bytes to bits  then miliseconds to seconds
	Speed /= 1000 ; // change to Kbps

	if ( dwTotalBytes >= PutFileSize ){
		RETAILMSG(1,(_T("Ethernet  File transfer success throughput speed was %d Kbits per second \r\n"),Speed));
	} else {
		RETAILMSG(1,(_T("Ethernet  File transfer error\r\n")));
	}
	bReturn = CESTRESS_PASS;
exit:
	if (hRequest){
		if (!InternetCloseHandle (hRequest))
			hRequest = NULL ; // must close before I can delete it.
	}
	FtpDeleteFile(hConnect,PutFileName );
	RETAILMSG(1,(_T("Ethernet exit FTP page send %d bytes\r\n"), dwTotalBytes));
	CloseTheHandles();
	return bReturn;
}



UINT GetInternetFile(LPCTSTR lpszServer, LPTSTR lpszProxyServer)
{
	UINT		bReturn = CESTRESS_FAIL;
	//bool		Fast ;

	DWORD		dwSize = 0, 
				dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE; 	
	char		*lpBufferA;
	LPTSTR		AcceptTypes[2] = {TEXT("*/*"), NULL}; 
	//CString FileName ;
	TCHAR PutFileName[]= _T("1k.pdf");

	DWORD End ;
	//DWORD dwBytesWritten;
	#define BufSize 4100
	DWORD dwTotalBytes = 0 ;
	RETAILMSG(1,(_T("Ethernet GetInternetFile from lpszServer=%s\r\n"),lpszServer));
	lpBufferA = new CHAR [BufSize];
	//FileName = TEXT("1k.pdf");

	//Set up the request...
	hOpen = InternetOpen (TEXT("FileTransferTestCpp"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, 0, 0);
	if (!hOpen)
	{
		LogFail(_T("\r\nEthernet  InternetOpen Error: %x\r\n"), GetLastError());
		return FALSE;
	}  
	RETAILMSG(1,(_T("InternetOpen success!\r\n")));
	if (!(hConnect = InternetConnect (hOpen, lpszServer, INTERNET_DEFAULT_FTP_PORT,  FTPServerEthernetUsername, FTPServerEthernetPassword, INTERNET_SERVICE_FTP, 0, 0)))
		{
			LogFail(_T("Ethernet InternetConnect Error: %x\r\n"), GetLastError());	
			RETAILMSG(1,(_T("Ethernet InternetConnect Error: %x\r\n"),GetLastError()));
			goto exit;
		}
	RETAILMSG(1,(_T("Ethernet InternetConnect success!\r\n")));
	DWORD dwContext =1;/* = 1 */
	HINTERNET hFile;
	hFile = FtpOpenFile(hConnect,PutFileName,GENERIC_READ,FTP_TRANSFER_TYPE_BINARY,dwContext);
	DWORD Start = GetTickCount();
	do
	{
		if (!InternetReadFile (hFile, (LPVOID)lpBufferA,(BufSize - 2), &dwSize))
		{
		RETAILMSG(1,(_T("Ethernet InternetReadFile fail\r\n")));
			LogFail(_T("Ethernet InternetReadFile Error: %x\r\n"), GetLastError());
			LogFail(_T("Ethernet HTTP page Received %d bytes before error\r\n"), dwTotalBytes);
			LogFail(_T("Ethernet Error reading HTTP connection\r\n"));
			goto exit;
		}
		RETAILMSG(1,(_T("Ethernet InternetReadFile success,get data Size=%x\r\n"),dwSize));

		//for(int i=0;i<512;i++){
		//	if(i%16==0){
       //    RETAILMSG(1,(_T("\r\n")));
		//	}
		//RETAILMSG(1,(_T("0x%x"),*(lpBufferA+i)));
		//}
		if (dwSize != 0)    
		{
			dwTotalBytes += dwSize ;
		} else {
			End = GetTickCount();
		}
			if (dwSize != 0)    
			{
				//if(!Fast){
					LogVerbose(_T("Ethernet FTP page Received %d bytes\r\n"), dwTotalBytes);
				//}
			} 
			//else {
			//	LogVerbose(_T("Ethernet HTTP page Received 0 bytes -- DONE "));
			//}

	} while (dwSize);
	DWORD Time = End - Start ;
	DWORD Speed ;
	if (Time < 1000){
		Time = 1000 ;
	}
	Speed = ( (dwTotalBytes *8) / (Time / 1000) ) ; // bytes to bits  then miliseconds to seconds
	Speed /= 1000 ; // change to Kbps
	if (dwSize == 0 ){
		LogVerbose(_T("Ethernet File download success FTP speed was %d Kbits per second \r\n"), Speed);
		LogVerbose(_T("Ethernet File download Time was %d mSeconds\r\n"), Time);
	} else {
		LogFail(_T("Ethernet File download error\r\n"));
	}

	bReturn = CESTRESS_PASS;
exit:

	delete[] lpBufferA;  
	LogVerbose(_T("Ethernet exit FTP page Received %d bytes\r\n"), dwTotalBytes);	
	CloseTheHandles();
	return (bReturn);
}

void CloseTheHandles()
{
	LogVerbose(_T("Ethernet Closeing Handles !!"));
	// Close the Internet handles.
	//if (hRequest!=NULL)
	//{
	//	if (!InternetCloseHandle (hRequest))
	//		LogFail(_T("CloseHandle (hRequest) Error: %x"), GetLastError());
	//	hRequest = NULL ;
	//}
	if (hConnect!=NULL)
	{
		if (!InternetCloseHandle (hConnect))
			LogWarn1(_T("Ethernet CloseHandle (hConnect) Error: %x"), GetLastError());	
		hConnect = NULL ;
	}
	if (hOpen!=NULL)
	{
		if (!InternetCloseHandle (hOpen))
			LogFail(_T("Ethernet CloseHandle (hOpen) Error: %x"), GetLastError());	
			hOpen = NULL ;
	}
	LogVerbose(_T("Ethernet Handles Closed !!"));

}

void ReadRouteValue(TCHAR *FTPServerEthernetUsername, TCHAR *FTPServerEthernetPassword,TCHAR *FTPServerWiFiUsername, TCHAR *FTPServerWiFiPassword,TCHAR *FTPServerWiFiIP, TCHAR *FTPServerEthernetIP, TCHAR *EthernetCardName)
{
      int nRetValue = 0;
      HKEY hKey;
      TCHAR szRegKey[MAX_PATH] = { 0 };
      swprintf(szRegKey, L"Software\\Intermec\\CESTRESS\\FileTransfer");
      nRetValue = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegKey, 0, KEY_ALL_ACCESS, &hKey);
      if(nRetValue == ERROR_SUCCESS)
      {
            //TCHAR szValue[] = 0;
			TCHAR szString[256];
            DWORD dwSize = sizeof(szString);
            RegQueryValueEx(hKey, L"FTPServerEthernetUsername", NULL, NULL, (LPBYTE)&szString, &dwSize);
			memcpy(FTPServerEthernetUsername,szString,dwSize);
            //FTPServerEthernetUsername = szString;
			RETAILMSG(1,(_T("\r\nszString=%s,FTPServerEthernetUsername=%s\r\n"),szString,FTPServerEthernetUsername));
           // szString = {_T("0")};
            dwSize = sizeof(szString);
			RegQueryValueEx(hKey, L"FTPServerEthernetPassword", NULL, NULL, (LPBYTE)&szString, &dwSize);
            //FTPServerEthernetPassword = szString;
			memcpy(FTPServerEthernetPassword,szString,dwSize);
			RETAILMSG(1,(_T("szString=%s,FTPServerEthernetPassword=%s\r\n"),szString,FTPServerEthernetPassword));
            //szString = {0};
            dwSize = sizeof(szString);
			RegQueryValueEx(hKey, L"FTPServerWiFiUsername", NULL, NULL, (LPBYTE)&szString, &dwSize);
           // FTPServerWiFiUsername = szString;
			memcpy(FTPServerWiFiUsername,szString,dwSize);
			RETAILMSG(1,(_T("szString=%s,FTPServerWiFiUsername=%s\r\n"),szString,FTPServerWiFiUsername));
            //szString = {0};
            dwSize = sizeof(szString);
		     RegQueryValueEx(hKey, L"FTPServerWiFiPassword", NULL, NULL, (LPBYTE)&szString, &dwSize);
            //FTPServerEthernetPassword = szString;
			memcpy(FTPServerWiFiPassword,szString,dwSize);
			RETAILMSG(1,(_T("szString=%s,FTPServerWiFiPassword=%s\r\n"),szString,FTPServerWiFiPassword));
            //szString = {0};
            dwSize = sizeof(szString);
            RegQueryValueEx(hKey, L"FTPServerWiFiIP", NULL, NULL, (LPBYTE)&szString, &dwSize);
            //FTPServerWiFiIP = szString;
			memcpy(FTPServerWiFiIP,szString,dwSize);
			RETAILMSG(1,(_T("szString=%s,FTPServerWiFiIP=%s\r\n"),szString,FTPServerWiFiIP));
           // szString = {0};
            dwSize = sizeof(szString);
            RegQueryValueEx(hKey, L"FTPServerEthernetIP", NULL, NULL, (LPBYTE)&szString, &dwSize);
            //FTPServerEthernetIP = szString;
			memcpy(FTPServerEthernetIP,szString,dwSize);
			RETAILMSG(1,(_T("szString=%s,FTPServerEthernetIP=%s\r\n"),szString,FTPServerEthernetIP));
           // szString = {0};
            dwSize = sizeof(szString);
            RegQueryValueEx(hKey, L"EthernetCardName", NULL, NULL, (LPBYTE)&szString, &dwSize);
            //EthernetCardName = szString;
			memcpy(EthernetCardName,szString,dwSize);
			RETAILMSG(1,(_T("szString=%s,EthernetCardName=%s\r\n"),szString,EthernetCardName));
            RegCloseKey(hKey);
      }
}
/*
void WriteResultsToRegistry(STRESS_RESULTS results)
{
	int nRetValue = 0;
	HKEY hKey;
	TCHAR szRegKey[MAX_PATH] = { 0 };
	LPDWORD dwDisposition ;
	swprintf(szRegKey, L"Software\\Intermec\\CESTRESS\\FileTransfer\Ethernet");
	nRetValue = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegKey, 0,NULL, REG_OPTION_NON_VOLATILE, 0, NULL, &hKey,dwDisposition);
	if (nRetValue == ERROR_SUCCESS) {
		RegSetValueEx(hKey, TEXT("Results"), 0, REG_DWORD,(LPBYTE)&results.nFail, sizeof(results.nFail));
		RegCloseKey(hKey);
	}
}
*/
