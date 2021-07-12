#pragma once
#include "ndbport.h"

class HeaderParser
{
public:
	HeaderParser(int clusterSize);
	bool ParseHeader(BYTE* pbufRead, int buffersize, int iteration, ULONGLONG* offset);
	HEADER GetHeader() { return header; }
	ULONGLONG GetFileSize() { return header.root.ibFileEof; }
	bool ValidHeader() { return validHeader; }
private:
	int clustersize;
	HEADER header;
	bool validHeader = FALSE;
};
