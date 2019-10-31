#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <queue>
#include <string>
#include <windows.h>
#include <time.h>

using namespace std;

FILE* openFile(char* fileName, char* mode)
{
	FILE* fp = fopen(fileName, mode);
	if (fp == NULL)
	{
		perror("Error while opening the file.\n");
		exit(EXIT_FAILURE);
	}
	return fp;
}

int prodAmount = 4;
int consAmount = 4;
int num_ways = 40;
const int run_size = 25000;
int fileNameId = num_ways;

queue<string> out;

HANDLE hMutex;

struct threadParam
{
	int* tArr;
	int amount;
};

struct threadMergeParam
{
	queue<string> filesQueue;
};

threadMergeParam mergeParam;

DWORD WINAPI producerFunc(LPVOID param)
{
	threadParam* prm = (threadParam*)param;
	int numb = 0;
	for (int i = 0; i < prm->amount; i++)
	{
		vector<int> toSort;
		copy(&prm->tArr[run_size * i], &prm->tArr[run_size * i + run_size], back_inserter(toSort));
		sort(toSort.begin(), toSort.end());

		WaitForSingleObject(hMutex, INFINITE);

		FILE* outFile = openFile((char*)out.front().c_str(), (char*)"w");

		for (int j = 0; j < run_size; j++)
			fprintf(outFile, "%d ", toSort[j]);

		mergeParam.filesQueue.push((char*)out.front().c_str());
		out.pop();
		fclose(outFile);

		ReleaseMutex(hMutex);
	}
	return 0;
}

DWORD WINAPI consumerFunc(LPVOID param)
{
	threadMergeParam* prm = (threadMergeParam*)param;

	WaitForSingleObject(hMutex, INFINITE);

	while (prm->filesQueue.size() > 1)
	{
		char fileName[5];
		snprintf(fileName, sizeof(fileName), "%d", fileNameId);
		FILE* temp = openFile(fileName, (char*)"w");

		FILE* first = openFile((char*)(prm->filesQueue.front()).c_str(), (char*)"r");
		prm->filesQueue.pop();

		FILE* second = openFile((char*)(prm->filesQueue.front()).c_str(), (char*)"r");
		prm->filesQueue.pop();

		ReleaseMutex(hMutex);

		int firstFilePos = 0, secondFilePos = 0;
		bool firstEnd = false, secondEnd = false;
		int tempValFirst = 0, tempValSecond = 0;
		fseek(first, 0, SEEK_SET);
		firstFilePos = ftell(first);
		fseek(second, 0, SEEK_SET);
		secondFilePos = ftell(second);

		while (true)
		{
			if (!firstEnd)
			{
				firstFilePos = ftell(first);
				if (fscanf(first, "%d ", &tempValFirst) != 1)
				{
					firstEnd = true;
				}
			}

			if (!secondEnd)
			{
				secondFilePos = ftell(second);
				if (fscanf(second, "%d ", &tempValSecond) != 1)
				{
					secondEnd = true;
				}
			}

			if (firstEnd && secondEnd)
			{
				break;
			}

			if (!firstEnd && !secondEnd)
			{

				if (tempValFirst < tempValSecond)
				{
					fprintf(temp, "%d ", tempValFirst);
					fseek(second, secondFilePos, SEEK_SET);
				}
				else
					if (tempValFirst > tempValSecond)
					{
						fprintf(temp, "%d ", tempValSecond);
						fseek(first, firstFilePos, SEEK_SET);
					}
					else
						if (tempValFirst == tempValSecond)
						{
							fprintf(temp, "%d ", tempValFirst);
							fprintf(temp, "%d ", tempValSecond);
						}
			}
			else if (!firstEnd && secondEnd)
			{
				fprintf(temp, "%d ", tempValFirst);
			}
			else if (firstEnd && !secondEnd)
			{
				fprintf(temp, "%d ", tempValSecond);
			}
		}

		WaitForSingleObject(hMutex, INFINITE);
		prm->filesQueue.push(fileName);
		//printf("%s %d %s\n", "queue.size after file processing = ", prm->filesQueue.size(), fileName);
		fileNameId++;
		fclose(first);
		fclose(second);
		fclose(temp);
	}

	ReleaseMutex(hMutex);
	return 0;
}


int main()
{
	char input_file[] = "input.txt";

	FILE* in = openFile(input_file, (char*)"w");
	srand(time(NULL));
	for (int i = 0; i < num_ways * run_size; i++)
		fprintf(in, "%d ", rand());
	fclose(in);

	LARGE_INTEGER liFrequency, liStartTime, liFinishTime;
	QueryPerformanceFrequency(&liFrequency);
	QueryPerformanceCounter(&liStartTime);

	hMutex = CreateMutex(NULL, FALSE, "queueMutex");

	in = openFile(input_file, (char*)"r");

	char fileName[5];
	for (int i = 0; i < num_ways; i++)
	{
		snprintf(fileName, sizeof(fileName), "%d", i);
		out.push(fileName);
	}

	HANDLE* hThreadProd = new HANDLE[prodAmount];
	HANDLE* hThreadCons = new HANDLE[consAmount];

	int cnt = num_ways / prodAmount;
	int mod = num_ways % prodAmount;

	threadParam* param = new threadParam[prodAmount];
	for (int i = 0; i < prodAmount; i++)
	{
		param[i].amount = cnt;
		if (mod > 0)
		{
			mod--;
			param[i].amount++;
		}
		param[i].tArr = new int[run_size * param[i].amount];
		for (int j = 0; j < run_size * param[i].amount; j++)
		{
			if (fscanf(in, "%d ", &param[i].tArr[j]) != 1)
				break;
		}
		hThreadProd[i] = CreateThread(NULL, 0, producerFunc, (LPVOID)(param + i), 0, NULL);
	}

	fclose(in);

	WaitForMultipleObjects(prodAmount, hThreadProd, TRUE, INFINITE);
	for (int i = 0; i < prodAmount; i++)
		CloseHandle(hThreadProd[i]);

	for (int i = 0; i < consAmount; i++)
		hThreadCons[i] = CreateThread(NULL, 0, consumerFunc, (LPVOID)(&mergeParam), 0, NULL);
	WaitForMultipleObjects(consAmount, hThreadCons, TRUE, INFINITE);

	for (int i = 0; i < consAmount; i++)
		CloseHandle(hThreadCons[i]);

	CloseHandle(hMutex);
	QueryPerformanceCounter(&liFinishTime);
	double dElapsedTime = 1000.*(liFinishTime.QuadPart - liStartTime.QuadPart) / liFrequency.QuadPart;
	printf("Elapsed time = %f\n", dElapsedTime);

	getchar();
	return 0;
}

// 4, 4
// 11145.341360 - 1Mb
// 26660.851280 - 2Mb
// 61703.709815 - 4Mb

// 2, 2
// 13746.756385 - 1Mb
// 29628.292040 - 2Mb
// 66581.406597 - 4Mb






































































