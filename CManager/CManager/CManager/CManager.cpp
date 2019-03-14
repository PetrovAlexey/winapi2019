#include "CManager.h";
static const std::string NamePrefix("petrov.workers.");



HANDLE GetHandleFile(std::string const &path) {
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	HANDLE hFile;
	DWORD dwBytesRead;
	char buf[512];
	hFile = CreateFile(path.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		&sa,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		std::cerr << "Could not open file" << std::endl;
		assert(false);
	}
	return hFile;
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

HANDLE OpenWorkEvent(DWORD processId)
{
	std::string name = std::string(NamePrefix) + (std::to_string(processId)) + ("WorkIsDone");
	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, (name.c_str()));
	if (hEvent == NULL) {
		std::cout << "DataReadyEventError" << std::endl;
	}
	return hEvent;
}

HANDLE GetDataEvent(DWORD processId)
{
	auto name = std::string(NamePrefix).append(std::to_string(processId).append("DataIsReady"));
	auto dataIsReadyEvent = CreateEvent(0, FALSE, FALSE, name.c_str());
	if (dataIsReadyEvent == INVALID_HANDLE_VALUE) {
		throw std::runtime_error("Failed to create event");
	}
	return dataIsReadyEvent;
}



void CManager::Close() {
	for (int i = 0; i < numberOfWorker; ++i) {
		if (!DisconnectNamedPipe(hPipes[i]))
		{
			printf("Disconnect failed %d\n", GetLastError());
		}
		else
		{
			printf("Disconnect successful\n");
		}
		TerminateProcess(pi[i].hProcess, NO_ERROR);
	}
	closed = 1;
}

void CManager::WaitAll() {
	while (!workerQueue.empty()) {
		Sleep(100);
	}
	while (1) {
		int q = -1;
		for (int i = 0; i < numberOfWorker; ++i) {
			if (stateWorkers[i] == 1) {
				q = i;
				break;
			}
		}
		if (q == -1) {
			return;
		}
	}
	
}


void CManager::InsideScheduler() {
while (TRUE) {
		DWORD result;
		if (closed) {
			return;
		}
		std::cout << "Waiting for Workers" << std::endl;
		result = WaitForMultipleObjects(numberOfWorker, stateWorkersReady, FALSE, INFINITY);
		if (result - WAIT_OBJECT_0 < numberOfWorker) {
			std::cout << "Found Worker" << std::endl;
			DWORD mutexResult = WaitForSingleObject(mutex, INFINITY);
			while (mutexResult != WAIT_OBJECT_0) {
				mutexResult = WaitForSingleObject(mutex, INFINITY);
			}
			
			int numWorker = result - WAIT_OBJECT_0;
			CTaskOpertaion temp;
			if (!workerQueue.empty()) {
				temp = workerQueue.front();
				workerQueue.pop();
				DWORD bytes_written;
				//ConnectNamedPipe(hPipes[numWorker], 0);
				if (!WriteFile(hPipes[numWorker], (LPVOID)&temp, sizeof(temp), &bytes_written, NULL))
				{
					DWORD er = GetLastError();
					char errs[200];
					sprintf_s(errs, "Error : %ld", er);
					printf("Error communicating to client.\n");
					printf(errs);
				}
				HANDLE hEvent = OpenDataEvent(pi[numWorker].dwProcessId);
				SetEvent(hEvent);
				//FlushFileBuffers(hPipes[numWorker]);
			}
			else {
				stateWorkers[numWorker] = 0;
			}

			ReleaseMutex(mutex);
		}
		else {
			Sleep(1000);
		}
	}
}

CManager::~CManager() {
	for (int i = 0; i < numberOfWorker; ++i) {
		if (!DisconnectNamedPipe(hPipes[i]))
		{
			printf("Disconnect failed %d\n", GetLastError());
		}
		else
		{
			printf("Disconnect successful\n");
		}
		TerminateProcess(pi[i].hProcess, NO_ERROR);
	}
	delete[] stateWorkersReady;
	delete[] stateDataReady;
	delete[] pi;
	delete[] cif;
	delete[] hPipes;
}

