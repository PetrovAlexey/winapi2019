#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <assert.h>
#include <set>
#include <fstream>
#include <sstream>
#include <iterator>
#include <algorithm>

static const std::string NamePrefix("petrov.workers.");

struct CMappedFile {
	HANDLE fileHandle, mappingHandle;
	PVOID mappedFilePtr;
	size_t size;
};

struct CTaskOpertaion {
	int q;
	enum Task { FilterWords, FilterLetters, Close };
	Task task;
	HANDLE FileToEdit; // Handle на файл для редактирования
	HANDLE ArgumentsFile; // Handle на файл с аргументами - списком слов или порогом отсечения
};


std::set<std::string> ReadDictionary(std::string &str) {
	std::set<std::string> dictionary;
	std::stringstream s(str); // Used for breaking words 
	while (!s.eof()) {
		std::string word;
		s >> word;
		dictionary.insert(word);
	}
	return dictionary;
}

std::string wordsFilter(const std::string &source, const std::set<std::string> &dictioinary) {
	std::string result = {};
	std::istringstream iss(source);
	std::vector<std::string> tokens;
	std::copy(std::istream_iterator<std::string>(iss),
		std::istream_iterator<std::string>(),
		std::back_inserter<std::vector<std::string> >(tokens));
	for (int i = 0; i < tokens.size(); ++i) {
		if (dictioinary.find(tokens[i]) == dictioinary.end()){
			result += (tokens[i] +" ");
		}
	}
	return result;
}

void countFreq(std::string str, std::vector<double> &res) {
	std::vector<double> a(256);
	int local = (int)('a');
	int len = str.size() - std::count(str.begin(), str.end(), ' ');
	for (int i = 0; i < 256; ++i) {
		a[i] = ((double)(std::count(str.begin(), str.end(), (char)(i + local))) / len);
		//std::cout << a[i] << std::endl;
	}
	res = a;
	
}

std::string wordsFreqFilter(const std::string &source, double fet) {
	std::vector<double> freq;
	countFreq(source, freq);
	std::string result = {};
	std::istringstream iss(source);
	std::vector<std::string> tokens;
	std::copy(std::istream_iterator<std::string>(iss),
		std::istream_iterator<std::string>(),
		std::back_inserter<std::vector<std::string> >(tokens));
	for (int i = 0; i < tokens.size(); ++i) {
		bool good = true;
		for (int j = 0; j < tokens[i].size(); ++j) {
			if (freq[(int)(tokens[i][j] - 'a')] < fet) {
				good = false;
			}
		}
		if (good) {
			result += (tokens[i] + " ");
		}
	}
	return result;
}




HANDLE OpenDataEvent(DWORD processId)
{
	std::string name = std::string(NamePrefix) + (std::to_string(processId)) + ("DataIsReady");
	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, (name.c_str()));
	if (hEvent == NULL) {
		std::cout << "DataReadyEventError" << std::endl;
	}
	return hEvent;
}

HANDLE GetDataEvent(DWORD processId)
{
	std::string name = std::string(NamePrefix) + std::to_string(processId) + ("DataIsReady");
	auto dataIsReadyEvent = CreateEvent(0, FALSE, FALSE, name.c_str());
	if (dataIsReadyEvent == INVALID_HANDLE_VALUE) {
		throw std::runtime_error("Failed to create event");
	}
	return dataIsReadyEvent;
}

HANDLE GetWorkEvent(DWORD processId)
{
	std::string name = std::string(NamePrefix) + std::to_string(processId) + ("WorkIsDone");
	auto workIsDoneEvent = CreateEvent(0, FALSE, FALSE, name.c_str());
	if (workIsDoneEvent == INVALID_HANDLE_VALUE) {
		throw std::runtime_error("Failed to create event");
	}
	return workIsDoneEvent;
}

