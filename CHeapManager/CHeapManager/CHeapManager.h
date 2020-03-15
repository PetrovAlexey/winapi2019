// Автор: Петров Алексей
// Аллокатор на куче
#pragma once
#include <Windows.h>
#include <memory>
#include <iostream>
#include<stdio.h>
#include<set>
#include<map>
#include <utility>
#include"ITEM.h"


class CHeapManager {
public:
	// max_size - размер зарезервированного региона памяти при создании
	// min_size - размер закоммиченного региона при создании
	CHeapManager(int min_size, int max_size);
	CHeapManager() {}
	void CHeapCreate(int min_size, int max_size);
	// Вся память должна быть освобождена после окончании работы
	~CHeapManager();
	// Аллоцировать регион размера size
	void* Alloc(int size); 
	// Освободить память
	void Free(void* ptr);
	// Вывод информации
	void printInfo();
	static void* operator new(size_t sz)
	{
		void* m = malloc(sz);
		std::cout << "Dummy :: Operator new" << std::endl;
		return m;
	}
	// Overloading CLass specific delete operator
	static void operator delete(void* m)
	{
		std::cout << "Dummy :: Operator delete" << std::endl;
		free(m);
	}
private:
	void *hHeap;
	int freeSize;
	int pageSize = 4096;
	int maxSize;
	std::map<LPVOID, ITEM> sm;
	std::map<LPVOID, ITEM> md;
	std::map<LPVOID, ITEM> bg;
	std::map<LPVOID, std::pair<bool, int> > pages;
	std::map<LPVOID, ITEM>* buckets[3];
	
	int getBucket(int size);
	void printBuckets(std::map<LPVOID, ITEM>* mapBuckets);
	void printAllBuckets();
	void printPages();
	void memoryCommiter(void* address, int allocatedSize, bool commit);
	ITEM mergeBuckets(std::map<LPVOID, ITEM>::iterator bucketIt, int index);
};

void* CHeapManager::Alloc(int size)
{
	int allocSize = size;
	if (size % 4 != 0)
		allocSize = (size / 4 + 1) * 4;
	if (freeSize < allocSize) {
		std::wcout << L"No memory\n" << freeSize << std::endl;

		return nullptr;
	}

	int index = getBucket(size);
	bool found = false;
	int mapIndex;
	LPVOID addres = nullptr;
	int bucketSize;
	std::map<LPVOID, ITEM>::iterator itup;
	for (int i = index; i < 3 && !found; ++i) {
		for (itup = buckets[i]->begin(); itup != buckets[i]->end() && !found; ++itup) {
			if ((itup->second.size >= size) && (itup->second.state == true)) {
				found = true;
				bucketSize = itup->second.size;
				addres = itup->second.addres;
				buckets[i]->erase(itup);
				mapIndex = i;
				break;
			}
		}
	}

	buckets[getBucket(allocSize)]->insert(std::make_pair(addres, ITEM(allocSize, addres, false)));
	freeSize -= allocSize;
	LPVOID tempAddres = reinterpret_cast<LPVOID>(reinterpret_cast<byte*>(addres) + allocSize);
	if (freeSize) {
		buckets[getBucket(freeSize)]->insert(std::make_pair(tempAddres, ITEM(bucketSize - allocSize, tempAddres, true)));
	}
	memoryCommiter(addres, allocSize, true);

	return addres;
}

int CHeapManager::getBucket(int size)
{
	if (size <= pageSize)
		return 0;
	if (size <= 128 * sizeof(char))
		return 1;
	return 2;
}

