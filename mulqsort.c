#include <stdio.h>
#include <stdlib.h>
#include <stack>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/shm.h>
using namespace std;

#define P sem_wait	//P operation
#define V sem_post	//V operation
#define Thr_Num_Max 20	//the maximun of threads
#define Num_Max 1000000	//the maximun of numbers
#define SUP 1000		//the length of data

//struct to store unsorted data segment
typedef struct segnode
{
	int start;
	int end;
}dataseg;		

//global variables and semaphore
int thr_num = 0;			//the amount of threads
int num_sorted = 0;	//the amount of sorted numbers
double sort_time = 0;		//the total time used
stack<pthread_t *> tid;	//the stack to store the created thread_id
sem_t num_lock;		//the lock to read and write num_sorted
sem_t thr_lock;			//the lock to read and write thr_num and create new thread
sem_t print_lock;		//the lock to print on the screen


//the function for quicksort
int partition(int *a, int start, int end)
{	
	//to find the partition of data
	//return the partition location
	int i = start;
	int j = end;
	int key = a[start];
	if(start < end)
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

void quicksort(int *a, int start, int end)
{
	//to do quicksort
	//return the amount of sorted numbers
	int i = partition(a, start, end);
	if(start < end)
	{
		quicksort(a, start, i-1);
		quicksort(a, i+1, end);
	}
}

void *mul_qsort(void *ds)
{	
	//get attached to the shared memory
	int shmid = shmget((key_t)1234, Num_Max*sizeof(int), IPC_CREAT | 0666);
	void *shm = shmat(shmid, NULL, 0);
	int *d = (int *)shm;
	dataseg f = *((dataseg *)ds);
	stack<dataseg> unsorted_ds;
	while(!unsorted_ds.empty())
		unsorted_ds.pop();
	unsorted_ds.push(f);
	P(&print_lock);
	printf("Creating new thread to process data from %d to %d.\n", f.start, f.end);
	V(&print_lock);
	
	while(!unsorted_ds.empty())
	{
			dataseg t = unsorted_ds.top();
			unsorted_ds.pop();
			if(t.start < t.end)
			{
				if(t.end - t.start < SUP)
				{
					clock_t begin = clock();
					quicksort(d, t.start, t.end);
					clock_t end = clock();
					double time = (double)(end - begin)/CLOCKS_PER_SEC;
					P(&num_lock);
					P(&print_lock);
					num_sorted += t.end - t.start + 1;
					printf("Quick sort data from %d to %d, using %f seconds.\n", t.start, t.end, time);
					sort_time += time;
					V(&print_lock);
					V(&num_lock);
				}
				else
				{
					int p = partition(d, t.start, t.end);
					if(p > t.start && p < t.end)
					{
						P(&num_lock);
						num_sorted++;
						V(&num_lock);
						P(&thr_lock);
						if(thr_num < Thr_Num_Max)
						{
							dataseg t0;
							t0.start = t.start;
							t0.end = p - 1;
							unsorted_ds.push(t0);
							dataseg *v = (dataseg *)malloc(sizeof(dataseg));
							v->start = p + 1;
							v->end = t.end;
							pthread_t *pt = (pthread_t *)malloc(sizeof(pthread_t));
							pthread_create(pt, NULL, mul_qsort, (void *)v);
							tid.push(pt);
							thr_num++;
							V(&thr_lock);
						}
						else
						{
							V(&thr_lock);
							dataseg t1, t2;
							t1.start = t.start;
							t1.end = p - 1;
							t2.start = p + 1;
							t2.end = t.end;
							unsorted_ds.push(t2);
							unsorted_ds.push(t1);
						}
					}
					else if(p == t.start)
					{
						P(&num_lock);
						num_sorted++;
						V(&num_lock);
						t.start++;
						unsorted_ds.push(t);
					}
					else
					{
						P(&num_lock);
						num_sorted++;
						V(&num_lock);
						t.end--;
						unsorted_ds.push(t);
					}
				}
			}
			else
			{
				P(&num_lock);
				num_sorted++;
				V(&num_lock);
			}
	}
}

int main()
{
	FILE *fp;
	//generate random data
	fp = fopen("rand_num.dat","w");
	int rd;
	srand((int)time(0));
	for(int i = 0; i < Num_Max; i++)
	{
		rd = rand();
		fprintf(fp, "%d ", rd);
	}
	fclose(fp);
	
	//read file and create shared memory
	int shmid = shmget((key_t)1234, Num_Max*sizeof(int), IPC_CREAT | 0666);
	if(shmid == -1)
	{
		printf("Shared Memory Got Error!\n");
		return -1;
	}
	void *shm = shmat(shmid, NULL, 0);
	if(shm == (void *)-1)
	{
		printf("Shared Memory Attached Error!\n");
		return -1;
	}
	int *d = (int *)shm;
	fp = fopen("rand_num.dat","r");
	for(int i = 0; i < Num_Max; i++)
		fscanf(fp, "%d", &d[i]);
	fclose(fp);
	printf("Shared Memory has been created!\n");
	
	//preparation
	while(!tid.empty())
		tid.pop();
	sem_init(&num_lock, 0, 1);
	sem_init(&thr_lock, 0, 1);
	sem_init(&print_lock, 0, 1);
	thr_num = 0;
	num_sorted = 0;
	dataseg *v = (dataseg *)malloc(sizeof(dataseg));
	v->start = 0;
	v->end = Num_Max - 1;
	
	//sorting begin
	clock_t begin_all = clock();
	P(&thr_lock);
	pthread_t *pt = (pthread_t *)malloc(sizeof(pthread_t));
	pthread_create(pt, NULL, mul_qsort, (void *)v);
	tid.push(pt);
	thr_num++;
	V(&thr_lock);
	
	//sorting end
	int len;
	for(len = 0; !tid.empty(); len++)
	{
		pthread_t *tid_temp;
		tid_temp = tid.top();
		pthread_join(*tid_temp, NULL);
		tid.pop();
		free(tid_temp);
	}
	if(num_sorted < Num_Max)
		printf("Quick Sorting Error!\n");
	clock_t end_all = clock();
	double time = (double)(end_all-begin_all)/CLOCKS_PER_SEC;
	printf("Sort %d numbers in all.\nCreate %d threads in all.\nTotal time for sorting: %f seconds.\nTotal time used %f seconds.\n", num_sorted, len, sort_time, time);
	
	//delete shared memory
	if((shmctl(shmid, IPC_RMID, NULL)) == -1)
		printf("Deleting shared memory error!\n");

	//save sorted number
	fp = fopen("sorted_num.dat","w");
	for(int i = 0; i < Num_Max; i++)
		fprintf(fp,"%d\n", d[i]);
	fclose(fp);
	return 0;
}

