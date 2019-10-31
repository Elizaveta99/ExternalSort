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

// Number of partitions of input file. 
int num_ways = 40;
// The size of each partition 
int run_size = 25000;
int fileNameId = num_ways;

queue<string> filesQueue;
FILE** out;

FILE* openFile(char* fileName, char* mode)
{
	FILE* fp = fopen(fileName, mode);
	if (fp == NULL)
	{
		perror("Error while opening the file\n");
		exit(EXIT_FAILURE);
	}
	return fp;
}

// Using a merge-sort algorithm, create the initial runs and divide them evenly among the output files 
void createInitialRuns(char *input_file, int run_size,
	int num_ways)
{
	// For big input file 
	FILE *in = openFile(input_file, (char*)"r");

	// output scratch files 
	out = new FILE*[num_ways];
	char fileName[5];
	for (int i = 0; i < num_ways; i++)
	{
		// convert i to string 
		snprintf(fileName, sizeof(fileName), "%d", i);
		// Open output files in write mode. 
		out[i] = openFile(fileName, (char*)"w");
		filesQueue.push(fileName);
	}

	// allocate a dynamic array large enough 
	// to accommodate runs of size run_size 
	int* arr = (int*)malloc(run_size * sizeof(int));

	bool more_input = true;
	int next_output_file = 0;
	int i;
	while (more_input)
	{
		// write run_size elements into arr from input file 
		for (i = 0; i < run_size; i++)
		{
			if (fscanf(in, "%d ", &arr[i]) != 1)
			{
				more_input = false;
				break;
			}
		}

		// sort array 
		//mergeSort(arr, 0, i - 1);
		vector<int> toSort;
		toSort.assign(arr, arr + i);
		sort(toSort.begin(), toSort.end());

		// write the records to the appropriate scratch output file 
		// can't assume that the loop runs to run_size 
		// since the last run's length may be less than run_size 
		for (int j = 0; j < i; j++) 
		{
			fprintf(out[next_output_file], "%d ", toSort[j]);
		}

		next_output_file++;
	}

	for (int i = 0; i < num_ways; i++)
		fclose(out[i]);
	fclose(in);
}

// For sorting data stored on disk 
void externalSort(char* input_file, char *output_file,
	int num_ways, int run_size)
{
	// read the input file, create the initial runs, 
	// and assign the runs to the scratch output files 
	createInitialRuns(input_file, run_size, num_ways);

	// Merge the runs using the K-way merging 
	//mergeFiles(output_file, run_size, num_ways);
}


// Driver program to test above 
int main()
{
	char input_file[] = "input.txt";
	char output_file[] = "output.txt";
	FILE* in = openFile(input_file, (char*)"w");

	srand(time(NULL));
	// generate input 
	for (int i = 0; i < num_ways * run_size; i++)
		fprintf(in, "%d ", rand());

	fclose(in);

	LARGE_INTEGER liFrequency, liStartTime, liFinishTime;
	QueryPerformanceFrequency(&liFrequency);
	QueryPerformanceCounter(&liStartTime);

	externalSort(input_file, output_file, num_ways, run_size);

	while (filesQueue.size() > 1)
	{
		char fileName[5];
		snprintf(fileName, sizeof(fileName), "%d", fileNameId);
		FILE* temp = openFile(fileName, (char*)"w");

		FILE* first = openFile((char*)(filesQueue.front()).c_str(), (char*)"r");
		filesQueue.pop();

		FILE* second = openFile((char*)(filesQueue.front()).c_str(), (char*)"r");
		filesQueue.pop();

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

		fclose(temp);
		filesQueue.push(fileName);
		fileNameId++;
		fclose(first);
		fclose(second);
	}

	QueryPerformanceCounter(&liFinishTime);
	double dElapsedTime = 1000.*(liFinishTime.QuadPart - liStartTime.QuadPart) / liFrequency.QuadPart;
	printf("Elapsed time = %f\n", dElapsedTime);

	getchar();
	return 0;
}

// 14715.579093 - 1Mb
// 37205.160861- 2Mb
// 82317.135711- 4Mb


























