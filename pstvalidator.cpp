// pstvalidator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <afx.h>
#include <iostream>
#include "headerparser.h"
#include "pageparser.h"
#include "blockparser.h"
#include "fileintegrity.h"
#include "parsers.h"
#include "logging.h"
#include "fileoperations.h"

int main(int argc, char* argv[])
{	
	const int clustersize = 4096;  //define the clustersize used by the validator
	char* filename;
	if (argc == 2)
	{
		std::cout << argv[1] << "\n";
		filename = argv[1];
	} 
	else
	{
		std::cout << "please provide the file or disk name, in the following format: \\\\.\\h:\\example.pst" << "\n";
		filename = (char*)"";
	}
	FileHandler filehandler(filename, clustersize);
	HeaderParser headerparser (clustersize);
	Reporter reporter;
	PageParser pageparser (reporter.ClusterReport(), reporter.FileReport(), reporter.DataBlocksReport(), clustersize);
	BlockParser blockparser (reporter.ClusterReport(), reporter.FileReport(), reporter.DataBlocksReport(), clustersize);

	DWORD bytesRead;
	BYTE pbufRead[clustersize];
	ULONGLONG iteration = 0;

	ULONGLONG startOfFile = 0;    // contains the location from which the data is recognized as valid PST file format
	ULONGLONG pstFileSize = 0;    // contains the PST file size in case a valid PST header is recognized
	ULONGLONG headeroffset = -1;  // contains the location in the current data from which the first header or Amap data structure is recognized.
	ULONGLONG headerBID = -1;     // contains the BID value of the first recognized header or Amap structure, 0 in case of header and the BID of the Amap is the absolute file offset within a PST file. 
	ULONGLONG corruptFrom = -1;   // contains the location from which the data no longer adheres to the PST file format or page/block corruption detected.
	ULONGLONG validUntil = 0;     // contains the location until the data is still valid according the PST file format

	bool headerFound = FALSE;
	bool secondHeaderFound = FALSE;
	bool invalidPageFound = FALSE;

	while (filehandler.ReadNextCluster(pbufRead, &bytesRead) && (!secondHeaderFound || !invalidPageFound))
	{
		// header recognition
		if (headerparser.ParseHeader((BYTE*)&pbufRead, sizeof(pbufRead), iteration, &headeroffset))
		{
			if (headerFound)
			{
				if ((iteration * clustersize) < startOfFile + pstFileSize)
				{
					secondHeaderFound = TRUE;
					corruptFrom = headeroffset;
				}
			}
			else
			{
				std::cout << "Valid PST header detected\n";
				headerBID = 0;
				startOfFile = (iteration * clustersize) + headeroffset;
				pstFileSize = headerparser.GetFileSize();
				validUntil = pstFileSize;
				std::cout << " start of file ";
				std::cout << startOfFile << std::endl;
				std::cout << " filesize ";
				std::cout << pstFileSize << std::endl;
				headerFound = TRUE;
			}
		}
		//  AMap used as header, in case no PST header present.
		if (!headerFound)
		{
			ULONGLONG foundHeaderBID;
			headerFound = pageparser.FindHeader((BYTE*)&pbufRead, sizeof(pbufRead), iteration, &headeroffset, &foundHeaderBID);
			if (headerFound)
			{
				startOfFile = headeroffset;
				headerBID = foundHeaderBID;
			}
		}
		
		//  page recognition, header offset must be known for validation
		if (!secondHeaderFound && headerFound)
		{
			ULONGLONG invalidPageLoc = -1;
			if (!pageparser.ValidatePages((BYTE*)&pbufRead, sizeof(pbufRead), iteration, headeroffset, headerBID, &invalidPageLoc))
			{
				if (invalidPageLoc < corruptFrom)
				{
					corruptFrom = invalidPageLoc;
				}
			}
		}
		validUntil += bytesRead;
		iteration++;
	}
	// determine upper bound: if pst size is known than upper bound is the pst file size, otherwise the first corruption point is used as upper bound.
	if (pstFileSize > 0 && corruptFrom > pstFileSize)
	{
		corruptFrom = pstFileSize;
	}
	else
	{
		if (corruptFrom == -1)
		{
			corruptFrom = validUntil;
		}
	}
	std::cout << " page validation, corrupt from: ";
	std::cout << corruptFrom << std::endl;
	// block validation, goes through the data a second time, since the references required to detect blocks are now available
	iteration = 0;
	ULONGLONG invalidBlockLoc = -1;
	filehandler.StartOfFile();
	while (filehandler.ReadNextCluster(pbufRead, &bytesRead) && ((iteration * clustersize) < corruptFrom))
	{
		if (!blockparser.ValidateBlocks((BYTE*)&pbufRead, sizeof(pbufRead), iteration, headeroffset, headerBID, pageparser.GetBlockRefs(), &invalidBlockLoc))
		{
			if (invalidBlockLoc < corruptFrom)
			{
				corruptFrom = invalidBlockLoc;
			}
		}
		iteration++;
	}
	std::cout << " block validation, corrupt from: ";
	std::cout << corruptFrom << std::endl;
	//  file integrity validation
	if (filehandler.FileOpen())
	{
		FileIntegrity fileintegrity(startOfFile, headerBID, corruptFrom, pageparser.GetRecurringData());
		ULONGLONG invalidRecurringStructure = -1;
		if (!fileintegrity.VerifyRecurringStructures(&invalidRecurringStructure))
		{
			if (invalidRecurringStructure < corruptFrom)
			{
				corruptFrom = invalidRecurringStructure;
			}
		}
		ULONGLONG invalidReference = -1;
		if (!fileintegrity.VerifyReferences(pageparser.GetBlockLocs(), blockparser.GetBlocksFound(), &invalidReference))
		{
			if (invalidReference < corruptFrom)
			{
				corruptFrom = invalidReference;
			}
		}
	}
	std::cout << " start of file ";
	std::cout << startOfFile << std::endl;
	std::cout << " end of file ";
	std::cout << corruptFrom << std::endl;
}
