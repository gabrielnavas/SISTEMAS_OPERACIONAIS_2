#include<stdio.h>

int main()
{
	int i,j,sum=0,n;
	int d[20];
	int disk;   //loc of head
	int temp, max;     
	int dloc;   //loc of disk in array
	
	printf("Enter number of location: ");
	scanf("%d", &n);
	
	printf("\nEnter position of head: ");
	scanf("%d", &disk);
	
	printf("\nEnter elements of disk queue:\n");	
	for(i=0 ; i<n ; i++)
	{
		printf("\nElement n %d: ", i+1);
		scanf("%d", &d[i]);
	}
		
	d[n] = disk;
	n = n+1;
	
	 // sorting disk locations
	for(i=0 ; i<n ; i++)   
		for(j=i ; j<n ; j++)
			if(d[i] > d[j])
			{
				temp = d[i];
				d[i] = d[j];
				d[j] = temp;
			}
	
	max = d[n];
	
	// to find loc of disc in array
	for(i=0 ; i<n ; i++)   
		if(disk == d[i]) 
		{
			dloc = i; 	
			i = n; //condition break for
		}	
			  
		
	for(i=dloc;i>=0;i--)
		printf("%d -->",d[i]);
		
	printf("0 -->");
	for(i=dloc+1 ; i<n ; i++)
		printf("%d-->", d[i]);
	
	sum = disk + max;
	printf("\n\nmovement of total %d", sum);
}
