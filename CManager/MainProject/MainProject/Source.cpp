#include "CManager.h";

std::vector<std::string> splitString(std::string str, int N) {
	std::vector<std::string> splits(N);
	std::string result = {};
	std::istringstream iss(str);
	std::vector<std::string> tokens;
	std::copy(std::istream_iterator<std::string>(iss),
		std::istream_iterator<std::string>(),
		std::back_inserter<std::vector<std::string> >(tokens));
	for (int i = 0; i < N; ++i) {
		for (int j = i * (tokens.size() / N); j < (i + 1)*(tokens.size() / N); ++j) {
			splits[i] += (tokens[j] + " ");
		}
	}
	return splits;
}

void mergeFile(std::string path, int n) {
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	DWORD dwBytesRead;
	char buf[1024];
	std::string resultString;
	for (int i = 0; i < n; ++i) {

		std::string name = "temp" + std::to_string(i) + ".txt";
		HANDLE current = CreateFile(name.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			&sa,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (current == INVALID_HANDLE_VALUE)
		{
			std::cerr << "Error Creating tempfile" << std::endl;
			assert(false);
		}
		TCHAR szTextBuf[1024] = {};
		DWORD nTextWritten;
		while (ReadFile(current, szTextBuf, 1024, &nTextWritten, NULL)) {
			resultString += szTextBuf;
			if (nTextWritten < 1024) {
				break;
			}
		}
		CloseHandle(current);
	}
	std::cout << "ResultString " << resultString << std::endl;

	HANDLE resultFile = CreateFile(path.c_str(),
		GENERIC_ALL,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		&sa,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (resultFile == INVALID_HANDLE_VALUE)
	{
		std::cerr << "Error Creating tempfile" << std::endl;
		assert(false);
	}
	DWORD dwBytesWritten = 0;
	bool bErrorFlag = WriteFile(
		resultFile,           // open file handle
		reinterpret_cast<LPCVOID> (resultString.c_str()),      // start of data to write
		resultString.size(),  // number of bytes to write
		&dwBytesWritten, // number of bytes that were written
		NULL);            // no overlapped structure
	if (FALSE == bErrorFlag)
	{
		std::cout << "WriteFile" << std::endl;
		printf("Terminal failure: Unable to write to file.\n");
	}
	Sleep(1000);
	CloseHandle(resultFile);
}

void splitFile(HANDLE file, int n) {
	std::string editString;
	//HANDLE currrent;
	TCHAR szTextBuf[1024] = {};
	DWORD nTextWritten;
	while (ReadFile(file, szTextBuf, 1024, &nTextWritten, NULL)) {
		editString += szTextBuf;
		if (nTextWritten < 1024) {
			break;
		}
	}
	CloseHandle(file);
	std::cout << editString << std::endl;
	std::vector<std::string> splits = splitString(editString, n);
	//params CreateFiles
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	DWORD dwBytesRead;
	char buf[1024];
	for (int i = 0; i < n; ++i) {
		std::string name = "temp" + std::to_string(i) + ".txt";
		HANDLE current = CreateFile(name.c_str(),
			GENERIC_ALL,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			&sa,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (current == INVALID_HANDLE_VALUE)
		{
			std::cerr << GetLastError() << std::endl;
			std::cerr << "Error Creating tempfile" << std::endl;
			assert(false);
		}
		DWORD dwBytesWritten = 0;
		bool bErrorFlag = WriteFile(
			current,           // open file handle
			reinterpret_cast<LPCVOID> (splits[i].c_str()),      // start of data to write
			splits[i].size(),  // number of bytes to write
			&dwBytesWritten, // number of bytes that were written
			NULL);            // no overlapped structure
		if (FALSE == bErrorFlag)
		{
			std::cout << "WriteFile" << std::endl;
			printf("Terminal failure: Unable to write to file.\n");
		}
		CloseHandle(current);
	}

}

HANDLE getHandleFile(std::string path) {
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	HANDLE hFile;
	DWORD dwBytesRead;
	char buf[512];
	hFile = CreateFile(path.c_str(),
		GENERIC_ALL,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		&sa,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("Could not open file (error %d)\n", GetLastError());
		return 0;
	}
	return hFile;
}

HANDLE makeHandleFile(std::string path) {
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	HANDLE hFile;
	DWORD dwBytesRead;
	char buf[512];
	hFile = CreateFile(path.c_str(),
		GENERIC_ALL,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		&sa,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("Could not create file (error %d)\n", GetLastError());
		return 0;
	}
	return hFile;
}


int main(void)
{
	int n = 8; //Number splits
	int m = 2; // Number Workers
	splitFile(getHandleFile("C:\\Users\\Алексей\\source\\repos\\Worker\\Debug\\text.txt"), n);

	HANDLE hFile = getHandleFile("C:\\Users\\Алексей\\source\\repos\\Worker\\Debug\\Dict.txt");
	HANDLE hFileFreq = getHandleFile("C:\\Users\\Алексей\\source\\repos\\Worker\\Debug\\Freq.txt");

	std::vector<HANDLE> hTempFiles(n);
	for (int i = 0; i < n; ++i) {
		hTempFiles[i] = getHandleFile("temp" + std::to_string(i) + ".txt");
	}


	CManager mag(m);
	CTaskOpertaion temp;

	temp.q = 0;
	for (int i = 0; i < n; ++i) {
		temp.FileToEdit = hTempFiles[i];
		if (i % 2) {
			temp.task = temp.FilterLetters;
			temp.ArgumentsFile = hFile;
		}
		else {
			temp.task = temp.FilterWords;
			temp.ArgumentsFile = hFileFreq;
		}

		mag.ScheduleTask(temp);
	}
	mag.WaitAll();
	mag.Close();
	for (int i = 0; i < n; ++i) {
		CloseHandle(hTempFiles[i]);
	}
	Sleep(1000);
	mergeFile("Results.txt", n);

	int q;
	std::cin >> q;
}