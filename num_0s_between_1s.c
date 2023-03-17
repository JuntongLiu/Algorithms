/*
 * Juntong Liu
 *            2023.02.22: 18:12
 *  
 * File name: num_0s_between_1s.c
 *
 * This program check a input integer and counts those continuous 0 bits between two 1 bits in the integer, 
 * save all lengthes in an array and print out the length of the longest number of 0 bits between two 1 bits.
 * For example, 
 * 
 * If the integer is 5678 = 1011000101110          The longest length of continuous 0 bits between two 1 bits is: 3
 *                              ^^^
 *
 * If the integer is 123  = 1111011                The longest length of continuous 0 bits between two 1 bits is: 1
 *                              ^
 * 
 * If the integer is 8765 = 10001000111101         The longest length of continuous 0 bits between two 1 bits is: 3
 *         
 * To compile and run the program:
 *       
 *  $ gcc -Wall num_0s_between_1s.c -o num_0s_between_1s
 *  $ ./num_0s_between_1s
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

static void print_binary(unsigned int integer)
{
	int i = CHAR_BIT * sizeof integer; /* bits in an integer */
	while (i--)
	{
		putchar('0' + ((integer >> i) & 1));
	}
}

static int mycompare( const void* a, const void* b)
{
   int int_a = * ( (int*) a );
   int int_b = * ( (int*) b );

   // an easy expression for comparing
   return (int_a > int_b) - (int_a < int_b);
}

/* find the length of all continuous 0 bits between two 1 bits and store those length in
 * an array. Return the longest length to the caller.
 */
static int num_z(int n)
{
#define INTEGER_SIZE CHAR_BIT * sizeof(int)
	int oneb = 1;
	int integer = n;
	int count = 0;
	int resultindex = 0;
	int checkedb = 0;
	int result[INTEGER_SIZE/2];
	
	while(checkedb < INTEGER_SIZE)
	{
		if((oneb & integer) == 0)
		{
			oneb <<= 1;
			checkedb++;
		}
		else
			break;
	}

	while(checkedb < INTEGER_SIZE) 
	{
		while(checkedb < INTEGER_SIZE)
		{
			if((oneb & integer) != 0)
			{
				checkedb++;
				oneb <<= 1;	
			}
			else
				break;
		}

		while(checkedb < INTEGER_SIZE)
		{
			if((oneb & integer) == 0)         
			{
				count++;
				checkedb++;
				oneb <<= 1;
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
		for(int i=0; i<resultindex; i++)
			printf("%d. has %d zero\n", i, result[i]);
		printf("The shortest length of continuous 0 bits between two 1 bits is: %d\n", result[0]);
		resultindex--;
		return (result[resultindex]);
	}
}

int main()
{
	char buff[8];
	int your_number;
	
	while(1)
	{
		
		printf("Type in a integer number, or type in q, Q to quit.\n");
		fgets(buff, sizeof(buff), stdin);
		if((buff[0] == 'q') || (buff[0] == 'Q'))
			return 0;

		your_number = atoi(buff);
		if(your_number == 0)
		{
			printf("Your input is 0 or no-digital! Try again!\n");
			continue;
		}
		
		printf("You have typed in: %d = ", your_number);
		print_binary((unsigned int)your_number);
		printf("\n");
		int result = num_z(your_number);

		printf("The max continuous 0 bits between two 1 bits = %d\n", result);
	}
	return 0;
}


