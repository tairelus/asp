// asp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

/// <summary>
/// Gets the process identifier by its name.
/// </summary>
/// <param name="exeName">Name of the executable.</param>
/// <returns></returns>
DWORD GetProcessIdByName(const std::string& exeName)
{
	DWORD Result = 0;
	DWORD dwCount = 0;

	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hSnapshot != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 pe = { 0 };
		pe.dwSize = sizeof(PROCESSENTRY32);

		BOOL bRet = ::Process32First(hSnapshot, &pe);

		while (bRet)
		{
			if (!_stricmp(pe.szExeFile, exeName.c_str()))
			{
				dwCount++;
				Result = pe.th32ProcessID;
								
			}
			bRet = ::Process32Next(hSnapshot, &pe);
		}

		if (dwCount > 1)
			Result = 0xFFFFFFFF;

		::CloseHandle(hSnapshot);
	}

	return Result;
}

/// <summary>
/// Injects the pszLibFile library into dwProcessId process.
/// </summary>
/// <param name="dwProcessId">The process identifier.</param>
/// <param name="pszLibFile">The library file.</param>
/// <returns></returns>
BOOL WINAPI InjectLib(DWORD dwProcessId, PCWSTR pszLibFile) {

	BOOL Result = FALSE; // Assume that the function fails
	HANDLE hProcess = NULL, hThread = NULL;
	PWSTR pszLibFileRemote = NULL;

	__try {
		// Get a handle for the target process.
		hProcess = ::OpenProcess(
			PROCESS_QUERY_INFORMATION | // Required by Alpha
			PROCESS_CREATE_THREAD | // For CreateRemoteThread
			PROCESS_VM_OPERATION | // For VirtualAllocEx/VirtualFreeEx
			PROCESS_VM_WRITE, // For WriteProcessMemory
			FALSE, dwProcessId);
		
		if (hProcess == NULL) 
			__leave;

		// Calculate the number of bytes needed for the DLL's pathname
		int cch = 1 + ::lstrlenW(pszLibFile);
		int cb = cch * sizeof(WCHAR);

		// Allocate space in the remote process for the pathname
		pszLibFileRemote = (PWSTR)::VirtualAllocEx(hProcess, NULL, cb, MEM_COMMIT, PAGE_READWRITE);
		if (pszLibFileRemote == NULL) 
			__leave;
		
		// Copy the DLL's pathname to the remote process's address space
		if (!::WriteProcessMemory(hProcess, pszLibFileRemote, (PVOID)pszLibFile, cb, NULL)) 
			__leave;

		// Get the real address of LoadLibraryW in Kernel32.dll
		PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)::GetProcAddress(::GetModuleHandle("KERNEL32.dll"), "LoadLibraryW");
		if (pfnThreadRtn == NULL) 
			__leave;

		// Create a remote thread that calls LoadLibraryW(DLLPathname)
		hThread = ::CreateRemoteThread(hProcess, NULL, 0, pfnThreadRtn, pszLibFileRemote, 0, NULL);
		if (hThread == NULL) 
			__leave;

		// Wait for the remote thread to terminate
		::WaitForSingleObject(hThread, INFINITE);

		Result = TRUE;
	}
	__finally { 

		// Free the remote memory that contained the DLL's pathname
		if (pszLibFileRemote != NULL)
			::VirtualFreeEx(hProcess, pszLibFileRemote, 0, MEM_RELEASE);

		if (hThread != NULL)
			::CloseHandle(hThread);

		if (hProcess != NULL)
			::CloseHandle(hProcess);
	}

	return Result;
}

int _tmain(int argc, _TCHAR* argv[])
{
	const char* logName = "asp_log.log";
	::DeleteFile(logName);

	DWORD proc_id = GetProcessIdByName("notepad.exe"); //"asp_test.exe"
	BOOL res = InjectLib(proc_id, L"asp_dll.dll"); 
		
	return 0;
}

