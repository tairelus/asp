// asp_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int _tmain(int argc, _TCHAR* argv[])
{
	std::cout << "Press any key to continue..." << std::endl;
	std::cin.get();
	
	const char* fileName = "d:\\asp_test.txt";
	::DeleteFile(fileName);
	
	HANDLE hFile = ::CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		std::cout << "File created successfully: " << fileName << std::endl;

		const char* buffer = "Hello world";
		DWORD numberOfBytesWritten = 0;
		if (::WriteFile(hFile, buffer, strlen(buffer), &numberOfBytesWritten, NULL))
			std::cout << "File written successfully: " << numberOfBytesWritten << " bytes" << std::endl;
	}
	
	std::cout << "Press any key to exit..." << std::endl;
	std::cin.get();
	
	return 0;
}

