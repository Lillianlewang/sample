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
#include <stressutils.h>
#include <stdlib.h>
#include <stdio.h>
#include <ProduceRecord.h>
#define STATUS_FREQUENCY		1000		// print out our status every time this number
											// of iterations has been performed

#define DEFAULT_WAITTIME		50			// Default time to wait (in ms) between binds.
											// This is used to lessen the chance that
											// multiple, concurrent bind tests will
											// interfere with one another and to prevent
											// the test from cycling thru the ports too
											// quickly (WSAEADDRINUSE).


#define BUFFER_SIZE 100000
#define CODEC_SAMPLE_RATE 44100.0
#define WAVE_BUFFER_SIZE (BUFFER_SIZE / sizeof(short))
DWORD g_dwProduceStartTick;
DWORD g_dwRecordStartTick;
DWORD dwRate;
WORD wSize;
UINT loop = 0;
TCHAR FileName[8];
TCHAR FileNameBuffer[11];
BOOL g_bFlagProduceTimeup =TRUE;
BOOL g_bFlagRecordTimeup =TRUE;

//UINT g_loopNum = 0;
UINT LOOPINDEX = 0;
UINT g_ncount  = 0;

// constants 

BOOL g_bProduce = TRUE;
BOOL g_bRecord = TRUE;
//BOOL g_bProduceThreadExit = FALSE;
//BOOL g_bRecordThreadExit = FALSE;

// globals
//HINSTANCE g_hInstance = NULL;

HANDLE  g_hInst                         = NULL;
LPTSTR  g_pszCmdLine                    = NULL;
DWORD		g_cycles ;
//HANDLE g_RandomThread = NULL;

//HANDLE g_ProduceAudioThread = NULL;
//HANDLE g_RecordAudioThread = NULL;
BOOL g_FlagExit = FALSE;

HANDLE *g_EventArray = NULL;
BOOL g_bproduce = FALSE;
BOOL g_brecord  = FALSE;

HANDLE hAuto                            = NULL;
HANDLE hManual                          = NULL;


// Function Prototypes
UINT ProduceThread(LPVOID);
UINT RecordThread(LPVOID);

//UINT DoStressIteration (void);
UINT Produce();
UINT Record();
void CALLBACK waveOutProc2(HWAVEOUT ,UINT ,DWORD ,DWORD ,DWORD );
void CALLBACK waveInProc(HWAVEIN ,UINT ,DWORD ,DWORD ,DWORD );
UINT SaveWaveFile();
UINT CleanupToneTest() ;
UINT OpenWaveFileForRecord();
UINT WriteWaveFile();
UINT CloseWaveDeviceForRecord();
UINT ToneGenerator3(short *,struct sWaveHdr *, float );

UINT i=0;
BOOL g_Exit=FALSE;
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
	//DWORD			 dwStartTime = GetTickCount();

	//UINT			ret;        
	LPTSTR			tszCmdLine;
	int				retValue = 0;	// Module return value.
									// You should return 0 unless you had to abort.
	MODULE_PARAMS	mp;				// Cmd line args arranged here.
									// Used to init the stress utils.
//	STRESS_RESULTS	results;		// Cumulative test results for this module.
	//DWORD			dwIterations;

	RETAILMSG(1,(_T("ProduceRecord.exe WinMain Entry.\r\n")));
	ZeroMemory((void*) &mp, sizeof(mp));
//	ZeroMemory((void*) &results, sizeof(results));

	
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
							_T("ProduceRecord"), // Module name to be used in logging
							LOGZONE(SLOG_SPACE_NET, SLOG_WINSOCK),     // Logging zones used by default
							&mp          // Module params passed on the cmd line
							);


	// TODO: Module-specific parsing of tszUser ...
	InitUserCmdLineUtils(mp.tszUser, NULL);


	
//	LogVerbose (TEXT("Starting at tick = %u"), dwStartTime);
//RETAILMSG(1,(_T("Starting at tick = %u\r\n"), dwStartTime));
	///////////////////////////////////////////////////////
	m_psinewavebuffer1 = new short[WAVE_BUFFER_SIZE];
    m_psinewavebuffer2 = new short[WAVE_BUFFER_SIZE];
	// Main test loop
	//
	//dwIterations = 0;
	HANDLE hProduceThread	= CreateThread( 0, 0, (LPTHREAD_START_ROUTINE)ProduceThread,  &mp, 0, NULL);
	HANDLE hRecordThread	= CreateThread( 0, 0, (LPTHREAD_START_ROUTINE)RecordThread,  &mp, 0, NULL);
	while(!g_Exit)
		{
	Sleep(0);
		}
	
	delete [] m_psinewavebuffer1;
	m_psinewavebuffer1 = NULL;
	delete [] m_psinewavebuffer2;
	m_psinewavebuffer2 = NULL;
//	while ( CheckTime(mp.dwDuration, dwStartTime) )
//	{
//		dwIterations++;
  //    RETAILMSG(1,(_T("dwIterations = %d,dwWaitTime=%x\r\n"),dwIterations,dwWaitTime));
		// Pause if we're supposed to
		//if(dwWaitTime)
		//	Sleep(dwWaitTime);

		//ret = DoStressIteration();

		
	//	RecordIterationResults (&results, ret);
//		if(dwIterations % STATUS_FREQUENCY == 0)
	//		LogVerbose(TEXT("completed %u iterations"), dwIterations);
//	}

	///////////////////////////////////////////////////////
	// Clean-up, report results, and exit
//	g_Exit=TRUE;
//	DWORD dwEnd = GetTickCount();
//	LogVerbose (TEXT("Leaving at tick = %d, duration (min) = %d"),
	//				dwEnd,
	//				(dwEnd - dwStartTime) / 60000
	//				);

//RETAILMSG(1,(_T("hdqread results.nFail=%d\r\n"),results.nFail));
	// DON'T FORGET:  You must report your results to the harness before exiting
