#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <windows.h>
#include <time.h>

using namespace std;

struct MinHeapNode
{
	int element;
	int i;
};

void swap(MinHeapNode* x, MinHeapNode* y);

// A class for Min Heap 
class MinHeap
{
	MinHeapNode* harr; // pointer to array of elements in heap 
	int heap_size;	 // size of min heap 

public:
	// Constructor: creates a min heap of given size 
	MinHeap(MinHeapNode a[], int size);

	// to heapify a subtree with root at given index 
	void MinHeapify(int);

	// to get index of left child of node at index i 
	int left(int i) { return (2 * i + 1); }

	// to get index of right child of node at index i 
	int right(int i) { return (2 * i + 2); }

	// to get the root 
	MinHeapNode getMin() { return harr[0]; }

	// to replace root with new node x and heapify() 
	// new root 
	void replaceMin(MinHeapNode x)
	{
		harr[0] = x;
		MinHeapify(0);
	}
};

MinHeap::MinHeap(MinHeapNode a[], int size)
{
	heap_size = size;
	harr = a; // store address of array 
	int i = (heap_size - 1) / 2;
	while (i >= 0)
	{
		MinHeapify(i);
		i--;
	}
}

void MinHeap::MinHeapify(int i)
{
	int l = left(i);
	int r = right(i);
	int smallest = i;
	if (l < heap_size && harr[l].element < harr[i].element)
		smallest = l;
	if (r < heap_size && harr[r].element < harr[smallest].element)
		smallest = r;
	if (smallest != i)
	{
		swap(&harr[i], &harr[smallest]);
		MinHeapify(smallest);
	}
}

void swap(MinHeapNode* x, MinHeapNode* y)
{
	MinHeapNode temp = *x;
	*x = *y;
	*y = temp;
}

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

void mergeFiles(char *output_file, int n, int k)
{
	FILE** in = new FILE*[k];
	for (int i = 0; i < k; i++)
	{
		char fileName[2];

		// convert i to string 
		snprintf(fileName, sizeof(fileName), "%d", i);

		// Open output files in read mode. 
		in[i] = openFile(fileName, (char*)"r");
	}

	// FINAL OUTPUT FILE 
	FILE *out = openFile(output_file, (char*)"w");

	// Create a min heap with k heap nodes. Every heap node has first element of scratch output file 
	MinHeapNode* harr = new MinHeapNode[k];
	int i;
	for (i = 0; i < k; i++)
	{
		// break if no output file is empty and index i will be number of input files 
		if (fscanf(in[i], "%d ", &harr[i].element) != 1)
			break;
		harr[i].i = i; // Index of scratch output file 
	}
	MinHeap hp(harr, i); // Create the heap 

	int count = 0;

	// Now one by one get the minimum element from min heap and replace it with next element. 
	// run till all filled input files reach EOF 
	while (count != i)
	{
		// Get the minimum element and store it in output file 
		MinHeapNode root = hp.getMin();
		fprintf(out, "%d ", root.element);


		// Find the next element that will replace current 
		// root of heap. The next element belongs to same 
		// input file as the current min element. 
		if (fscanf(in[root.i], "%d ", &root.element) != 1)
		{
			root.element = INT_MAX;
			count++;
		}

		// Replace root with next element of input file 
		hp.replaceMin(root);
	}

	// close input and output files 
	for (int i = 0; i < k; i++)
		fclose(in[i]);

	fclose(out);
}

int processAmount = 4;
int num_ways = 10;
const int run_size = 1000;

//HANDLE hSemaphore;
int next_output_file = 0;
CRITICAL_SECTION cs;
FILE** out;

struct threadParam
{
	int* tArr;
	int amount;
};

DWORD WINAPI threadFunc(LPVOID param)
{
	threadParam* prm = (threadParam*)param;
	for (int i = 0; i < prm->amount; i++)
	{
		vector<int> toSort;
		copy(&prm->tArr[run_size * i], &prm->tArr[run_size * i + run_size], back_inserter(toSort));

		//toSort.assign(prm->tArr, prm->tArr + run_size);
		sort(toSort.begin(), toSort.end());

		EnterCriticalSection(&cs); // ??
		for (int j = 0; j < run_size; j++)
			fprintf(out[next_output_file], "%d ", toSort[j]);
		next_output_file++;
		LeaveCriticalSection(&cs);
	}
	return 0;
}


int main()
{
	char input_file[] = "input.txt";
	char output_file[] = "output1.txt";

	FILE* in = openFile(input_file, (char*)"w");

	srand(time(NULL));
	for (int i = 0; i < num_ways * run_size; i++)
		fprintf(in, "%d ", rand());

	fclose(in);

	LARGE_INTEGER liFrequency, liStartTime, liFinishTime;
	QueryPerformanceFrequency(&liFrequency);
	QueryPerformanceCounter(&liStartTime);

	//externalSort(input_file, output_file, num_ways, run_size);	

	//createInitialRuns(input_file, run_size, num_ways);
	
	in = openFile(input_file, (char*)"r");
	//FILE**
	out = new FILE*[num_ways];
	char fileName[2];
	for (int i = 0; i < num_ways; i++)
	{
		// convert i to string 
		snprintf(fileName, sizeof(fileName), "%d", i);
		// Open output files in write mode. 
		out[i] = openFile(fileName, (char*)"w");
	}

	// allocate a dynamic array large enough to accommodate runs of size run_size 
	int* arr = (int*)malloc(run_size * sizeof(int));

	HANDLE* hThread = new HANDLE[processAmount];
	//DWORD* dwThread = new DWORD[processAmount];
	int cnt = num_ways / processAmount;
	int mod = num_ways % processAmount;

	InitializeCriticalSection(&cs);

	threadParam* param = new threadParam[processAmount];
	for (int i = 0; i < processAmount; i++)
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
			{
				break; // ??
			}
		}
		
		hThread[i] = CreateThread(NULL, 0, threadFunc, (LPVOID)(param + i), 0, NULL/*, &dwThread[i]*/);
	}

	// close input and output files 
	for (int i = 0; i < num_ways; i++)
		fclose(out[i]);

	fclose(in);

	//mergeFiles(output_file, run_size, num_ways);

	WaitForMultipleObjects(processAmount, hThread, TRUE, INFINITE);
	for (int i = 0; i < processAmount; i++)
		CloseHandle(hThread[i]);

	mergeFiles(output_file, run_size, num_ways);

	printf("%d %d %d %d\n", num_ways, cnt, mod, next_output_file);

	DeleteCriticalSection(&cs);

	QueryPerformanceCounter(&liFinishTime);
	double dElapsedTime = 1000.*(liFinishTime.QuadPart - liStartTime.QuadPart) / liFrequency.QuadPart;
	printf("Elapsed time = %f\n", dElapsedTime);

	getchar();
	return 0;
}

