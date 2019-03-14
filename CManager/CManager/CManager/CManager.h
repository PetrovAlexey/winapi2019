#pragma once
#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <assert.h>
#include <vector>
#include <cstdio>
#include <queue>
#include <algorithm>
#include <sstream>
#include <iterator>

class CTaskOpertaion {
public:
	int q;
	enum Task { FilterWords, FilterLetters, Close }; // Enum: { FilterWords, FilterLetters,Close
	Task task;
	HANDLE FileToEdit; // Handle на файл для редактирования
	HANDLE ArgumentsFile; // Handle на файл с аргументами - списком слов или порогом отсечения
};

class CManager {
public:
	void InsideScheduler();
	explicit CManager(int numberOfWorkers); // numberOfWorkers -сколько процессов Worker создать
	~CManager();
	void ScheduleTask(const CTaskOpertaion& operation); // Добавить в пул задачу
	void WaitAll(); // Дождаться исполнения всех задач
	void Close(); // Дождаться всех дочерних Worker, и завершить их работу
//private:
	int closed;
	const DWORD BUFFER_SIZE = 1024;
	std::vector<int> stateWorkers;
	HANDLE *stateWorkersReady;
	HANDLE *stateDataReady;
	PROCESS_INFORMATION *pi;
	STARTUPINFO *cif;
	HANDLE *hPipes;
	//std::vector<PROCESS_INFORMATION> pi;
	std::queue<CTaskOpertaion> workerQueue;
	HANDLE mutex;
	int numberOfWorker;
	static DWORD WINAPI Wrap_Thread_no_1(LPVOID lpParam)
	{
		CManager *self = reinterpret_cast<CManager*>(lpParam);
		self->InsideScheduler();
		return 0;
	}
};