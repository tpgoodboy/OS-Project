#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stack>
#include <time.h>
using namespace std;

#define P sem_wait	//P operation
#define V sem_post	//V operation
#define Thr_Num_Max 20	//the maximun of threads
#define Num_Max 1000000	//the maximun of numbers
#define SUP 1000		//the length of data

//global variable
FILE *fp;

typedef struct segnode
{
	int start;
	int end;
}dataseg;
stack<dataseg> seg_unsorted;	//to store the unsorted segement 

int data[Num_Max];	//the array to be sorted
int num_sorted = 0;	//the amount of sorted number
int thr_num = 0;	//the amount of threads
stack<pthread_t *> tid;	//the stack to store created pthread
clock_t beginall, endall;	//the time of beginning and ending

//the semaphore
sem_t num_lock;		//the lock to write num_sorted         sem_init(&,0,1)
sem_t thr_lock;		//the lock to write thr_num           sem_init(&,0,1)
sem_t print_lock;		//the lock to print on the screen     sem_init(&,0,1)

//the function for quicksort
typedef struct sort_variable
{
	int *data;
	int start;
	int end;
}sort_var;	//variable conveyed to quick sort

int partition(sort_var v)
{	
	//to find the partition of data
	//return the partition location
	int *a;
	a = v.data;
	int i = v.start;
	int j = v.end;
	int key = a[v.start];
	if(v.start < v.end)
	{
		
		while(i < j)
		{
			while(i < j && a[j] >= key)
				j--;
			if(i < j)
				a[i++] = a[j];
			while(i < j && a[i] < key)
				i++;
			if(i < j)
				a[j--] = a[i];
		}
		a[i] = key;
	}
	return i;
}

void quicksort(sort_var v)
{
	//to do quicksort
	//return the amount of sorted numbers
	int i = partition(v);
	if(v.start < v.end)
	{
		sort_var t1 = v;
		t1.end = i-1;
		quicksort(t1);
		sort_var t2 = v;
		t2.start = i+1;
		quicksort(t2);
	}
}

void *one_thr_qsort(void *v)
{
	sort_var t = *((sort_var *)v); 
	clock_t begin, end;
	begin = clock();
	quicksort(t);
	end = clock();
	double time = (double)(end-begin)/CLOCKS_PER_SEC;
	P(&num_lock);
	P(&thr_lock);
	P(&print_lock);
	num_sorted += t.end-t.start+1;
	thr_num--;
	printf("Finish sorting data from %d to %d, using time %f seconds.\n", t.start, t.end, time);
	V(&num_lock);
	V(&thr_lock);
	V(&print_lock);
	free(v);
}

int main()
{
	//generate random data
	srand((int)time(0));
	int rd[Num_Max];
	for(int i = 0; i < Num_Max; i++)
		rd[i] = rand();
	fp = fopen("rand_num.dat","w");
	for(int i = 0; i < Num_Max; i++)
		fprintf(fp, "%d ", rd[i]);
	fclose(fp);
	
	//read file and preparation work	
	fp = fopen("rand_num.dat","r");
	for(int i = 0; i < Num_Max; i++)
		fscanf(fp,"%d", &data[i]);
	fclose(fp);
	while(!tid.empty())
		tid.pop();
	while(!seg_unsorted.empty())
		seg_unsorted.pop();
	dataseg f;
	f.start = 0;
	f.end = Num_Max-1;
	seg_unsorted.push(f);
	sem_init(&num_lock, 0, 1);
	sem_init(&thr_lock, 0, 1);
	sem_init(&print_lock, 0, 1);
	beginall = clock();
	
	//multiple threads for quick sorting
	while(num_sorted < Num_Max)
	{
		dataseg t;
		if(!seg_unsorted.empty())
		{
			t = seg_unsorted.top();
			seg_unsorted.pop();	
			if(t.start < t.end)
			{
				sort_var *v = (sort_var *)malloc(sizeof(sort_var));
				v->data = data;
				v->start = t.start;
				v->end = t.end;
				if(t.end - t.start < SUP)
				{
					if(thr_num < Thr_Num_Max)
					{
						P(&thr_lock);					
						thr_num++;
						V(&thr_lock);
						pthread_t *pt = (pthread_t *)malloc(sizeof(pthread_t));
						pthread_create(pt, NULL, one_thr_qsort, (void *)v);
						tid.push(pt);
					}
					else
					{
						seg_unsorted.push(t);
						free(v);
					}
				}
				else
				{
					int i = partition(*v);
					dataseg t1, t2;
					t1.start = t.start;
					t1.end = i-1;
					t2.start = i+1;
					t2.end = t.end;
					seg_unsorted.push(t2);
					seg_unsorted.push(t1);
					free(v);
				}
			}	
			else	
			{
				P(&num_lock);
				num_sorted++;
				V(&num_lock);
			}
		}
		else
			break;
	}
	
	//ending of sorting
	int len;
	for(len = 0; !tid.empty(); len++)
	{
		pthread_t *tid_temp;
		tid_temp = tid.top();
		pthread_join(*tid_temp, NULL);
		tid.pop();
		free(tid_temp);
	}
	endall = clock();
	double time = (double)(endall-beginall)/CLOCKS_PER_SEC;
	printf("Created %d threads in all.\nTotal time for sorting: %f seconds.\n", len, time);
	
	//save sorted number
	fp = fopen("sorted_num.dat","w");
	for(int i = 0; i < Num_Max; i++)
		fprintf(fp,"%d\n", data[i]);
	fclose(fp);
	return 0;
}