//	ReportResults (&results);
	return retValue;
}
/*
UINT DoStressIteration (void)
{
	UINT uRet = CESTRESS_FAIL;
	UINT uRet1 = CESTRESS_FAIL;
	RETAILMSG(1,(_T("hdqread DoStressIteration[%d]\r\n"),i++));
	BOOL io = TRUE;
DWORD dwActualBytes = 0;
BYTE HDQInfo = 0;
uRet1 = HDQRead1();
if(uRet1==CESTRESS_PASS){
	uRet1 = HDQRead2();
	if(CESTRESS_PASS == uRet1){
	uRet = CESTRESS_PASS;
	}
}
	return uRet;
}
*/
UINT ProduceThread(LPVOID lpParameter){
RETAILMSG(1,(_T("\r\nProduceThread ENTRY!\r\n")));
UINT retValue;
STRESS_RESULTS	results;		// Cumulative test results for this module.
ZeroMemory((void*) &results, sizeof(results));
MODULE_PARAMS *pmp = (MODULE_PARAMS*)lpParameter;
 retValue=CESTRESS_FAIL;
DWORD			 dwStartTime = GetTickCount();
int				ret = 0;
DWORD			dwIterations=0;
LogVerbose (TEXT("Starting at tick = %u"), dwStartTime);
RETAILMSG(1,(_T("Starting at tick = %u\r\n"), dwStartTime));
//while(!g_Exit){
	while ( CheckTime(pmp->dwDuration, dwStartTime) )
	{
		dwIterations++;
	ret = Produce();
	if (ret == CESTRESS_ABORT)
		{
			LogFail (TEXT("Aborting on test iteration %i!"), results.nIterations);
			retValue = CESTRESS_ABORT;
			break;
		}
		if(ret==CESTRESS_PASS){
        RETAILMSG(1,(_T("Produce iter[%d] return PASS\r\n"),dwIterations));
		}
		else if(ret==CESTRESS_FAIL){
			RETAILMSG(1,(_T("Produce iter[%d] return FAIL\r\n"),dwIterations));
		}
		RecordIterationResults (&results, ret);
	}

	if(dwIterations % STATUS_FREQUENCY == 0)
		LogVerbose(TEXT("Produce completed %u iterations"), dwIterations);
	//g_Exit=TRUE;
//}
	///////////////////////////////////////////////////////
	// Clean-up, report results, and exit
	DWORD dwEnd = GetTickCount();
	LogVerbose (TEXT("Leaving at tick = %d, duration (min) = %d"),
					dwEnd,
					(dwEnd - dwStartTime) / 60000
					);

    RETAILMSG(1,(_T("ProduceThread results.nFail=%d\r\n"),results.nFail));
	// DON'T FORGET:  You must report your results to the harness before exiting
	ReportResults (&results);
	g_Exit=TRUE;

return retValue;
}
UINT RecordThread(LPVOID lpParameter){
	RETAILMSG(1,(_T("RecordThread ENTRY!\r\n")));
	//MODULE_PARAMS	* pmp=NULL;				// Cmd line args arranged here.
	//RETAILMSG(1,(_T("after pmp declare!\r\n")));
	UINT retValue;
	STRESS_RESULTS	results;		// Cumulative test results for this module.
	//ZeroMemory((void*) pmp, sizeof(MODULE_PARAMS));
	//RETAILMSG(1,(_T("middle zeromem\r\n")));
	ZeroMemory((void*) &results, sizeof(results));
MODULE_PARAMS *pmp = (MODULE_PARAMS*)lpParameter;
 retValue=CESTRESS_FAIL;
 
DWORD			 dwStartTime = GetTickCount();

int				ret = 0;

DWORD			dwIterations=0;
LogVerbose (TEXT("Starting at tick = %u"), dwStartTime);
RETAILMSG(1,(_T("Starting at tick = %u\r\n"), dwStartTime));

//while(!g_Exit){
	while ( CheckTime(pmp->dwDuration, dwStartTime) )
	{
		dwIterations++;
	ret = Record();
	if (ret == CESTRESS_ABORT)
		{
			LogFail (TEXT("Aborting on Record test iteration %i!"), results.nIterations);
			retValue = CESTRESS_ABORT;
			break;
		}
		if(ret==CESTRESS_PASS){
        RETAILMSG(1,(_T("Record iter[%d] return PASS\r\n"),dwIterations));
		}
		else if(ret==CESTRESS_FAIL){
			RETAILMSG(1,(_T("Record iter[%d] return FAIL\r\n"),dwIterations));
		}
		RecordIterationResults (&results, ret);

	}

	if(dwIterations % STATUS_FREQUENCY == 0)
		LogVerbose(TEXT("Record completed %u iterations"), dwIterations);
//}
	///////////////////////////////////////////////////////
	// Clean-up, report results, and exit
	DWORD dwEnd = GetTickCount();
	LogVerbose (TEXT("Leaving Record at tick = %d, duration (min) = %d"),
					dwEnd,
					(dwEnd - dwStartTime) / 60000
					);

    RETAILMSG(1,(_T("RecordThread results.nFail=%d\r\n"),results.nFail));
	// DON'T FORGET:  You must report your results to the harness before exiting
	ReportResults (&results);
	//g_Exit=TRUE;
	/*
	if (hWavesave != INVALID_HANDLE_VALUE){
		   CloseHandle(hWavesave);
		   hWavesave = INVALID_HANDLE_VALUE;	
		  }
		  if(DeleteFile(FileName))
			{
			RETAILMSG(1,(_T("Delete file %s success.\r\n"),FileName));
			}
		  else{
			  RETAILMSG(1,(_T("Delete file %s fail.\r\n"),FileName));
			}
		  */
//}
return retValue;
}
UINT Produce()
{
///////
RETAILMSG(1,(_T("Produce entry\r\n")));
int          maxsamples = 0;
MMRESULT     mr = 0;
memset((void *)&m_WAVEHDR1, '\0', sizeof(struct sWaveHdr));
m_WAVEHDR1.nIndex = 1;
memset((void *)&m_WAVEHDR2, '\0', sizeof(struct sWaveHdr));
m_WAVEHDR2.nIndex = 2;
//HANDLE 		 hEvents[2];

//g_EventArray       = new HANDLE[EventCounts];
//INT i = 0;
   // g_EventArray = new HANDLE[EventCounts];
	//for (i = 0; i < EventCounts; i +=1)
	//{ 
  //     g_EventArray[i] = CreateEvent(NULL,FALSE,FALSE,NULL); 
	//}
//	m_dwWaveDataLen = 0;
//	static TCHAR const wav[] = TEXT(".wav");
//	static TCHAR const tempwav[] = TEXT("temp.wav");
//	 memset(FileNameBuffer,'\0',sizeof(FileNameBuffer));
//	 _itow(Random()%1000,FileNameBuffer,10);
//	 wcscat(FileNameBuffer,tempwav);
//	 RETAILMSG(1,(_T("FileNameBuffer %s\r\n"),FileNameBuffer));
//	 memset(FileName,'\0',sizeof(FileName));
//	 _itow(Random()%1000,FileName,10);
//	 RETAILMSG(1,(_T("FileName %s\r\n"),FileName));
	switch (Random() % AUDIO_RATE_TOTAL)
		{
		case AUDIO_RATE_8KHZ_NUM:
			dwRate = 8000;
			break;
		case AUDIO_RATE_11KHZ_NUM:
			dwRate = 11000;
			break;
		case AUDIO_RATE_22KHZ_NUM:
			dwRate = 22000;
			break;
		case AUDIO_RATE_44KHZ_NUM:
			dwRate = 44000;
			break;
		case AUDIO_RATE_48KHZ_NUM:
			dwRate = 48000;
			break;
		case AUDIO_RATE_96KHZ_NUM:
			dwRate = 96000;
			break;
		default:
			break;		
		}	
	    switch(Random() % AUDIO_SIZE_TOTAL)
		{
		case AUDIO_SIZE_8BITS_NUM:
			wSize =	8;
			break;
		case AUDIO_SIZE_16BITS_NUM:
			wSize =	16;
			break;
		default:
			break;
		}
		g_bFlagProduceTimeup =FALSE;

//////
//UINT          maxsamples = 0;
//MMRESULT     mr = 0;
 UINT Result = CESTRESS_PASS;
 DWORD dwWaitObject = 0;
m_wfeProduce.cbSize = 0;	
m_wfeProduce.wFormatTag = 1;
m_wfeProduce.nChannels = 1;//Just mono
m_wfeProduce.wBitsPerSample = wSize;
m_wfeProduce.nSamplesPerSec = dwRate;
m_wfeProduce.nBlockAlign = (m_wfeProduce.nChannels * m_wfeProduce.wBitsPerSample) / 8;
m_wfeProduce.nAvgBytesPerSec = m_wfeProduce.nSamplesPerSec * m_wfeProduce.nBlockAlign;
// m_bPlayBack = FALSE;
//	m_bPlayBack = TRUE;

 	mr = waveOutOpen(&m_hWaveOut,
				            WAVE_MAPPER,
				            &m_wfeProduce,
				            (DWORD)waveOutProc2,
				            NULL,
				            CALLBACK_FUNCTION);	
	RETAILMSG(1,(_T("waveOutOpen mr=%x\r\n"),mr));
					// Setup flags
				    m_bPlayBack = TRUE;
					if (!mr == MMSYSERR_NOERROR){
					LogFail(_T("waveOutOpen failed.error %u"), mr);
					goto done;
					}
					//RETAILMSG(1,(_T(" After waveOutOpen mr = %x,m_hWaveOut=%d\r\n"),mr,m_hWaveOut));
					memset((void *)m_psinewavebuffer1, '\0', BUFFER_SIZE);
				    memset((void *)m_psinewavebuffer2, '\0', BUFFER_SIZE);				
				    m_WAVEHDR1.m_WaveHdr.lpData = (char *)m_psinewavebuffer1;
				    m_WAVEHDR1.m_WaveHdr.dwBufferLength = BUFFER_SIZE;
				    m_WAVEHDR1.m_WaveHdr.dwFlags = 0;
				    maxsamples = ToneGenerator3(m_psinewavebuffer1, &m_WAVEHDR1, (float)(m_wfeProduce.nSamplesPerSec));
				    mr = waveOutPrepareHeader(m_hWaveOut, &m_WAVEHDR1.m_WaveHdr, sizeof(WAVEHDR));
					if (!mr == MMSYSERR_NOERROR){
				//	RETAILMSG(1,(_T(" waveOutPrepareHeader m_WAVEHDR1 fail\r\n")));
					LogFail(_T("waveOutPrepareHeader1 faile.Error %u"), mr);
				    Result = CESTRESS_FAIL;
					goto done;
					}
					//RETAILMSG(1,(_T(" waveOutPrepareHeader m_WAVEHDR1 pass\r\n")));
				    m_WAVEHDR1.m_WaveHdr.dwBufferLength = maxsamples * 2;
				    m_WAVEHDR2.m_WaveHdr.lpData = (char *)m_psinewavebuffer2;
				    m_WAVEHDR2.m_WaveHdr.dwBufferLength = BUFFER_SIZE;
				    m_WAVEHDR2.m_WaveHdr.dwFlags = 0;
				    maxsamples = ToneGenerator3(m_psinewavebuffer2, &m_WAVEHDR2, (float)( m_wfeProduce.nSamplesPerSec));
				    mr =waveOutPrepareHeader(m_hWaveOut, &m_WAVEHDR2.m_WaveHdr, sizeof(WAVEHDR));
					if (!mr == MMSYSERR_NOERROR){
					//RETAILMSG(1,(_T(" waveOutPrepareHeader m_WAVEHDR2 fail\r\n")));
					LogFail(_T("waveOutPrepareHeader2 fail.Error %u"), mr);
					goto done;
					}
					//RETAILMSG(1,(_T(" waveOutPrepareHeader m_WAVEHDR2 pass\r\n")));
				    m_WAVEHDR2.m_WaveHdr.dwBufferLength = maxsamples * 2;
					g_dwProduceStartTick = GetTickCount();
					// Play buffer one
					MMRESULT mrWrite1 = waveOutWrite(m_hWaveOut, &m_WAVEHDR1.m_WaveHdr, sizeof(WAVEHDR));
					//RETAILMSG(1,(_T("  After waveOutWrite mrWrite1 = %x!\r\n"),mrWrite1));
					// Play buffer two
					MMRESULT mrWrite2 = waveOutWrite(m_hWaveOut, &m_WAVEHDR2.m_WaveHdr, sizeof(WAVEHDR));
//HANDLE hEvent = g_EventArray[ProduceAudioDoneEvent];

	while(!g_bFlagProduceTimeup){
		Sleep(500);

	}
 // dwWaitObject = WaitForSingleObject(hEvent,INFINITE);
		 //  switch(dwWaitObject){
		   //	    case WAIT_OBJECT_0:{
                  if(CESTRESS_FAIL == CleanupToneTest()){
		            LogFail(_T("CleanupToneTest fail"));
					Result = CESTRESS_FAIL;
					//break;
					}
               //  break;
				//}
             //   default:
				//	break;
		   ////	}
	//}
done:
	return Result;
}
UINT Record(){
/////
RETAILMSG(1,(_T("Record entry!\r\n")));
int          maxsamples = 0;
MMRESULT     mr = 0;

//memset((void *)&m_WAVEHDR1, '\0', sizeof(struct sWaveHdr));
//m_WAVEHDR1.nIndex = 1;
//memset((void *)&m_WAVEHDR2, '\0', sizeof(struct sWaveHdr));
//m_WAVEHDR2.nIndex = 2;
//HANDLE 		 hEvents[2];
//m_psinewavebuffer1 = new short[WAVE_BUFFER_SIZE];
//m_psinewavebuffer2 = new short[WAVE_BUFFER_SIZE];
//g_EventArray       = new HANDLE[EventCounts];
INT i = 0;
  //  g_EventArray = new HANDLE[EventCounts];
//	for (i = 0; i < EventCounts; i +=1)
//	{ 
 //      g_EventArray[i] = CreateEvent(NULL,FALSE,FALSE,NULL); 
//	}
	m_dwWaveDataLen = 0;
	static TCHAR const wav[] = TEXT(".wav");
	static TCHAR const tempwav[] = TEXT("temp.wav");
	 memset(FileNameBuffer,'\0',sizeof(FileNameBuffer));
	 _itow(Random()%1000,FileNameBuffer,10);
	 wcscat(FileNameBuffer,tempwav);
	 RETAILMSG(1,(_T("FileNameBuffer %s\r\n"),FileNameBuffer));
	 memset(FileName,'\0',sizeof(FileName));
	 _itow(Random()%1000,FileName,10);
	 wcscat(FileName,wav);
	 RETAILMSG(1,(_T("FileName %s\r\n"),FileName));
	switch (Random() % AUDIO_RATE_TOTAL)
		{
		case AUDIO_RATE_8KHZ_NUM:
			dwRate = 8000;
			break;
		case AUDIO_RATE_11KHZ_NUM:
			dwRate = 11000;
			break;
		case AUDIO_RATE_22KHZ_NUM:
			dwRate = 22000;
			break;
		case AUDIO_RATE_44KHZ_NUM:
			dwRate = 44000;
			break;
		case AUDIO_RATE_48KHZ_NUM:
			dwRate = 48000;
			break;
		case AUDIO_RATE_96KHZ_NUM:
			dwRate = 96000;
			break;
		default:
			break;		
		}	
	    switch(Random() % AUDIO_SIZE_TOTAL)
		{
		case AUDIO_SIZE_8BITS_NUM:
			wSize =	8;
			break;
		case AUDIO_SIZE_16BITS_NUM:
			wSize =	16;
			break;
		default:
			break;
		}
		g_bFlagRecordTimeup =FALSE;
/////
UINT Result = CESTRESS_PASS;
//HANDLE hEvent = g_EventArray[RecordAudioDoneEvent];
Result = OpenWaveFileForRecord();
RETAILMSG(1,(_T("OpenWaveFileForRecord Result=%x\r\n"),Result));
	while(!g_bFlagRecordTimeup){
		Sleep(500);
	    RETAILMSG(1,(_T("+")));
	}
	//bRet = WaitForSingleObject(hEvent,INFINITE);
	  //  if(bRet == WAIT_OBJECT_0){
                LogVerbose(_T("RecordAudioDoneEvent time up,next to clean up."));
				if (!m_pBuffer1){
					//RETAILMSG(1,(_T( "m_pBuffer1 is NULL.Set bRecordSweepDone = TRUE\n")));
					 LogVerbose(_T("m_pBuffer1 is NULL.Set bRecordSweepDone = TRUE."));
			       // break;
				}
				if( WriteWaveFile()!=CESTRESS_PASS){
					RETAILMSG(1,(_T("WriteWaveFile fail.\r\n")));
                Result =CESTRESS_FAIL;
				goto Done;
				//break;
				}
				if(SaveWaveFile()!=CESTRESS_PASS){
                Result =CESTRESS_FAIL;
				goto Done;
				}
				if(CloseWaveDeviceForRecord()!=CESTRESS_PASS){
                Result =CESTRESS_FAIL;
				goto Done;
				}		
				LogVerbose(_T("Normal:Set bRecordSweepDone = TRUE"));
		
		//}
	
Done:
			if (hWavesave != INVALID_HANDLE_VALUE){
				   CloseHandle(hWavesave);
				   hWavesave = INVALID_HANDLE_VALUE;	
				  }
				  if(DeleteFile(FileName))
					{
					RETAILMSG(1,(_T("Delete file %s  success.\r\n"),FileName));
					}
				  else{
					  RETAILMSG(1,(_T("Delete file %s  fail.error=%x\r\n"),FileName,GetLastError()));
					}

	return Result;
}

