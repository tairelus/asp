// asp_dll.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "asp_dll.h"

std::ofstream logFile;
const char* logName = "asp_log.log";
const char* targetModuleName = "KERNEL32.dll";

// Prototypes for the hooked functions
typedef int (WINAPI* PFNWRITEFILE) (
	HANDLE hFile,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped);

/// <summary>
/// This function will replace WriteFile() from kernel32.dll
/// </summary>
/// <returns></returns>
ASP_LIBAPI BOOL WriteFile_Hook(
	HANDLE hFile,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped)
{
	if (!logFile.is_open()) 
		logFile.open(logName, std::ofstream::out | std::ofstream::app);
	
	logFile << "\t" << "WriteFile_Hook: hFile = " << hFile << std::endl;

	PFNWRITEFILE origProc = (PFNWRITEFILE)::GetProcAddress(::GetModuleHandle(targetModuleName), "WriteFile");
	BOOL Result = origProc(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);

	if (Result)
	{
		const char* add_text = "!!!";
		DWORD bytesWritten = 0;
		Result = origProc(hFile, add_text, strlen(add_text), &bytesWritten, NULL);
	}

	return Result;
}

/// <summary>
/// Replaces the IAT entry.
/// </summary>
/// <param name="targetModuleName">Name of the target module.</param>
/// <param name="originalProc">The original proc.</param>
/// <param name="userProc">The user proc.</param>
/// <param name="callerModule">The caller module.</param>
/// <returns></returns>
BOOL ReplaceIATEntry(PCSTR targetModuleName, PROC originalProc, PROC userProc, HMODULE callerModule) {

	if (!logFile.is_open()) 
		logFile.open(logName, std::ofstream::out | std::ofstream::app);
	
	logFile << "\t" << "targetModuleName = " << targetModuleName << std::endl;
	logFile << "\t" << "originalProc = " << originalProc << std::endl;
	logFile << "\t" << "userProc = " << userProc << std::endl;
	logFile << "\t" << "callerModule = " << callerModule << std::endl;

	// Get the address of the caller module's import section
	ULONG cbSize = 0;
	PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)::ImageDirectoryEntryToData(callerModule, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &cbSize);
	logFile << "\t" << "importDesc = " << importDesc << std::endl;

	if (importDesc == NULL)
		return FALSE; // This module has no import section
	
	// Find the import descriptor containing references to callee's functions
	while (importDesc->Name != 0)
	{
		PSTR moduleName = (PSTR)((PBYTE)callerModule + importDesc->Name);
		logFile << "\t" << "moduleName = " << moduleName << "; targetModuleName = " << targetModuleName << std::endl;

		if (_stricmp(moduleName, targetModuleName) == 0)
			break;
		
		importDesc++;
	}

	// This module doesn't import any functions from this callee
	if (importDesc->Name == 0)
		return FALSE;
		
	// Get caller's import address table (IAT) for the callee's functions
	PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)((PBYTE)callerModule + importDesc->FirstThunk);
	logFile << "\t" << "Caller's IAT: pThunk = " << pThunk << std::endl;

	// Replace current function address with new function address
	while (pThunk->u1.Function != 0)
	{
		// Get the address of the function address
		PROC* ppfn = (PROC*)&pThunk->u1.Function;
		logFile << "\t" << "ppfn = " << *ppfn << "; originalProc = " << originalProc << std::endl;

		if (*ppfn == originalProc) {
			
			// The addresses match, change the import section address
			MEMORY_BASIC_INFORMATION mbi = {0};
			::VirtualQuery(ppfn, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
			
			BOOL Result = ::VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_READWRITE, &mbi.Protect);
			logFile << "\t" << "Change protection: Result = " << Result << std::endl;
			if (!Result)
				return Result;
						
			Result = ::WriteProcessMemory(::GetCurrentProcess(), ppfn, &userProc, sizeof(userProc), NULL);
			logFile << "\t" << "Replace original function: Result = " << Result << std::endl;
						
			// Restore the protection back
			DWORD oldProtect;
			::VirtualProtect(mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &oldProtect);
			
			return Result;
		}

		pThunk++;
	}

	return FALSE;
}
