/*
 * C implementation.
 *
 * paradromicPatterns.c
 *
 * This program searches and finds out paradromic patterns in a string, like:
 * 
 *    abccba, abcba, aaa, abba
 *
 * The program will pick out all paradromic patterns from a string and save it onto
 * a data structure.
 * Duplicated paradromic patterns will be picked out, and only one copy of them will
 * be saved.
 *
 * Note, pattern "aaabbbbbbaaa" contains 4 paradromic patterns, they are:
 *        aaa, bbbbbb, aaa, aaabbbbbbaaa
 *
 * Among the above patterns, one of the two "aaa" will be dropped, only one is saved.
 * The longest paradromic pattern will be detected.
 * All the saved paradromic patterns, as well as the longest one will be printed out.
 *
 * Following is an example input string and search result:
 * =======================================================
 *
 * Type in a string with paradromic patterns, like: abccba, abcba or aaa:
 *
 *    abcba aabbbbaa cccc dd
 *
 * Unique paradromic patterns contained in the string are:
 *
 *  "aa"   "bbbb"   "cccc"   "dd"   " aabbbbaa "   " cccc "   "abcba"   "a a"
 *
 * Longest Paradromic Pattern: " aabbbbaa ", size=10
 *
 *
 *
 *
 *                                                                        Juntong Liu
 *                                                                                          2019-10-30
 * 
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFF_SIZE 256
#define MAX_NUM_SAVE 20       // Number of patterns can be saved

// All paradromic patterns (store 20)
struct allPPatterns
{
    int atNow;
    char *pPatterns[MAX_NUM_SAVE];
};

// Struct to record the longest paradromic pattern
struct longestPattern
{
    int size;
    char longPatt[BUFF_SIZE];
};

// Copy n chars from src to dest and make it a string
static void my_strn_cpy(char * dest, const char * src, int n) {

        int i;
        for(i = 0; i<n; i++){
                *dest = *src;
                dest++; src++;
        }
        *dest++ = '\0';
}

// Compare two string, return 0 if same
static int my_str_cmp(const char * str1, const char * str2) {

        while (*str1 != '\0' && (*str1 == *str2)) {
                str1++;
                str2++;
        }
        return (int) (*str1 - *str2);

}

// Check the list to see if the pattern is alreay stored in the data structure
static int check_pattern_existance(struct allPPatterns *allPat, const char *pat)
{
    int i;
    if(allPat->atNow > 0)
    {
      for (i=0; i<allPat->atNow; i++)
          if(my_str_cmp(allPat->pPatterns[i], pat) == 0)
              return 1;
    }
    return 0;
}

// First pass will detect all the same letter paradromic patterns and save them
static int firstPass(char *str, struct longestPattern *longPat, struct allPPatterns *allPPat)
{
    char *fwp, *bwp;
    int numberOfPattern = 0;
    int forwardCheck;
    char tmp[256];

    int i, strlength = (int)strlen(str);
    fwp = &str[0];                                                // Set the pointers to point to letters
    bwp = &str[1];
    for(i=0; i < strlength-1; i++)                                // Walk through the string
    {
       if(*fwp == *bwp)
       {
           forwardCheck = 0;
           numberOfPattern++;
           while(*fwp == *++bwp)
               forwardCheck++;
           bwp--;

           int length = bwp - fwp + 1;
           if(length > longPat->size)
           {
               my_strn_cpy(longPat->longPatt, fwp, length);
               longPat->size = length;
           }

           my_strn_cpy(tmp, fwp, length);
           int exist = check_pattern_existance(allPPat, tmp);
           if(allPPat->atNow < MAX_NUM_SAVE && exist == 0)
           {
               if(!(allPPat->pPatterns[allPPat->atNow] = malloc(length+1)))
               {
                   perror("Malloc error....");
                   exit(0);
               }
               my_strn_cpy(allPPat->pPatterns[allPPat->atNow], fwp, length);
               allPPat->atNow++;
           }

           if(forwardCheck != 0)
           {
              fwp += forwardCheck + 1;
              bwp++;
              i += forwardCheck + 1;
           }
       }
       fwp++;
       bwp++;
    }
    return numberOfPattern;
}

// Second pass will detect all paradromic pattern with even and different letters and save them.
static int secondPass(char *str, struct longestPattern *longPat, struct allPPatterns *allPPat)
{
    char *fwp, *bwp;
    int numberOfPattern = 0;
    int expand;
    int i, strlength = (int)strlen(str);
    fwp = &str[0];                                                // Set the pointers to point to letters
    bwp = &str[1];
    for(i=0; i < strlength-1; i++)                                // Walk through the string
    {
       if(*fwp == *bwp)
       {
           expand = 0;
           numberOfPattern++;
           while(*--fwp == *++bwp)
               expand++;
           fwp++;
           bwp--;

           int notSame = 0, length = bwp - fwp + 1;
           char *tempp = fwp;
           while(bwp- ++tempp >= 0)
           {
               if(*tempp != *fwp)
               {
                   notSame = 1;
                   break;
               }
           }

           if(notSame == 1)
           {
              if(length > longPat->size)
              {
                  my_strn_cpy(longPat->longPatt, fwp, length);
                  longPat->size = length;
              }

              char tmp[BUFF_SIZE];
              my_strn_cpy(tmp, fwp, length);
              int exist = check_pattern_existance(allPPat, tmp);
              if(allPPat->atNow < MAX_NUM_SAVE && exist == 0)
              {
                  if(!(allPPat->pPatterns[allPPat->atNow] = malloc(length+1)))
                  {
                      perror("Malloc error....");
                      exit(0);
                  }
                  my_strn_cpy(allPPat->pPatterns[allPPat->atNow], tmp, length);
                  allPPat->atNow++;
              }
           }

           if(expand != 0)
           {
               fwp += expand*2 + 1;
               bwp++;
               i += expand + 1;
           }
       }
       fwp++; bwp++;
    }

    return numberOfPattern;
}

// Third pass will detect all paradromic pattern with different and odd number of letters and save them.
static int thirdPass(char *str, struct longestPattern *longPat, struct allPPatterns *allPPat)
{
    char *fwp, *bwp;
    int numberOfPattern = 0;
    int expand;
    int i, strlength = (int)strlen(str);
    fwp = bwp = &str[0];                                                // set the pointers to point to letters

    for(i=0; i < strlength-1; i++)                                // Walk through the string
    {
         expand = 0;
         numberOfPattern++;
         while(*--fwp == *++bwp)
             expand++;
         fwp++;
         bwp--;

         int notSame = 0, length = bwp - fwp + 1;
         char *tempp = fwp;
         while(bwp- ++tempp >= 0)
         {
            if(*tempp != *fwp)
            {
                notSame = 1;
                break;
            }
         }

         if(notSame == 1)
         {
            if(length > longPat->size)
            {
                my_strn_cpy(longPat->longPatt, fwp, length);
                longPat->size = length;
            }

            char tmp[BUFF_SIZE];
            my_strn_cpy(tmp, fwp, length);
            int exist = check_pattern_existance(allPPat, tmp);
            if(allPPat->atNow < MAX_NUM_SAVE && exist == 0)
            {
                if(!(allPPat->pPatterns[allPPat->atNow] = malloc(length+1)))
                {
                    perror("Malloc error....");
                    exit(0);
                }
                my_strn_cpy(allPPat->pPatterns[allPPat->atNow], tmp, length);
                allPPat->atNow++;
            }
         }

         if(expand != 0)
         {
             fwp += expand*2;
             i += expand;
         }
         fwp++; bwp++;
    }

    return numberOfPattern;
}

int main()
{
    char charStr[BUFF_SIZE];
    struct allPPatterns allPPatterns;
    struct longestPattern longestPattern;
    longestPattern.size = 0;
    int numberOfPatterns = 0;
    allPPatterns.atNow = 0;

    // Ask the user to type in a string with paradromic patterns.
    printf("\nType in a string with paradromic pattern, like: abccba, abcba or aaa:\n\n");
    if(!fgets(charStr, BUFF_SIZE , stdin))
    {
        perror("fgets error:");
        exit(0);
    }

    int numberOfSame = firstPass(charStr, &longestPattern, &allPPatterns);
    int numberOfPara = secondPass(charStr, &longestPattern, &allPPatterns);
    int onumberOfPara = thirdPass(charStr, &longestPattern, &allPPatterns);
    numberOfPatterns = numberOfSame + numberOfPara + onumberOfPara;

    if(numberOfPatterns == 0)
       printf("No paradromic pattern found in the string.\n");
    printf("%s\n", charStr);

    int j;
    if(allPPatterns.atNow > 0)
    {
        printf("Unique paradromic patterns contained in the string are:\n");
        for(j=0; j<allPPatterns.atNow; j++)
             printf("\"%s\"  ", allPPatterns.pPatterns[j]);
        printf("\n\n");

    }

    if(longestPattern.size > 0)
         printf("Longest Paradromic Pattern: \"%s\", size=%d\n", longestPattern.longPatt, longestPattern.size);
    printf("\n");

    if(allPPatterns.atNow > 0)
    {
       for(j=0; j<allPPatterns.atNow; j++)
           free(allPPatterns.pPatterns[j]);             // Free the heap memory
    }

    return 0;
}