void CManager::ScheduleTask(const CTaskOpertaion& operation) {
	CTaskOpertaion temp = operation;

	DWORD mutexResult = WaitForSingleObject(mutex, INFINITE);
	while (mutexResult != WAIT_OBJECT_0) {
		mutexResult = WaitForSingleObject(mutex, INFINITE);
	}
	///
	

	// Проверяем свободных воркеров
	int freeWorker = -1;
	for (int i = 0; i < numberOfWorker; ++i) {
		if (stateWorkers[i] == 0) {
			stateWorkers[i] = 1;
			freeWorker = i;
			DWORD bytes_written;
			if (!WriteFile(hPipes[i], (LPVOID)&temp, sizeof(CTaskOpertaion), &bytes_written, NULL))
			{
				DWORD er = GetLastError();
				char errs[200];
				sprintf_s(errs, "Error : %ld", er);
				printf("Error communicating to client.\n");
				printf(errs);
			}
			HANDLE hEvent = OpenDataEvent(pi[freeWorker].dwProcessId);
			ResetEvent(stateWorkersReady);
			SetEvent(hEvent);
			break;
			}
		}
	
	if (freeWorker == -1) {
		workerQueue.push(operation);
	}
	ReleaseMutex(mutex);
}

CManager::CManager(int numberOfWorkers) {
	closed = false;
	numberOfWorker = numberOfWorkers;
	mutex = CreateMutex(NULL, FALSE, "queue");
	stateWorkers = std::vector<int>(numberOfWorkers);
	stateWorkersReady = new HANDLE[numberOfWorkers];
	stateDataReady = new HANDLE[numberOfWorkers];
	//set params process
	pi = new PROCESS_INFORMATION[numberOfWorkers];
	cif = new STARTUPINFO[numberOfWorkers];
	hPipes = new HANDLE[numberOfWorkers];

	std::string name = std::string(NamePrefix) + ("Init");
	auto InitEvent = CreateEvent(0, FALSE, FALSE, name.c_str());
	if (InitEvent == INVALID_HANDLE_VALUE) {
		throw std::runtime_error("Failed to create event");
	}

	for (int i = 0; i < numberOfWorkers; ++i) {
		std::string name = "\\\\.\\pipe\\testpipe"+ std::to_string(i);
		hPipes[i] = CreateNamedPipe((name.c_str()),
			PIPE_ACCESS_OUTBOUND |
			FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE |
			PIPE_READMODE_MESSAGE |
			PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			BUFFER_SIZE,
			BUFFER_SIZE,
			2000,
			NULL);
		if (hPipes[i] == INVALID_HANDLE_VALUE)
		{
			std::cerr << "Error while creating pipe" << std::endl;
			assert(false);
		}
		ZeroMemory(&cif[i], sizeof(STARTUPINFO));
		/*(CreateProcess("C:\\Users\\Алексей\\source\\repos\\Worker\\Debug\\Worker.exe", NULL,
			NULL, NULL, TRUE, NULL, NULL, NULL, &cif, &pi) == TRUE)*/
		std::string procName = "C:\\Users\\Алексей\\source\\repos\\Worker\\Debug\\Worker.exe " + std::to_string(i);
		if (CreateProcess(NULL, (LPSTR)procName.c_str(), NULL, NULL, TRUE, NULL, NULL, NULL, &cif[i], &pi[i]) == TRUE) {
			std::cout << "process" << std::endl;
			std::cout << "handle " << pi[i].hProcess << std::endl;
		}
		ConnectNamedPipe(hPipes[i], 0);
		DWORD waitResult = WaitForSingleObject(InitEvent, INFINITE);
		if (waitResult == WAIT_OBJECT_0) {
			stateDataReady[i] = OpenDataEvent(pi[i].dwProcessId);
			stateWorkersReady[i] = OpenWorkEvent(pi[i].dwProcessId);
		}
	}
	
	HANDLE HandleThread = CreateThread(NULL, 0, Wrap_Thread_no_1, this, 0, NULL);
	if (HandleThread == NULL)
	{
		std::cerr << "CreateThread error: " <<  GetLastError() << std::endl;
		assert(false);
	}
	
}
