#pragma once
#include <windows.h>

class FileHandler
{
public:
	FileHandler(char * filename, int clustersize);
	bool FileOpen();
	void StartOfFile();
	bool ReadNextCluster(BYTE* pbufRead, DWORD* bytesRead);
	void CloseFile();
private:
	HANDLE hDevice = INVALID_HANDLE_VALUE;  // handle to the drive to be examined
	DWORD filePointer;
	int clusterSize;
};
