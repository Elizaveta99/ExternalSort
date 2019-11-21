#define _CRT_SECURE_NO_WARNINGS
#define HAVE_STRUCT_TIMESPEC

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <queue>
#include <string>
#include <time.h>
#include <assert.h>
#include <thread>

using namespace std;

int prodAmount = 4;
int consAmount = 4;
int num_ways = 40;
const int run_size = 25000;
int fileNameId = num_ways;
queue<string> out;

pthread_mutex_t queueMutex;
pthread_cond_t queueCond;

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

void* producerFunc(void* param)
{
	threadParam* prm = (threadParam*)param;
	int numb = 0;
	for (int i = 0; i < prm->amount; i++)
	{
		
		vector<int> toSort;
		copy(&prm->tArr[run_size * i], &prm->tArr[run_size * i + run_size], back_inserter(toSort));
		sort(toSort.begin(), toSort.end());

		//WaitForSingleObject(hMutex, INFINITE);
		pthread_mutex_lock(&queueMutex);

		FILE* outFile = openFile((char*)out.front().c_str(), (char*)"w");

		for (int j = 0; j < run_size; j++)
			fprintf(outFile, "%d ", toSort[j]);

		mergeParam.filesQueue.push((char*)out.front().c_str());
		out.pop();
		fclose(outFile);

		//ReleaseSemaphore(hSemaphore, 1, NULL);
		//pthread_cond_broadcast(&queueCond);
		pthread_cond_signal(&queueCond);
		pthread_mutex_unlock(&queueMutex);
		//ReleaseMutex(hMutex);
	}
	return 0;
}

void* consumerFunc(void* param)
{
	threadMergeParam* prm = (threadMergeParam*)param;

	//WaitForSingleObject(hSemaphore, INFINITE);
	//WaitForSingleObject(hMutex, INFINITE);

	pthread_mutex_lock(&queueMutex);
	while (prm->filesQueue.size() <= 1)
		pthread_cond_wait(&queueCond, &queueMutex);

	while (prm->filesQueue.size() > 1)
	{
		char fileName[5];
		snprintf(fileName, sizeof(fileName), "%d", fileNameId);
		FILE* temp = openFile(fileName, (char*)"w");
		
		//printf("id = %d", std::this_thread::get_id());

		FILE* first = openFile((char*)(prm->filesQueue.front()).c_str(), (char*)"r");
		prm->filesQueue.pop();

		FILE* second = openFile((char*)(prm->filesQueue.front()).c_str(), (char*)"r");
		prm->filesQueue.pop();

		//ReleaseMutex(hMutex);
		//pthread_cond_broadcast(&queueCond); // ???????
		pthread_mutex_unlock(&queueMutex);

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

		//WaitForSingleObject(hMutex, INFINITE);
		pthread_mutex_lock(&queueMutex);
		prm->filesQueue.push(fileName);
		//ReleaseSemaphore(hSemaphore, 1, NULL);
		//pthread_cond_broadcast(&queueCond);      //??
		if (prm->filesQueue.size() > 1)           // ???
			pthread_cond_signal(&queueCond);
		//pthread_mutex_unlock(&queueMutex);  не надо - т.к. для проверки while оставить

		fclose(first);
		fclose(second);
		fclose(temp);
		fileNameId++;
	}

	//ReleaseMutex(hMutex);
	pthread_mutex_unlock(&queueMutex);
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

	clock_t start, end;
	start = clock();

	//time_t startTime = time(NULL); // получаем стартовое время 

	int result;
	result = pthread_mutex_init(&queueMutex, NULL);
	if (result != 0)
	{
		perror("\nMutex initialization failed");
		exit(EXIT_FAILURE);
	}
	result = pthread_cond_init(&queueCond, NULL);
	if (result != 0)
	{
		perror("\nCondition variable initialization failed");
		exit(EXIT_FAILURE);
	}

	in = openFile(input_file, (char*)"r");
	char fileName[5];
	for (int i = 0; i < num_ways; i++)
	{
		snprintf(fileName, sizeof(fileName), "%d", i);
		out.push(fileName);
	}

	pthread_t* producerThreads = new pthread_t[prodAmount];
	pthread_t* consumerThreads = new pthread_t[consAmount];

	pthread_attr_t attr; // атрибуты потока 

	pthread_attr_init(&attr); // инициализация атрибутов потока 
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); // установка атрибута в требуемое значение

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
		result = pthread_create(&producerThreads[i], &attr,  producerFunc, (void *)(&param[i])); 
		if (result != 0)
		{
			//perror("\nProducer thread creation failed");
			printf("\nProducer thread creation failed\n");
			getchar();
			exit(EXIT_FAILURE);
		}

		result = pthread_create(&consumerThreads[i], &attr, consumerFunc, (void *)(&mergeParam)); 
		if (result != 0)
		{
			//perror("\nConsumer thread creation failed");
			printf("\nConsumer thread creation failed\n");
			getchar();
			exit(EXIT_FAILURE);
		}
	}

	fclose(in);
	// освобождение ресурсов 
	pthread_attr_destroy(&attr); 

	//WaitForMultipleObjects(prodAmount, hThreadProd, TRUE, INFINITE);
	//for (int i = 0; i < prodAmount; i++)
		//CloseHandle(hThreadProd[i]);

	// apart - if different amounts of producers and consumers
	//for (int i = 0; i < consAmount; i++)
		//hThreadCons[i] = CreateThread(NULL, 0, consumerFunc, (LPVOID)(&mergeParam), 0, NULL);

	void * t_return;
	for (int i = 0; i < prodAmount; i++)
	{
		result = pthread_join(producerThreads[i], &t_return);
		if (result != 0)
		{
			//perror("\nProducer thread join failed");
			printf("\nProducer thread join failed\n");
			getchar();
			exit(EXIT_FAILURE);
		}
	}
	for (int i = 0; i < consAmount; i++)
	{
		result = pthread_join(consumerThreads[i], &t_return);
		if (result != 0)
		{
			//perror("\nConsumer thread join failed");
			printf("\nConsumer thread join failed\n");
			getchar();
			exit(EXIT_FAILURE);
		}
	}

	pthread_mutex_destroy(&queueMutex);
	pthread_cond_destroy(&queueCond);

	//time_t finishTime = time(NULL); // получаем финишное время 

	end = clock();
	double  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

	//printf("time = %f sec\n", difftime(finishTime, startTime)); 
	printf("time = %f sec\n", cpu_time_used);

	getchar();
	return 0;
}