int main(int argc, char *argv[])
{
	std::string init = std::string(NamePrefix) + ("Init");
	HANDLE initEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, (init.c_str()));
	if (initEvent == NULL) {
		std::cout << "DataReadyEventError" << std::endl;
	}
	HANDLE dataReady = GetDataEvent(GetCurrentProcessId());
	HANDLE workReady = GetWorkEvent(GetCurrentProcessId());
	SetEvent(initEvent);

	const DWORD BUFFER_SIZE = 1024;
	HANDLE hPipe;
	std::string numWork;
	if (argc > 1) {
		numWork = argv[1];
	}
	else {
		std::cout << "Need arguments" << std::endl;
		return 0;
	}
	std::cout << "Worker " << numWork << std::endl;
	while (1)
	{
		std::string name = "\\\\.\\pipe\\testpipe" + numWork;
		hPipe = CreateFile(name.c_str(),
			GENERIC_READ,
			0,                   // no sharing 
			NULL,                // default security attributes
			OPEN_EXISTING,   // opens existing pipe 
			0,                // default attributes 
			NULL);           // no template file 

// Break if the pipe handle is valid. 

		if (hPipe != INVALID_HANDLE_VALUE)
			break;


		// Exit if an error other than ERROR_PIPE_BUSY occurs. 

		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			printf("Could not open pipe %d\n", GetLastError());
			return -1;
		}

		// All pipe instances are busy, so wait for sometime.

		if (!WaitNamedPipe(name.c_str(), NMPWAIT_USE_DEFAULT_WAIT))
		{
			printf("Could not open pipe: wait timed out.\n");
		}
	}


	
	printf("Returned\n");
	

	DWORD bytes_to_read = BUFFER_SIZE;
	char *buf = reinterpret_cast<char *>(malloc(bytes_to_read));
	DWORD bytes_read;

	
	while (TRUE) {
		std::cout << "Waitig for job" << std::endl;
		DWORD waitResult = WaitForSingleObject(dataReady, INFINITE);
		switch (waitResult) {
		case WAIT_FAILED:
		case WAIT_TIMEOUT:
			assert(false);
		case WAIT_OBJECT_0:
			printf("Waiting for read\n");
			ReadFile(hPipe, buf, sizeof(CTaskOpertaion), &bytes_read, 0);
			CTaskOpertaion *temp = reinterpret_cast<CTaskOpertaion*>(buf);
			std::string dicString;
			std::string editString;
			SetFilePointer(temp->ArgumentsFile, 0, NULL, FILE_BEGIN);
			SetFilePointer(temp->FileToEdit, 0, NULL, FILE_BEGIN);
			//std::cout << bytes_read << " " << sizeof(CTaskOpertaion) << std::endl;
			if (bytes_read != sizeof(CTaskOpertaion))
			{
				printf("ReadFile from pipe failed. GLE \n");
			}
			else {
				printf("Read %d bytes: %s\n", bytes_read, buf);
				
				
				if (temp->task == temp->Close) {
					std::cout << "Close worker" << std::endl;
					SetEvent(workReady);
					ResetEvent(dataReady);
					CloseHandle(dataReady);
					CloseHandle(workReady);
					CloseHandle(initEvent);
					CloseHandle(hPipe);
					return 0;
					break;
				}
				///

				if (temp->task == temp->FilterWords) {
					std::string freqString;
					SetFilePointer(temp->ArgumentsFile, 0, NULL, FILE_BEGIN);
					TCHAR szBuf[BUFFER_SIZE] = {};
					DWORD nWritten;
					while (ReadFile(temp->ArgumentsFile, szBuf, BUFFER_SIZE, &nWritten, NULL)) {
						freqString += szBuf;
						std::wcout << (szBuf) << std::endl;
						if (nWritten < BUFFER_SIZE) {
							break;
						}
					}
					double fet = atof(freqString.c_str());
					TCHAR szTextBuf[BUFFER_SIZE] = {};
					DWORD nTextWritten;
					while (ReadFile(temp->FileToEdit, szTextBuf, BUFFER_SIZE, &nTextWritten, NULL)) {
						editString += szTextBuf;
						std::wcout << (szTextBuf) << std::endl;
						if (nTextWritten < BUFFER_SIZE) {
							break;
						}
					}

					std::string resultString = wordsFreqFilter(editString, fet);
					std::cout << "resultsString: " << resultString << std::endl;

					DWORD dwBytesWritten = 0;
					SetFilePointer(temp->FileToEdit, 0, NULL, FILE_BEGIN);
					SetEndOfFile(temp->FileToEdit);

					DWORD dwBytesToWrite = (DWORD)(resultString.size() + 1);
					bool bErrorFlag = WriteFile(
						temp->FileToEdit,           // open file handle
						reinterpret_cast<LPCVOID> (resultString.c_str() + '\0'),      // start of data to write
						resultString.size() + 1,  // number of bytes to write
						&dwBytesWritten, // number of bytes that were written
						NULL);            // no overlapped structure
					std::cout << "bytes " << dwBytesWritten << std::endl;

					if (FALSE == bErrorFlag)
					{
						std::cout << "WriteFile" << std::endl;
						printf("Terminal failure: Unable to write to file.\n");
					}
					else
					{
						if (dwBytesWritten != dwBytesToWrite)
						{

							std::cout << "Error: dwBytesWritten != dwBytesToWrite" << std::endl;
						}
						else
						{
							std::cout << "Wrote successfully" << std::endl;
							SetEvent(workReady);
							ResetEvent(dataReady);
						}
					}

				}

				if (temp->task == temp->FilterLetters) {
					SetFilePointer(temp->ArgumentsFile, 0, NULL, FILE_BEGIN);
					TCHAR szBuf[BUFFER_SIZE] = {};
					DWORD nWritten;
					while (ReadFile(temp->ArgumentsFile, szBuf, BUFFER_SIZE, &nWritten, NULL)) {
						dicString += szBuf;
						std::wcout << (szBuf) << std::endl;
						if (nWritten < BUFFER_SIZE) {
							break;
						}
					}

					std::set<std::string> dict = ReadDictionary(dicString);
					//dict.erase(dict.find(""));
					std::cout << dict.size() << std::endl;
					for (auto & person : dict)
					{
						std::cout << " <" << person << "> ";
					}
					TCHAR szTextBuf[BUFFER_SIZE] = {};
					DWORD nTextWritten;
					while (ReadFile(temp->FileToEdit, szTextBuf, BUFFER_SIZE, &nTextWritten, NULL)) {
						editString += szTextBuf;
						std::wcout << (szTextBuf) << std::endl;
						if (nTextWritten < BUFFER_SIZE) {
							break;
						}
					}

					std::string resultString = wordsFilter(editString, dict);
					std::cout << "resultsString: " << resultString << std::endl;
					
					DWORD dwBytesWritten = 0;
					SetFilePointer(temp->FileToEdit, 0, NULL, FILE_BEGIN);
					SetEndOfFile(temp->FileToEdit);
					//DWORD dwBytesToWrite = (DWORD)(resultString.size());
					DWORD dwBytesToWrite = (DWORD)(resultString.size()+1);
					bool bErrorFlag = WriteFile(
						temp->FileToEdit,           // open file handle
						reinterpret_cast<LPCVOID> (resultString.c_str()+'\0'),      // start of data to write
						resultString.size()+1,  // number of bytes to write
						&dwBytesWritten, // number of bytes that were written
						NULL);            // no overlapped structure
					std::cout << "bytes " << dwBytesWritten << std::endl;
					
					if (FALSE == bErrorFlag)
					{
						std::cout << "WriteFile" << std::endl;
						printf("Terminal failure: Unable to write to file.\n");
					}
					else
					{
						if (dwBytesWritten != dwBytesToWrite)
						{

							std::cout << "Error: dwBytesWritten != dwBytesToWrite" << std::endl;
						}
						else
						{
							std::cout << "Wrote successfully" << std::endl;
							SetEvent(workReady);
							ResetEvent(dataReady);
						}
					}

				}
				///
			}
			SetEvent(workReady);
			ResetEvent(dataReady);
			break;
		}
		
	}
	

	CloseHandle(hPipe);
	return 0;
}