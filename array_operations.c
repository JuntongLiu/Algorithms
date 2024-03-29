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
#include <assert.h>

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
    assert(len > 0);
    
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
    assert(len >= elements && len > 0 && elements >= 0);
    
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
    assert(len > 0);
   
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
    int array[] = {0,1,2,3,4,5,6,7,8,9};
    size_t len = sizeof(array) / sizeof(array[0]);
    int evenelem[len];
    int elen;

    printf("\nOriginal array elements:\n");
    for(int i=0; i<len; i++)
        printf("%d  ", array[i]);

    // reverse the array
    reverse_int_array(array, len);
    printf("\n\nArray after reversing: \n");
    for(int i=0; i<len; i++)
        printf("%d  ", array[i]);

    // sort the array
    qsort(array, len, sizeof(int), mycompare);
    printf("\n\nArray after sorted acendingly: \n");
    for(int i=0; i<len; i++)
        printf("%d  ", array[i]);

    // reverse and move array begining section
    reverse_and_move_section(array, len, 3);
    printf("\n\nArray has reversed-begining-section moved to tail: \n");
    for(int i=0; i<len; i++)
        printf("%d  ", array[i]);

    // find even elements in an integer array
    find_even_elements(array, len, evenelem, &elen);
    printf("\n\nArray has following even elements: \n");
    for(int i=0; i<elen; i++)
        printf("%d  ", evenelem[i]);
    printf("\n\n");
   
    return 0; 
}
