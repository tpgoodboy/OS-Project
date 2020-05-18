#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <queue>
using namespace std;

#define P sem_wait
#define V sem_post
#define counter_num 5	//the number of counters

//the struct to save the customer infomation
typedef struct customer_info
{
	int at;		//the arriving time
	int no;		//the number the customer gets
	int cono;	//the number of the counter
	int ct;		//the time the customer gets service
	int st;		//the service time needed
	int lt;		//the time the customer leaves
}cus_info;			

//the global variables
int re_time;		//the reference time
int cus_num;		//the number of customers
int left_num;		//the number of left customers
int get_no;		//the number to be allocated
int call_no;		//the number to be called

queue<cus_info> all_cus;	//the queue to store all the information
queue<cus_info> wait_cus;	//the waiting queue
queue<cus_info> left_cus;	//the queue of left customers
//the semaphore
sem_t wait_lock;		//the lock of access to wait_cus queue, get_no and call_no
sem_t left_lock;		//the lock of access to left_cus queue and print onto screen
//the thread id
pthread_t cus_tid;				//the thread to get service number
pthread_t counter_tid[counter_num];		//the thread to call number
pthread_t clock_tid;				//the thread to compute time
int counter_no[counter_num];

void *get_time(void *argv){
	//the function to compute reference time
	sleep(1);
	while(1){
		re_time++;
		sleep(1);
	}
}

void *customer(void *argv){
	//the function to push cus_info into the queue
	while(1){
		if(all_cus.empty())
			break;
		else{
			int now = re_time;
			cus_info cus_a = all_cus.front();
			while(cus_a.at == now){
				all_cus.pop();
				cus_a.no = get_no;
				get_no++;
				P(&wait_lock);
				wait_cus.push(cus_a);
				printf("customer arrived at time %d, gets number %d and needs service time %d\n\n", cus_a.at, cus_a.no, cus_a.st);
				V(&wait_lock);
				cus_a = all_cus.front();
			}
		}
	}
}

void *counter(void *argv){
	//the function to show the counters' operation
	int counter_no = *(int *)argv;
	while(1){
		P(&wait_lock);
		if(!wait_cus.empty()){
			call_no++;
			cus_info cus_s = wait_cus.front();
			wait_cus.pop();
			V(&wait_lock);
			cus_s.cono = counter_no;
			cus_s.ct = re_time;
			cus_s.lt = cus_s.ct + cus_s.st;
			while(re_time < cus_s.lt);
			P(&left_lock);
			printf("Customer arrives at time %d, gets number %d,\n", cus_s.at, cus_s.no);
			printf("gets serviced at counter %d at time %d and leaves at time %d\n\n", cus_s.cono, cus_s.ct, cus_s.lt);
			left_cus.push(cus_s);
			left_num++;
			V(&left_lock);
		}
		else
			V(&wait_lock);	
	}
}

int main(){
	//initial the global varibles
	while(!all_cus.empty())
		all_cus.pop();
	while(!wait_cus.empty())
		wait_cus.pop();
	while(!left_cus.empty())
		left_cus.pop();
	sem_init(&wait_lock, 0, 1);
	sem_init(&left_lock, 0, 1);
	re_time = 0;
	cus_num = 0;
	left_num = 0;
	get_no = 1;
	call_no = 1;
	for(int i = 0; i < counter_num; i++)
		counter_no[i] = i+1;
	
	//read file and get the customers' infomation
	cus_info t = {0, 0, 0, 0, 0, 0};
	FILE *fp = fopen("customer_info.data","r");
	fscanf(fp, "%d%d%d", &cus_num, &t.at, &t.st);
	while(!feof(fp)){
		all_cus.push(t);
		fscanf(fp, "%d%d%d", &cus_num, &t.at, &t.st);
	}
	fclose(fp);
	
	//create threads of clock, customer and counters	
	pthread_create(&clock_tid, NULL, get_time, NULL);
	pthread_create(&cus_tid, NULL, customer, NULL);
	for(int i = 0; i < counter_num; i++)
		pthread_create(&counter_tid[i], NULL, counter, (void *)&counter_no[i]);
	
	//wait for the service finished
	while(left_num < cus_num);
	
	//wirte all the time information to the file
	fp = fopen("time_info.data", "w");
	fprintf(fp, "arrive  number  counter  start  service  finish\n");
	while(!left_cus.empty()){
		t = left_cus.front();
		fprintf(fp, "%d   %d   %d   %d   %d   %d\n", t.at, t.no, t.cono, t.ct, t.st, t.lt);
		left_cus.pop();
	}
	fclose(fp);
	
	return 0;
}

