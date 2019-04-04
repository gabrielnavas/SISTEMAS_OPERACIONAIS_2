#include<stdio.h>

void fcfs(int size)
{
	int i, j, result;
	int arrival[size];
	int burst[size];
	int waiting[size];
	
	/* initialize all values with 0 */
	for(i=0; i < size; ++i)	
		waiting[i]=0;	

	/* input the arrival time and burst time for each process */
	for(i=0; i < size; ++i) {
		printf("\nEnter arrival time for process %d : ", i+1);
		scanf("%d", &arrival[i]);

		printf("Enter burst time for process %d : ", i+1);
		scanf("%d", &burst[i]);
	}

	/* first come first serve */
	for(i=1; i<size; ++i) {
		result=0;
		
		for(j=0; j<i; ++j)	
			result+= burst[j];
		
		/* waiting time = starting time - arrival time */
		waiting[i] = result - arrival[i];	
	}

	/* print the waiting time */
	printf("\nWaiting Time:\t");
	for(i=0; i<size; ++i)
 		printf("%d\t", waiting[i]);

	/* average waiting time */
	for(i=0; i<size; ++i)	 		
		result+=waiting[i];
	
	result /= size;	
	printf("\nAverage Waiting Time:\t%d\n", result);	
}

int main()
{
	int size;
	
	
	printf("size: ");
	scanf("%d", &size);
	
	if(size > 0)
		fcfs(size);
}
