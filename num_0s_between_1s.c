/*
 * Juntong Liu
 *            2023.02.22: 18:12
 *  
 * File name: num_0s_between_1s.c
 *
 * This program counts those continuous 0 bits between two 1 bits in an integer 
 * and save their length in an array, and print out the length of the longest number of 0 bits between two 1 bits.
 * For example, 
 * 
 * If the integer is 5678 = 1011000101110          The length of longest continuous 0 bits between two 1 bits is: 3
 *                              ^^^
 *
 * If the integer is 123  = 1111011                The length of longest continuous 0 bits between two 1 bits is: 1
 *                              ^
 * 
 * If the integer is 8765 = 10001000111101         The length of longest continuous 0 bits between two 1 bits is: 3
 *         
 * 
 * my_compare() is a function for qsort() library function to sort a array in acending order.
 * int arr[6] = {9,2,5,7,8,1};
 * After sorted:
 * int arr[6] = {1,2,5,7,8,9};
 *
 * To compile and run the program:
 *       
 *  $ gcc -Wall num_0s_between_1s.c -o num_0s_between_1s
 *  $ ./num_0s_between_1s
 *
 */

#include <stdio.h>
#include <stdlib.h>

int mycompare( const void* a, const void* b)
{
   int int_a = * ( (int*) a );
   int int_b = * ( (int*) b );

   // an easy expression for comparing
   return (int_a > int_b) - (int_a < int_b);
}

int num_z(int n)
{
	int int_size = sizeof(int);
	int result[int_size];
	int oneb = 1;
	int integer = n;
	int count = 0;
	int resultindex = 0;
	int checkedb = 0;
	int_size *= 8;
	
	while(checkedb < int_size)
	{
		if((oneb & integer) == 0)
		{
			oneb <<= 1;
			checkedb++;
		}
		else
			break;
	}

	while(checkedb < int_size) 
	{
		while(checkedb < int_size)
		{
			if((oneb & integer) != 0)
			{
				checkedb++;
				oneb <<= 1;	
			}
			else
				break;
		}

		while(checkedb < int_size)
		{
			if((oneb & integer) == 0)         
			{
				count++;
				checkedb++;
				oneb <<= 1;
				printf("3. zbit; checkedb = %d\n", checkedb);
			}
			else{
				result[resultindex] = count;
				resultindex++;
				count = 0;
				break;
			}
		}
	}
  
	qsort(result, resultindex, sizeof(int), mycompare);
	
	if(resultindex == 0)
		return 0;            // no 0 bits between two 1s in the integer
	else{
		resultindex--;
		return (result[resultindex]);
	}
}

int main()
{
	int num = 8765;         // 1024; // 123; // 5678;
	int result = num_z(num);
	printf("The length of longest continuous 0s between two 1s is:  %d\n", result);
	return 0;
}
