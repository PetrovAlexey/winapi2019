#pragma once
#include <cmath>
#include <time.h>
#include <random>
#include "CHeapManager.h"
#define ITER_NUM 100
HANDLE DefHeap;

std::random_device rd;  //Will be used to obtain a seed for the random number engine
std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
std::uniform_int_distribution<> dis(1, 1024);



class CTest
{
public:
	void* operator new(size_t size) {
		return HeapAlloc(DefHeap, 0, 1024);
	}
	void operator delete(void*mem) {
		HeapFree(DefHeap, 0, mem);
	}

};

class CRandomDefaultTest
{
public:
	void* operator new(size_t size) {
		return HeapAlloc(DefHeap, 0, dis(gen) * 1024);
	}
	void operator delete(void* mem) {
		HeapFree(DefHeap, 0, mem);
	}
};



void MyHeapTest() {
	CHeapManager MyHeap(1024, 1024 * 1024);
	clock_t start, finish;
	start = clock();
	int* arr[ITER_NUM];
	for (int i = 0; i < ITER_NUM; ++i) {
		arr[i] = reinterpret_cast<int*>(MyHeap.Alloc(1024));
	}
	for (int i = 0; i < ITER_NUM; ++i) {
		MyHeap.Free(arr[i]);
	}
	finish = clock();
	std::wcout << L"my heap time on simple test: " << float(finish - start) / CLOCKS_PER_SEC << std::endl;
	std::cout << "Yeah Boy" << std::endl;
}

void LiteTest() {
	CHeapManager MyHeap(1024, 1024 * 10);
	clock_t start, finish;
	start = clock();
	int* mas[7];
	mas[0] = reinterpret_cast<int*>(MyHeap.Alloc(1025));
	MyHeap.printInfo();
	for (int i = 1; i < 6; ++i) {
		mas[i] = reinterpret_cast<int*>(MyHeap.Alloc(1024));
		MyHeap.printInfo();
	}
	mas[6] = reinterpret_cast<int*>(MyHeap.Alloc(1023));
	MyHeap.printInfo();
	for (int i = 0; i < 7; ++i) {
		MyHeap.Free(mas[i]);
		MyHeap.printInfo();
	}

	finish = clock();
	std::wcout << L"CHeapManager time on LiteTest: " << float(finish - start) / CLOCKS_PER_SEC << std::endl;
}

void DefaultHeapTest() {
	clock_t start, finish;
	DefHeap = HeapCreate(0, 1024, 1024 * 1024);
	start = clock();
	CTest* arr[ITER_NUM];
	for (int i = 0; i < ITER_NUM; ++i) {
		arr[i] = new CTest;
	}
	for (int i = 0; i < ITER_NUM; ++i) {
		CTest::operator delete(arr[i]);
	}
	HeapDestroy(DefHeap);
	finish = clock();
	std::wcout << L"Heap time on SimpleTest: " << float(finish - start) / CLOCKS_PER_SEC << std::endl;
}

void MyHeapRandomTest() {
	clock_t start, finish;
	CHeapManager MyHeap(1024, 1024 * 1024 * ITER_NUM);
	start = clock();
	int* arr[ITER_NUM];
	for (int i = 0; i < ITER_NUM; ++i) {
		arr[i] = reinterpret_cast<int*>(MyHeap.Alloc(dis(gen) * 1024));
	}
	for (int i = 0; i < ITER_NUM; ++i) {
		MyHeap.Free(arr[i]);
	}
	finish = clock();
	std::wcout << L"CHeapManager time on RandomTest: " << float(finish - start) / CLOCKS_PER_SEC << std::endl;
}

void RandomDefaultHeapTest() {
	clock_t start, finish;
	DefHeap = HeapCreate(0, 1024, 1024 * 1024 * ITER_NUM);
	start = clock();
	CRandomDefaultTest* arr[ITER_NUM];
	for (int i = 0; i < ITER_NUM; ++i) {
		arr[i] = new CRandomDefaultTest;
	}
	for (int i = 0; i < ITER_NUM; ++i) {
		CRandomDefaultTest::operator delete(arr[i]);
	}
	HeapDestroy(DefHeap);
	finish = clock();
	std::wcout << L"Heap time on RandomTest: " << float(finish - start) / CLOCKS_PER_SEC << std::endl;
}