UINT CleanupToneTest() 
{
UINT Result = CESTRESS_PASS;
//RETAILMSG(1,(_T( "CleanupToneTest!\r\n")));	
	LogVerbose(_T(" CleanupToneTest!"));
    if (m_hWaveOut != NULL)
    {
        m_bPlayBack = FALSE;
       // if (bReset)
        //{
        	waveOutReset(m_hWaveOut);
       // }

		// Unprepare the wave headers
		if( MMSYSERR_NOERROR != waveOutUnprepareHeader(m_hWaveOut,
			&m_WAVEHDR1.m_WaveHdr,
            sizeof(WAVEHDR))){
            LogFail(_T("waveOutUnprepareHeader1 fail."));
            Result = CESTRESS_FAIL;
			goto EXIT;
		}
		if(MMSYSERR_NOERROR != waveOutUnprepareHeader(m_hWaveOut,
			&m_WAVEHDR2.m_WaveHdr,
            sizeof(WAVEHDR))){
            LogFail(_T("waveOutUnprepareHeader1 fail."));
            Result = CESTRESS_FAIL;
			goto EXIT;
		}
	    waveOutClose(m_hWaveOut);
        m_hWaveOut = NULL;
    }
	//RETAILMSG(1,(_T( "CleanupToneTest done!\r\n")));
EXIT:
return Result;
	LogVerbose(_T(" CleanupToneTest done!"));
}
UINT OpenWaveFileForRecord()
{
//RETAILMSG(1,(_T( "OpenWaveFileForRecord!\r\n")));	
UINT Result = CESTRESS_PASS;
LogVerbose(_T("OpenWaveFileForRecord!"));
	//WAVEFORMATEX m_wfe;
	BOOL bRet = FALSE;
	WORD channels = 1;
	 long bufflen = 0;
	 MMRESULT mr = 0;
	// TCHAR FileNameBuffer[4];
	// Create the file
	//_itow(Random()%1000,FileNameBuffer,10);
	//m_hWaveDataFile = CreateFile(_T("temprec.wav"),
	m_hWaveDataFile = CreateFile(FileNameBuffer,
	GENERIC_WRITE | GENERIC_READ,
	FILE_SHARE_WRITE | FILE_SHARE_READ,
	NULL,
	CREATE_ALWAYS,
	FILE_ATTRIBUTE_NORMAL,
	NULL);
	if (m_hWaveDataFile == INVALID_HANDLE_VALUE){
		
	    LogFail(_T("%S:  CreateFile  %s fail.ERROR=%d"),__FUNCTION__,FileNameBuffer,GetLastError());
		bRet = FALSE;
		goto EXIT;
	}
	// Create buffers - two second buffer
	bRet = TRUE;
	//prepare the wave format struct
			m_wfeRecord.cbSize = 0;
			m_wfeRecord.nChannels = channels;
			m_wfeRecord.nSamplesPerSec = dwRate;
			m_wfeRecord.wBitsPerSample = wSize;
			m_wfeRecord.wFormatTag = WAVE_FORMAT_PCM;
			m_wfeRecord.nBlockAlign = (m_wfeRecord.nChannels * m_wfeRecord.wBitsPerSample) / 8;
			m_wfeRecord.nAvgBytesPerSec = m_wfeRecord.nSamplesPerSec * m_wfeRecord.nBlockAlign;
			//RETAILMSG(1,(_T("m_wfe.cbSize = %d\r\nm_wfe.nChannels = %d\r\nm_wfe.nSamplesPerSec = %d\r\nm_wfe.wBitsPerSample = %d\r\nm_wfe.nBlockAlign = %d\r\nm_wfe.nAvgBytesPerSec = %d\r\n"),m_wfe.cbSize,m_wfe.nChannels,m_wfe.nSamplesPerSec,m_wfe.wBitsPerSample,m_wfe.nBlockAlign,m_wfe.nAvgBytesPerSec));		
	bufflen = (2 * m_wfeRecord.nSamplesPerSec) * m_wfeRecord.nBlockAlign;
	//RETAILMSG(1,(_T( "	-- bufflen = %d"),bufflen));
	if (m_wfeRecord.wBitsPerSample == 8){
		m_pBuffer1 = new BYTE[bufflen];
		m_pBuffer2 = new BYTE[bufflen];
		if((!m_pBuffer1)||(!m_pBuffer2))

		{
		//RETAILMSG(1,(_T( "	- - RecordAudioThread--no enough heap memory to alloc.\r\n")));
			LogFail(_T( "  - - RecordAudioThread--no enough heap memory to alloc.\r\n"));
		    Result = CESTRESS_FAIL;
			goto EXIT;

		}
	}
	else{
		m_pBuffer1 = new short[bufflen / 2];
		m_pBuffer2 = new short[bufflen / 2];
		if((!m_pBuffer1)||(!m_pBuffer2))
{
		//RETAILMSG(1,(_T( "	- - RecordAudioThread--no enough heap memory to alloc.\r\n")));
			LogFail(_T( "  - - RecordAudioThread--no enough heap memory to alloc.\r\n"));
			   Result = CESTRESS_FAIL;
			goto EXIT;
		}
	}
	memset(m_pBuffer1, 0, bufflen);
	memset(m_pBuffer2, 0, bufflen);
	  // delete [] pRecordNode;
	////RETAILMSG(1,(_T( "RecordThreadStartEvent is signaled.\r\n")));			
	// m_RecordInProcess = TRUE;
	// Open wave in device
	 mr = waveInOpen(&m_hWaveIn,
			WAVE_MAPPER,
			&m_wfeRecord,
			(DWORD)waveInProc,
			NULL,
			CALLBACK_FUNCTION);
	if(mr != MMSYSERR_NOERROR){
		//RETAILMSG(1,(_T( "	- - waveInOpen--return fail...mr = %x,err= %x\r\n"),mr,GetLastError()));
		   LogFail(_T( "  - -   - - waveInOpen--return fail,mr = %x,err= %x\r\n"),mr,GetLastError());
			   Result = CESTRESS_FAIL;
			goto EXIT;
	}
	else {
		RETAILMSG(1,(_T( "	- - waveInOpen success\r\n")));
			bRecordSweepDone = FALSE;
	}
	//waveInReset(m_hWaveIn);
	// Prepare wave header one
	m_WavHdr1.dwFlags = 0;
	m_WavHdr1.dwBufferLength = bufflen;
	m_WavHdr1.lpData = (char *)m_pBuffer1;
	mr = waveInPrepareHeader(m_hWaveIn,
		&m_WavHdr1,
		sizeof(WAVEHDR));
	if(mr != MMSYSERR_NOERROR)
		{
		//RETAILMSG(1,(_T( "	- - waveInPrepareHeader1--return fail...mr = %x\r\n"),mr));
		LogFail(_T( "  - - waveInPrepareHeader1--return fail..\r\n"));
		   Result = CESTRESS_FAIL;
		goto EXIT;
	}
	//RETAILMSG(1,(_T( "  - - waveInPrepareHeader1 success.\r\n")));
	// Prepare wave header two
	m_WavHdr2.dwFlags = 0;
	m_WavHdr2.dwBufferLength = bufflen;
	m_WavHdr2.lpData = (char *)m_pBuffer2;
	mr = waveInPrepareHeader(m_hWaveIn,
		&m_WavHdr2,
		sizeof(WAVEHDR));
	if(mr != MMSYSERR_NOERROR){
		//RETAILMSG(1,(_T( "	- - waveInPrepareHeader2--return fail...mr = %x\r\n"),mr));
		LogFail(_T( "  - -   - - waveInPrepareHeader2--return fail.mr = %x\r\n"),mr);
		   Result = CESTRESS_FAIL;
		goto EXIT;
	}
	//RETAILMSG(1,(_T( "  - - waveInPrepareHeader2 success.\r\n"),mr));
	// Queue an input buffer
	mr = waveInAddBuffer(m_hWaveIn,
			&m_WavHdr1,
			sizeof(WAVEHDR));
	if(mr != MMSYSERR_NOERROR){
		//RETAILMSG(1,(_T( "	- - waveInAddBuffer--return fail...mr = %x\r\n"),mr));
		LogFail(TEXT( "  - -   - - waveInAddBuffer--return fail.mr = %x\r\n"),mr);
		   Result = CESTRESS_FAIL;
		 goto EXIT;
	}
	//RETAILMSG(1,(_T( "  - - waveInAddBuffer success.\r\n"),mr));
	// Start recording
	m_bStopRecording = FALSE;
	m_bSwitchBuffer = FALSE;
	//m_dwDuration = duration * 1000;
	g_dwRecordStartTick = GetTickCount();
	waveInStart(m_hWaveIn);	
EXIT:
	return Result;;
}
UINT WriteWaveFile()
{
//RETAILMSG(1,(_T( "WriteWaveFile!\r\n")));	
UINT Result = CESTRESS_PASS;
BOOL bRet = TRUE;
LogVerbose(_T("WriteWaveFile!"));
 DWORD dwBytesWritten = 0,
       dwBytesRead = 0;

//BOOL bRet = FALSE;
void *p = m_pBuffer1;	
			    WAVEHDR *pWaveHdr = &m_WavHdr1;
			       
				   //RETAILMSG(1,(_T( "  - - RecordAudioDoneEvent is signaled.\r\n")));
				   //m_wfe.cbSize = 0;
				   //m_wfe.nChannels = channels;
				   //m_wfe.nSamplesPerSec = g_sRandomPara.dwRate;
				   //m_wfe.wBitsPerSample = g_sRandomPara.dwSize;
				   //m_wfe.wFormatTag = WAVE_FORMAT_PCM;
				  // m_wfe.nBlockAlign = (m_wfe.nChannels * m_wfe.wBitsPerSample) / 8;
				   //m_wfe.nAvgBytesPerSec = m_wfe.nSamplesPerSec * m_wfe.nBlockAlign;

					// Write wave data
						if (m_bSwitchBuffer){
						    if(!m_pBuffer2){
							 // RETAILMSG(1,(_T( "m_pBuffer2 is NULL,set bRecordSweepDone\r\n")));
							  LogVerbose(_T("m_pBuffer2 is NULL,set bRecordSweepDone."));
							  //bRecordSweepDone = TRUE;
			                  goto EXIT;
							}
							p = m_pBuffer2;
							pWaveHdr = &m_WavHdr2;
						}
						 
						bRet = WriteFile(m_hWaveDataFile,
							p,
							pWaveHdr->dwBytesRecorded,
							&dwBytesWritten,
							NULL);
						if (!bRet){
						    RETAILMSG(1,(_T( " WriteFile m_hWaveDataFile fail,close it.error=%x\r\n"),GetLastError()));
						   LogFail(_T("WriteFile m_hWaveDataFile fail,close it.."));
							Result = CESTRESS_FAIL;
							CloseHandle(m_hWaveDataFile);
							m_hWaveDataFile = INVALID_HANDLE_VALUE;
							//bRecordSweepDone = TRUE;
							goto EXIT;			
						}
					    RETAILMSG(1,(_T( " WriteFile m_hWaveDataFile succeed.\r\n")));
						m_dwWaveDataLen += dwBytesWritten;		
						// Move the file pointer back to the beginning
						SetFilePointer(m_hWaveDataFile,
							0,
							NULL,
							FILE_BEGIN);
						m_fmtchunkhdr.dwCKID = RIFF_FORMAT;
					    m_fmtchunkhdr.dwSize = sizeof(PCMWAVEFORMAT);
					    m_datachunkhdr.dwCKID = RIFF_CHANNEL;
					    m_datachunkhdr.dwSize = m_dwWaveDataLen;

					    m_riffhdr.dwRiff = RIFF_FILE;
					    m_riffhdr.dwWave = RIFF_WAVE;
					    m_riffhdr.dwSize = 4 +
					        sizeof(RIFF_CHUNKHEADER) +
					        m_fmtchunkhdr.dwSize +
					        sizeof(RIFF_CHUNKHEADER) +
					        m_dwWaveDataLen;
						
EXIT:
	RETAILMSG(1,(_T( "WriteWaveFile return Result = %x\r\n"),Result));	
	return Result;

}
UINT SaveWaveFile()
{
//RETAILMSG(1,(_T( "SaveWaveFile!\r\n")));	
 LogVerbose(_T("SaveWaveFile!"));
UINT Result = CESTRESS_PASS;
    BOOL bRet = TRUE;
	//WAVEFORMATEX m_wfe;
    char copybuf[32768 + 1];
	DWORD dwBytesRead = 0,
        dwBytesToRead = 0,
        dwBytesRemaining = 0,
        dwBytesWritten = 0;
	//SaveWaveFile
	//HANDLE hWavesave = INVALID_HANDLE_VALUE;
    //TCHAR FileName[4];
	//_itow(Random()%1000,FileName,10);
	// Create the file
	//hWavesave = CreateFile(_T("RecordAudio.wav"),
	hWavesave = CreateFile(FileName,
	GENERIC_WRITE,
	FILE_SHARE_WRITE,
	NULL,
	CREATE_ALWAYS,
	FILE_ATTRIBUTE_NORMAL,
	NULL);
	if (hWavesave == INVALID_HANDLE_VALUE){
	//RETAILMSG(1,(_T(" RecordAudio Create fail.\r\n")));
	 LogFail(_T("RecordAudio Create  %s fail."),FileName);
	 bRet = FALSE;
	// bRecordSweepDone = TRUE;
	goto EXIT;
	}
	//RETAILMSG(1,(_T(" RecordAudio Create suceeded.\r\n")));
	// Write RIFF header
	bRet = WriteFile(hWavesave,
	(void *)&m_riffhdr,
	sizeof(RIFF_FILEHEADER),
	&dwBytesWritten,
	NULL);
	// Write format chunk header
	bRet = WriteFile(hWavesave,
	(void *)&m_fmtchunkhdr,
	sizeof(RIFF_CHUNKHEADER),
	&dwBytesWritten,
	NULL);
	if (!bRet){
	//RETAILMSG(1,(_T(" Write format chunk header fail.\r\n")));
	LogFail(_T("Write format chunk header fail."));
     Result = CESTRESS_FAIL;
	CloseHandle(hWavesave);
	hWavesave = INVALID_HANDLE_VALUE;
	//bRecordSweepDone = TRUE;
	goto EXIT;
	}
	//RETAILMSG(1,(_T(" Write format chunk header pass.\r\n")));
	// Write wave format
	bRet = WriteFile(hWavesave,
	(void *)&m_wfeRecord,
	m_fmtchunkhdr.dwSize,
	&dwBytesWritten,
	NULL);
	if (!bRet){
	//RETAILMSG(1,(_T(" Write wave format fail.\r\n")));
	LogFail(_T("Write wave format fail."));
	 Result = CESTRESS_FAIL;
	CloseHandle(hWavesave);
	hWavesave = INVALID_HANDLE_VALUE;
	//bRecordSweepDone = TRUE;
	goto EXIT;
	}
	//RETAILMSG(1,(_T(" Write wave format pass.\r\n")));
	// Write data chunk header
	bRet = WriteFile(hWavesave,
	(void *)&m_datachunkhdr,
	sizeof(RIFF_CHUNKHEADER),
	&dwBytesWritten,
	NULL);
	if (!bRet){
	 //RETAILMSG(1,(_T(" Write data chunk header fail.\r\n")));
	 LogFail(_T("Write data chunk header fail."));
	  Result = CESTRESS_FAIL;
	CloseHandle(hWavesave);
	hWavesave = INVALID_HANDLE_VALUE;
	goto EXIT;
	}
	//RETAILMSG(1,(_T(" Write data chunk header pass.\r\n")));
	// Write wave data
	dwBytesRemaining = m_datachunkhdr.dwSize;
	//RETAILMSG(1,(_T(" dwBytesRemaining = %d\r\n"),dwBytesRemaining));
	int j = 0;
	while (dwBytesRemaining){
		RETAILMSG(1,(_T(" ReadFile/WriteFile [ %d ]\r\n"),j++));
		dwBytesToRead = dwBytesRemaining;
		if (dwBytesToRead > 32768){
			dwBytesToRead = 32768;
		}

		bRet = ReadFile(m_hWaveDataFile,
			copybuf,
			dwBytesToRead,
			&dwBytesRead,
			NULL);
		if (!bRet){
			//RETAILMSG(1,(_T(" ReadFile m_hWaveDataFile = %x fail.\r\n"),m_hWaveDataFile));
			 LogFail(_T("ReadFile m_hWaveDataFile = %x fail."),m_hWaveDataFile);
			 Result = CESTRESS_FAIL;
			CloseHandle(hWavesave);
			hWavesave = INVALID_HANDLE_VALUE;
			goto EXIT;
		}
	  // RETAILMSG(1,(_T(" ReadFile m_hWaveDataFile=%x sucess.\r\n"),m_hWaveDataFile));
		bRet = WriteFile(hWavesave,
			copybuf,
			dwBytesRead,
			&dwBytesWritten,
			NULL);
		if (!bRet){
		    //RETAILMSG(1,(_T(" WriteFile hWavesave fail.\r\n")));
		    LogFail(_T("WriteFile hWavesave fail."));
			 Result = CESTRESS_FAIL;
			CloseHandle(hWavesave);
			hWavesave = INVALID_HANDLE_VALUE;
			// bRecordSweepDone = TRUE;
			goto EXIT;
		}
		//RETAILMSG(1,(_T(" WriteFile hWavesave pass.\r\n")));
		RETAILMSG(1,(_T(" dwBytesRead = %d\r\n"),dwBytesRead));
		dwBytesRemaining -= dwBytesRead;
		RETAILMSG(1,(_T(" dwBytesRemaining = %d\r\n"),dwBytesRemaining));
	}
	if (hWavesave != INVALID_HANDLE_VALUE){
				   CloseHandle(hWavesave);
				   hWavesave = INVALID_HANDLE_VALUE;	
				  }
	//CloseHandle(hWavesave);
   // hWavesave = INVALID_HANDLE_VALUE;
EXIT:
	return Result;
}
UINT CloseWaveDeviceForRecord()
{
UINT Result = CESTRESS_PASS;
RETAILMSG(1,(_T("CloseWaveDeviceForRecord \r\n")));
   LogVerbose(_T("CloseWaveDeviceForRecord "));
				// Recall the buffers from the codec
		  MMRESULT  mr = waveInReset(m_hWaveIn);
				if(mr != MMSYSERR_NOERROR){
                   //RETAILMSG(1, (TEXT("warning: waveInReset failed. mr=%08x\r\n"), mr));
				   LogFail(_T("waveInReset failed. mr=%08x"), mr);
				   Result = CESTRESS_FAIL;
				   goto EXIT;
				}
				//Sleep(500);
				RETAILMSG(1,(_T("mr = %x\r\n"),mr));

				// Unprepare the wave headers
			    if(waveInUnprepareHeader(m_hWaveIn,
			        &m_WavHdr1,
			        sizeof(WAVEHDR))){
				   LogFail(_T("waveInUnprepareHeader1 fail."));
				   Result = CESTRESS_FAIL;
				   goto EXIT;
				}

			    if(waveInUnprepareHeader(m_hWaveIn,
			        &m_WavHdr2,
			        sizeof(WAVEHDR))){
				   LogFail(_T("waveInUnprepareHeader1 fail."));
				   Result = CESTRESS_FAIL;
				   goto EXIT;
				}
EXIT:				
				// Delete buffers
			    if (m_pBuffer1!=NULL){
			        delete [] m_pBuffer1;
			        m_pBuffer1 = NULL;
			    }
			    if (m_pBuffer2!=NULL){
				    delete [] m_pBuffer2;
			        m_pBuffer2 = NULL;
			    }    
				// Close input device
			    waveInClose(m_hWaveIn);
			    m_hWaveIn = NULL;
			    if (m_hWaveDataFile != INVALID_HANDLE_VALUE){
			        CloseHandle(m_hWaveDataFile);
			        m_hWaveDataFile = INVALID_HANDLE_VALUE;			       
			    }
				if( DeleteFile(FileNameBuffer)){
                     RETAILMSG(1,(_T("DeleteFile temp %s success.\r\n"),FileNameBuffer));
			       	}
				   else{
					   RETAILMSG(1,(_T("DeleteFile temp %s fail error=%d\r\n"),FileNameBuffer,GetLastError()));
				   }

RETAILMSG(1,(_T("CloseWaveDeviceForRecord return\r\n")));
return Result;
}