//struct MinHeapNode
//{
//	// The element to be stored 
//	int element;
//
//	// index of the array from which the element is taken 
//	int i;
//};
//
//// Prototype of a utility function to swap two min heap nodes 
//void swap(MinHeapNode* x, MinHeapNode* y);
//
//// A class for Min Heap 
//class MinHeap
//{
//	MinHeapNode* harr; // pointer to array of elements in heap 
//	int heap_size;	 // size of min heap 
//
//public:
//	// Constructor: creates a min heap of given size 
//	MinHeap(MinHeapNode a[], int size);
//
//	// to heapify a subtree with root at given index 
//	void MinHeapify(int);
//
//	// to get index of left child of node at index i 
//	int left(int i) { return (2 * i + 1); }
//
//	// to get index of right child of node at index i 
//	int right(int i) { return (2 * i + 2); }
//
//	// to get the root 
//	MinHeapNode getMin() { return harr[0]; }
//
//	// to replace root with new node x and heapify() 
//	// new root 
//	void replaceMin(MinHeapNode x)
//	{
//		harr[0] = x;
//		MinHeapify(0);
//	}
//};
//
//// Constructor: Builds a heap from a given array a[] of given size 
//MinHeap::MinHeap(MinHeapNode a[], int size)
//{
//	heap_size = size;
//	harr = a; // store address of array 
//	int i = (heap_size - 1) / 2;
//	while (i >= 0)
//	{
//		MinHeapify(i);
//		i--;
//	}
//}
//
//// A recursive method to heapify a subtree with root at given index. This method assumes that the subtrees are already heapified 
//void MinHeap::MinHeapify(int i)
//{
//	int l = left(i);
//	int r = right(i);
//	int smallest = i;
//	if (l < heap_size && harr[l].element < harr[i].element)
//		smallest = l;
//	if (r < heap_size && harr[r].element < harr[smallest].element)
//		smallest = r;
//	if (smallest != i)
//	{
//		swap(&harr[i], &harr[smallest]);
//		MinHeapify(smallest);
//	}
//}
//
//// A utility function to swap two elements 
//void swap(MinHeapNode* x, MinHeapNode* y)
//{
//	MinHeapNode temp = *x;
//	*x = *y;
//	*y = temp;
//}


// Merges k sorted files. Names of files are assumed to be 0, 1, 2, ... k - 1 
//void mergeFiles(char *output_file, int n, int k)
//{
//	FILE** in = new FILE*[k];
//	for (int i = 0; i < k; i++)
//	{
//		char fileName[3];
//
//		// convert i to string 
//		snprintf(fileName, sizeof(fileName), "%d", i);
//
//		// Open output files in read mode. 
//		in[i] = openFile(fileName, (char* )"r");
//		//q.push(in[i]);
//	}
//
//	// FINAL OUTPUT FILE 
//	FILE *out = openFile(output_file, (char*)"w");
//
//	// Create a min heap with k heap nodes. Every heap node has first element of scratch output file 
//	MinHeapNode* harr = new MinHeapNode[k];
//	int i;
//	for (i = 0; i < k; i++)
//	{
//		// break if no output file is empty and index i will be number of input files 
//		if (fscanf(in[i], "%d ", &harr[i].element) != 1)
//			break;
//
//		harr[i].i = i; // Index of scratch output file 
//	}
//	MinHeap hp(harr, i); // Create the heap 
//
//	int count = 0;
//
//	// Now one by one get the minimum element from min heap and replace it with next element. run till all filled input files reach EOF 
//	while (count != i)
//	{
//		// Get the minimum element and store it in output file 
//		MinHeapNode root = hp.getMin();
//		fprintf(out, "%d ", root.element);
//
//
//		// Find the next element that will replace current root of heap. The next element belongs to same input file as the current min element. 
//		if (fscanf(in[root.i], "%d ", &root.element) != 1)
//		{
//			root.element = INT_MAX;
//			count++;
//		}
//
//		// Replace root with next element of input file 
//		hp.replaceMin(root);
//	}
//
//	// close input and output files 
//	for (int i = 0; i < k; i++)
//		fclose(in[i]);
//
//	fclose(out);
//}