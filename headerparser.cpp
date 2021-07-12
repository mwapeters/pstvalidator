#include "stdafx.h"
#include "headerparser.h"
#include "ndbport.h"
#include <iostream>

HeaderParser::HeaderParser(int clusterSize)
{
	clustersize = clusterSize;
}

bool HeaderParser::ParseHeader(BYTE* pbufRead, int buffersize, int iteration, ULONGLONG* offset)
{
	DWORD data;
	DWORD dwCRC;
	bool partialCRCPass = FALSE;
	bool fullCRCPass = FALSE;
	bool success = FALSE;
	for (int i = 0; i < buffersize - 3; i++)
	{
		data = (pbufRead[i] << 24) | (pbufRead[i + 1] << 16) | (pbufRead[i + 2] << 8) | (pbufRead[i + 3]);
		if (data == dwMagicHL || data == dwMagicLH)
		{
			memcpy(&this->header, pbufRead, sizeof(this->header));
			dwCRC = ComputeCRC((((BYTE*)&this->header) + ibHeaderCRCPartialStart), cbHeaderCRCPartial);
			partialCRCPass = (dwCRC == this->header.dwCRCPartial);
			dwCRC = ComputeCRC((((BYTE*)&this->header) + ibHeaderCRCFullStart), cbHeaderCRCFull);
			fullCRCPass = (dwCRC == this->header.dwCRCFull);

			if (fullCRCPass && partialCRCPass)
			{
				success = TRUE;
				*offset = (iteration * clustersize) + i;
				validHeader = TRUE;
			}
			else
			{
				validHeader = FALSE;
				success = FALSE;
			}
		}
	}
	return success;
}