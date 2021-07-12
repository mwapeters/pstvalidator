#include "fileoperations.h"
#include <winnt.h>
#include <fileapi.h>
#include <WinNls.h>
#include <iostream>

FileHandler::FileHandler(char* filename, int clustersize)
{
	const WCHAR* pwcsName; //LPCWSTR
	int size = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
	pwcsName = new WCHAR[size];
	MultiByteToWideChar(CP_ACP, 0, filename, -1, (LPWSTR)pwcsName, size);
	hDevice = CreateFileW(pwcsName,          // drive to open
		GENERIC_READ,                        // read access to the drive
		FILE_SHARE_READ | // share mode
		FILE_SHARE_WRITE,
		NULL,             // default security attributes
		OPEN_EXISTING,    // disposition
		0,                // file attributes
		NULL);            // do not copy file attributes
	clusterSize = clustersize;
	if (hDevice != INVALID_HANDLE_VALUE)
	{
		filePointer = SetFilePointer(hDevice, 0, NULL, FILE_BEGIN);
	}
	else 
	{
		std::cout << "unable to open file: " << filename << "\n";
		filePointer = 0;
	}
	delete[] pwcsName;
}

bool FileHandler::FileOpen()
{
	return (hDevice != INVALID_HANDLE_VALUE);
}

void FileHandler::StartOfFile()
{
	if (hDevice != INVALID_HANDLE_VALUE)
	{
		filePointer = SetFilePointer(hDevice, 0, NULL, FILE_BEGIN);
	}
}

bool FileHandler::ReadNextCluster(BYTE* pbufRead, DWORD* bytesRead)
{
	bool result = FALSE;	
	if (hDevice != INVALID_HANDLE_VALUE)
	{
		result = ReadFile(hDevice, pbufRead, clusterSize, bytesRead, (LPOVERLAPPED)NULL);
		if (*bytesRead == 0)
		{
			result = FALSE;
		}
	}
	else
	{
		*bytesRead = 0;
		result = FALSE;
	}
	return result;
}

void FileHandler::CloseFile()
{
	if (hDevice != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hDevice);
	}
}


