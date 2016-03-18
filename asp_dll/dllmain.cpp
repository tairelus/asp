// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

extern std::ofstream logFile;
extern const char* logName;
extern const char* targetModuleName;

extern BOOL ReplaceIATEntry(PCSTR targetModuleName, PROC originalProc, PROC userProc, HMODULE callerModule);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (!logFile.is_open()) logFile.open(logName, std::ofstream::out | std::ofstream::app);
	logFile << "DllMain entry." << " hModule = " << hModule << std::endl;
		
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{	
		logFile << "DLL_PROCESS_ATTACH" << std::endl;

		PROC writeFileOrig = ::GetProcAddress(::GetModuleHandle(targetModuleName), "WriteFile");
		PROC writeFileHook = ::GetProcAddress(hModule, "WriteFile_Hook");
		ReplaceIATEntry(targetModuleName, writeFileOrig, writeFileHook, ::GetModuleHandle(NULL));
	}
		break;
	case DLL_THREAD_ATTACH:
		logFile << "DLL_THREAD_ATTACH" << std::endl;
		break;
	case DLL_THREAD_DETACH:
		logFile << "DLL_THREAD_DETACH" << std::endl;
		break;
	case DLL_PROCESS_DETACH:
		logFile << "DLL_PROCESS_DETACH" << std::endl;
		break;
	}
		
	logFile.close();

	return TRUE;
}