void
CALLBACK
waveInProc
(
    HWAVEIN hWaveIn,
    UINT uMsg,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
)
{
BOOL bRC = FALSE;
DWORD dwBytesWritten = 0;
DWORD  dwCurrentTick = GetTickCount();
void *p = NULL;
//void *pPrintBuffer = p;
WAVEHDR *pAddWaveHdr = NULL,
        *pWriteWaveHdr = NULL;

//DWORD	m_dwDuration = 2000;
	
    //RETAILMSG(1,(_T( "  - - waveInProc --entry\r\n")));
    // Only act on the message that says that the codec
    // has put data in a buffer
    if (uMsg == WIM_DATA)
    {
    RETAILMSG(1,(_T( " --WIM_DATA is recieved.\r\n")));
	LogVerbose(_T("--WIM_DATA is recieved."));

		   	{
            if (!m_bStopRecording)
				{
				if (m_bSwitchBuffer == FALSE)
					{
					p = m_pBuffer1;
					pWriteWaveHdr = &m_WavHdr1;
					pAddWaveHdr = &m_WavHdr2;
					m_bSwitchBuffer = TRUE;

					  RETAILMSG(1,(_T( " --WIM_DATA +.\r\n")));
					  LogVerbose(_T("--WIM_DATA +."));
					}	
				else
					{
					p = m_pBuffer2;
					pWriteWaveHdr = &m_WavHdr2;
					pAddWaveHdr = &m_WavHdr1;
					m_bSwitchBuffer = FALSE;
					  RETAILMSG(1,(_T( " --WIM_DATA -.\r\n")));
					   LogVerbose(_T("--WIM_DATA -."));
					}
				//RETAILMSG(1,(_T( "  - - waveInProc--Prepare waveInAddBuffer.\r\n")));

				MMRESULT  mr = waveInAddBuffer(m_hWaveIn,
				pAddWaveHdr,
				sizeof(WAVEHDR));
				if( mr !=   MMSYSERR_NOERROR)
					{
					RETAILMSG(1,(_T( "  -- waveInAddBuffer return error,mr = %d\r\n"),mr));
					LogFail(_T(" -- waveInAddBuffer return error,mr = %d"),mr);
					//goto done;
					}
				else 
				//RETAILMSG(1,(_T( "  -- waveInAddBuffer return success,mr = %d\r\n"),mr));
					{
				if (pWriteWaveHdr->dwBytesRecorded > 0)
						{
							//{
							//int i=0;
							//while(i<128)
							//	{
							//	RETAILMSG(1,(_T( "%2x  %2x  %2x  %2x  %2x  %2x  %2x  %2x  \r\n"),*((BYTE*)p+i),*((BYTE*)p+i+1)
							//	,*((BYTE*)p+i+2),*((BYTE*)p+i+3),*((BYTE*)p+i+4),*((BYTE*)p+i+5),*((BYTE*)p+i+6),*((BYTE*)p+i+7)));
							//	i=i+8;
							//	}
							//}
						bRC = WriteFile(m_hWaveDataFile,
						p,
						pWriteWaveHdr->dwBytesRecorded,
						&dwBytesWritten,
						NULL);

						m_dwWaveDataLen += dwBytesWritten;
						//if (!bRC){
						//RETAILMSG(1,(_T( "waveInProc WriteFile return false,bRC = %x\r\n"),bRC));

						//}
						//else{
						//RETAILMSG(1,(_T( "waveInProc WriteFile return true,bRC = %x\r\n"),bRC));

						//}
						//memset(m_pBuffer1, 0, bufflen);
						// memset(m_pBuffer2, 0, bufflen);
						}
					}
				
				// First check to see if time is up 	
					if (dwCurrentTick > g_dwRecordStartTick + m_dwRecordDuration)
						{
						// Set the flag to stop recording
						m_bStopRecording = TRUE;
						RETAILMSG(1,(_T( "record time duration is up,set g_bFlagRecordTimeup TRUE.\r\n")));
						g_bFlagRecordTimeup = TRUE;
						//waveInReset(m_hWaveIn);
						//RETAILMSG(1,(_T( "record time duration is up,set RecordAudioDoneEvent-.\r\n")));
						//SetEvent(g_EventArray[RecordAudioDoneEvent]);
						}
				}
		   	}
    }	
return;
}