ITEM CHeapManager::mergeBuckets(std::map<LPVOID, ITEM>::iterator bucketIt, int index) {
	
	int size = bucketIt->second.size;
	void* address = bucketIt->second.addres;
	void* newAddress = address;
	bool flag = false;
	
	if (bucketIt != buckets[index]->begin()) {
		bucketIt--;
		if (bucketIt != buckets[index]->end() && bucketIt->second.state) {
			newAddress = bucketIt->second.addres;
			size += bucketIt->second.size;
			buckets[index]->erase(address);
			buckets[index]->erase(newAddress);
			buckets[index]->insert(std::make_pair(newAddress, ITEM(size, newAddress, true)));
			flag = true;
		}
	}

	bucketIt = buckets[index]->find(newAddress);
	if (bucketIt != buckets[index]->end()) {
		bucketIt++;
		if (bucketIt != buckets[index]->end() && bucketIt->second.state) {
			size += bucketIt->second.size;
			buckets[index]->at(newAddress).size = size;
			address = bucketIt->second.addres;
			buckets[index]->erase(address);
			flag = true;
		}
	}

	int newIndex = getBucket(size);

	if (index != newIndex && flag) {
		buckets[index]->erase(newAddress);
		buckets[newIndex]->insert(std::make_pair(newAddress, ITEM(size, newAddress, true)));
	}

	buckets[newIndex]->at(newAddress).state = true;
	//return (newAddress, size);
	return ITEM(size, newAddress);
}

void CHeapManager::Free(void* ptr) {
	std::map<LPVOID, ITEM>::iterator bucketIt;
	int sizeFree = 0;
	int index = 0;
	LPVOID addres = nullptr;
	for (int i = 0; i < 3; ++i) {
		bucketIt = buckets[i]->find(ptr);
		if (bucketIt != buckets[i]->end()) {
			index = i;
			sizeFree = bucketIt->second.size;
			addres = bucketIt->second.addres;
			break;
		}
	}
	freeSize += sizeFree;
	memoryCommiter(addres, sizeFree, false);
	mergeBuckets(bucketIt, index);
}

void CHeapManager::memoryCommiter(LPVOID address, int alloc_size, bool commit) {

	std::map<LPVOID, std::pair<bool, int>>::iterator it;
	it = pages.upper_bound(address);
	LPVOID firstPage = nullptr;
	if (it != pages.end()) {
		firstPage = reinterpret_cast<LPVOID>(reinterpret_cast<byte*>(it->first) - pageSize);
	}
	int rest = reinterpret_cast<byte*>(firstPage) + pageSize - reinterpret_cast<byte*>(address);
	int num = 0;
	if ((alloc_size - rest) % pageSize != 0) {
		num = (alloc_size - rest) / pageSize + 1;
	}
	else {
		num = (alloc_size - rest) / pageSize;
	}
	num++;
	if (alloc_size < rest) {
		num = 1;
	}

	auto current = pages.find(firstPage);
	int sizeCommit = alloc_size;
	LPVOID pagesCommit = nullptr;
	bool startedCommit = false;
	if (commit) {
		for (int i = 0; i < num && current != pages.end(); i++) {
			if (current->second.first) {
				if (i == 0) {
					VirtualAlloc(address, rest, MEM_COMMIT, PAGE_READWRITE);
					current->second.second += 1;
				}
				else {
					VirtualAlloc(current->first, pageSize, MEM_COMMIT, PAGE_READWRITE);
					current->second.second += 1;
				}

			}
			else {
				VirtualAlloc(current->first, pageSize, MEM_COMMIT, PAGE_READWRITE);
				current->second.first = true;
				current->second.second += 1;
				sizeCommit -= pageSize;
			}
			current++;
		}
	}
	else {
		int temp = reinterpret_cast<byte*>(address) - reinterpret_cast<byte*>(firstPage);
		if ((alloc_size + temp) % pageSize != 0) {
			num = (alloc_size + temp) / pageSize + 1;
		}
		else {
			num = (alloc_size + temp) / pageSize;
		}
		for (int i = 0; i < num && current != pages.end(); i++) {
			if (current->second.first) {
				current->second.second -= 1;
				if (current->second.second == 0) {
					VirtualFree(current->first, pageSize, MEM_DECOMMIT);
					current->second.first = false;
				}
			}
			else {
					VirtualFree(current->first, pageSize, MEM_DECOMMIT);
					current->second.first = false;
			}
			current++;
		}
	}

}

