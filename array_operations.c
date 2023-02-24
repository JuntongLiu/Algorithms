/* 
 * Juntong Liu
 *            2023.02.23   11:38
 *
 * File name: array_operations.c
 * 
 * This program contains functions to do some basic array operations:
 *   
 *   --- array sorting
 *   --- reverse integer array 
 *   --- reverse and move integer array section 
 *   --- find even elements in an integer array 
 *   
 *  To compile and run the program:
 * 
 *    $ gcc -Wall array_operation.c -o array_operations
 *    $ ./array_operations
 */

#include <stdio.h>
#include <stdlib.h>

int mycompare( const void* a, const void* b)
{
   int int_a = * ( (int*) a );
   int int_b = * ( (int*) b );

   // an easy expression for comparing
   return (int_a > int_b) - (int_a < int_b);     // asending order
   //return (int_a < int_b) - (int_a > int_b);   // desending order
}


// reverse an integer array
static void reverse_int_array(int *arr, int len)
{
    int tmp_arr[len];   
    int length = len - 1;
    
    for(int i=0, j=length; i<len; i++, j--)
        tmp_arr[i] = arr[j];
    
    for(int i=0; i<len; i++)
        arr[i] = tmp_arr[i];

}

// reverse the begining "elements" elements and move it to the tail of an integer array
static void reverse_and_move_section(int *arr, int len, int elements)
{
   
    if((elements > len) || (len < 0) || (elements < 0))
    {
        printf("Array length or array section length error!\n");
        return;
    }
    int tmp_array[len];
   
    // store the section to be moved in temp array
    for(int i=0, *sec1=arr; i<elements; i++, sec1++)
        tmp_array[i] = *sec1;

    // shift rest elements left to the begining
    for(int i=0, *sec1=arr, *sec2=&arr[elements]; i<(len-elements); i++, sec1++, sec2++)   
        *sec1 = *sec2;
    
    // put the first section reversely at the tail of the array
    for(int i=0, *sec2=&arr[len-1]; i<elements; i++, sec2--)
        *sec2 = tmp_array[i];
}

/* find even elements in an integer array and store them in an array */
static void find_even_elements(int *array, int len, int *evenelem, int *rlen)
{
    int rindex = 0;
    for(int i=0; i<len; i++)
    {
        if((array[i]%2) == 0)
        {
            evenelem[rindex] = array[i];
            rindex++;
        }
    }
    *rlen = rindex;
}

int main()
{
    int len = 10;
    int array[] = {0,1,2,3,4,5,6,7,8,9};
    int evenelem[len];
    int elen;

    for(int i=0; i<len; i++)
        printf("Original element %d is: %d\n", i, array[i]);

    // reverse the array
    reverse_int_array(array, len);
    for(int i=0; i<len; i++)
        printf("Array after reversing, element %d is: %d\n", i, array[i]);

    // sort the array
    qsort(array, len, sizeof(int), mycompare);
    for(int i=0; i<len; i++)
        printf("Array after sorted acendingly, element %d is: %d\n", i, array[i]);

    // reverse and move array begining section
    reverse_and_move_section(array, len, 3);
    for(int i=0; i<len; i++)
        printf("Array after moved the reversed begining section to the tail, element %d is: %d\n", i, array[i]);

    // find even elements in an integer array
    find_even_elements(array, len, evenelem, &elen);
    for(int i=0; i<elen; i++)
        printf("Array has even element: %d \n", evenelem[i]);

    return 0; 
}