UINT ToneGenerator3(short *psinewavebuffer,struct sWaveHdr *pWaveHdr, float m_currentfreq)
{
//RETAILMSG(1,(_T("ToneGenerator3\r\n")));
LogVerbose(_T("ToneGenerator3"));
	BOOL bRet = FALSE;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    double amplitude = 0,
		SplitRecip = 0,
		GrowthFactor = 0,
		radians = 0,
		step = 0,
		samplesperperiod = CODEC_SAMPLE_RATE / m_currentfreq,
		samplespersecond = samplesperperiod * m_currentfreq,
        twopi = (2.0 * M_PI);
    short codecnum = 0;
	UINT i = 0,
		j = 0,
		limit = 0;
	double *m_psamples = new double [((int)CODEC_SAMPLE_RATE / 2)];

    if (!m_psamples)
    {
        goto EXIT;
    }

	step = (m_currentfreq * 2 * M_PI) / CODEC_SAMPLE_RATE;

	//while (1)
	while (i < ((int)CODEC_SAMPLE_RATE / 2))
    {
		m_psamples[i++] = sin(radians);

		radians += step;

        //if ((DWORD)(radians * 100000) == (DWORD)(twopi * 100000))
		if ((DWORD)(radians * 100000) - (DWORD)(twopi * 100000) < 1000)
        {
            break;
        }

        if (radians > twopi)
        {
            radians -= twopi;//Data abort.
        }
    }

	limit = i;

	i = 0;
	while (i < samplespersecond)
	{
		j = 0;
		while (j < limit)
		{
			amplitude = m_psamples[j++];
			if (amplitude >= 0)
			{
				codecnum = (short)round((0x7FFF * amplitude));
			}
			else
			{
				codecnum = (short)round((0x8000 * amplitude));
			}

			psinewavebuffer[i] = codecnum;
            i++;
		}
	}

	pWaveHdr->m_currentfreq = m_currentfreq;


	//SplitRecip = 1 / m_stepfreq;
	//GrowthFactor = pow(2 , SplitRecip); //1.0293922366 for 24th octaves
	//m_dwShadDwell--;
	//if (!m_dwShadDwell)
	//{
		//m_currentfreq += m_stepfreq;
	//	m_currentfreq = m_currentfreq * GrowthFactor;
	//	m_dwShadDwell = m_dwDwell;
	//}

EXIT:
	//	RETAILMSG(1,(_T("ToneGenerator3 return i=%d\r\n"),i));

	return i;
}