CHeapManager::CHeapManager(int min_size, int max_size)
{
	if (min_size % pageSize != 0) {
		min_size = pageSize*(min_size/pageSize+1);
	}
	if (max_size % pageSize != 0) {
		max_size = pageSize*(max_size/pageSize+1);
	}
	freeSize = max_size;
	maxSize = max_size;
	hHeap = VirtualAlloc(NULL, max_size, MEM_RESERVE, PAGE_READWRITE);
	VirtualAlloc(hHeap, min_size, MEM_COMMIT, PAGE_READWRITE);


	buckets[0] = &sm;
	buckets[1] = &md;
	buckets[2] = &bg;
	buckets[getBucket(max_size)]->insert(std::make_pair(hHeap, ITEM(max_size, hHeap, true)));

	for (byte* address = reinterpret_cast<byte*>(hHeap); address < reinterpret_cast<byte*>(hHeap) + max_size; address += pageSize) {
		pages[reinterpret_cast<LPVOID>(address)].first = false;
		pages[reinterpret_cast<LPVOID>(address)].second = 0;
	}
}
void CHeapManager::CHeapCreate(int min_size, int max_size)
{
	if (min_size % pageSize != 0) {
		min_size = pageSize * (min_size / pageSize + 1);
	}
	if (max_size % pageSize != 0) {
		max_size = pageSize * (max_size / pageSize + 1);
	}
	freeSize = max_size;
	maxSize = max_size;
	hHeap = VirtualAlloc(NULL, max_size, MEM_RESERVE, PAGE_READWRITE);
	VirtualAlloc(hHeap, min_size, MEM_COMMIT, PAGE_READWRITE);


	buckets[0] = &sm;
	buckets[1] = &md;
	buckets[2] = &bg;
	buckets[getBucket(max_size)]->insert(std::make_pair(hHeap, ITEM(max_size, hHeap, true)));

	for (byte* address = reinterpret_cast<byte*>(hHeap); address < reinterpret_cast<byte*>(hHeap) + max_size; address += pageSize) {
		pages[reinterpret_cast<LPVOID>(address)].first = false;
		pages[reinterpret_cast<LPVOID>(address)].second = 0;
	}
}

CHeapManager::~CHeapManager()
{
	VirtualFree(pages.begin()->first, maxSize, MEM_RELEASE);
}

void CHeapManager::printInfo()
{
	printAllBuckets();
	printPages();
}

void CHeapManager::printBuckets(std::map<LPVOID, ITEM>* mapBuckets)
{
	std::map<LPVOID, ITEM>::iterator it = mapBuckets->begin();
	for (int i = 0; i < 10 && it != mapBuckets->end(); ++it) {
		std::wcout << it->second.addres << L" => size: " << it->second.size << L"  free: " << it->second.state << std::endl;
		++i;
	}
}

void CHeapManager::printAllBuckets()
{
	if (sm.size()) {
		std::wcout << L"Size < 4 KB:\n";
		printBuckets(&sm);
	}
	if (md.size()) {
		std::wcout << L"4 KB < Size < 128 KB:\n";
		printBuckets(&md);
	}
	if (bg.size()) {
		std::wcout << L"Size > 128 KB:\n";
		printBuckets(&bg);
	}
}

void CHeapManager::printPages()
{
	std::wcout << L"Pages: \n";
	int temp = 0;
	std::map<LPVOID, std::pair<bool, int> >::iterator it;
	for (it = pages.begin(); it != pages.end(); ++it) {
		std::wcout << it->first << " ~ " << it->second.first << L' ' << it->second.second << std::endl;
		temp++;
		if (temp > 10) {
			break;
		}
	}
}