void CALLBACK waveOutProc2(HWAVEOUT hWaveDev,UINT uMsg,DWORD dwInstance,
						   DWORD dwParam1,DWORD dwParam2)
{
	DWORD dwDuration = 2000;

//RETAILMSG(1,(_T( " waveOutProc2 entry\r\n")));
	DWORD dwCurrentTick = GetTickCount();
	BOOL flag=TRUE;

	struct sWaveHdr *pWAVEHDRToPlay = NULL,
        *pWAVEHDR = (struct sWaveHdr *)dwParam1;
    if (uMsg == WOM_DONE)
    {     
        RETAILMSG(1,(_T( "waveOutProc2, recieve WOM_DONE\r\n")));
		LogVerbose(_T("waveOutProc2, recieve WOM_DONE"));
        //Check to see if time is up
	    if(dwCurrentTick > g_dwProduceStartTick + m_dwProduceDuration){
			RETAILMSG(1,(_T( " Produce is timing up,set g_bFlagProduceTimeup TRUE\r\n")));
			g_bFlagProduceTimeup = TRUE;
	        //SetEvent(g_EventArray[ProduceAudioDoneEvent]);
			goto EXIT;
		}
		if (!m_bPlayBack)
				{
				RETAILMSG(1,(_T( " m_bPlayBack false,goto exit.\r\n")));
					goto EXIT;
				}
		//RETAILMSG(1,(_T( " pWAVEHDR->nIndex =%d\r\n"),pWAVEHDR->nIndex));
		switch (pWAVEHDR->nIndex)
		{
		case 1:
            pWAVEHDRToPlay = &m_WAVEHDR1;
			break;

		case 2:
            pWAVEHDRToPlay = &m_WAVEHDR2;
			break;
		}
		
        // Play next buffer
        if (pWAVEHDRToPlay !=NULL)
        {  
        	  // 	RETAILMSG(1,(_T( " Produce - pWAVEHDRToPlay !=NULL\r\n")));
           if(m_hWaveOut!=NULL){	  
		  // 	RETAILMSG(1,(_T( " Produce - Play next buffer\r\n")));
		    waveOutWrite(m_hWaveOut,
			    &pWAVEHDRToPlay->m_WaveHdr,
			    sizeof(WAVEHDR));
           	}
        }

	}
EXIT:
    return;
}
