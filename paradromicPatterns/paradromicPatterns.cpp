/*
 * C++ implementation.
 *
 * paradromicPatterns.cpp
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
 * Following is an example input and result:
 * =========================================
 *
 * Type in a string with paradromic patterns like: abcba, aabbbbaa, dddd :
 *
 * aaa aabbbbaa abvba adfafa aa ccc a a afas
 *
 * Found 13 paradromic patterns:
 *
 *   ' a a '  ' aa '  'a a'  'a aabbbbaa a'  'a abvba a'  'a ccc a'  'aa'  'aa aa'  'aaa'  'afa'  'bbbb'  'ccc'  'faf'
 *
 * The longest pattern is: 'a aabbbbaa a' Size=12

 *
 *
 *
 *
 *                                                                        Juntong Liu
 *                                                                                          2019-11-01
 *
 *
 *  Note, comments to the codes will be added later.
 */

#include <iostream>
#include <map>
#include <set>
#include <iterator>
#include <string>
#include <cstring>

#define BUFF_SIZE 256

// Struct to record the longest paradromic pattern
struct longestPattern
{
    int size;
    char longPatt[BUFF_SIZE];
};


class ParadromicPatterns
{
public:
    ParadromicPatterns();
    ~ParadromicPatterns();

    void receiveData(void);                                     // Interface fuctions
    void processData(void);
    void displayData(void);

private:
                                                                
    int myStringCompare(const char * str1, const char * str2);
    int detectSameLetterPatterns(void);
    int detectEvenParadromicPatterns(void);
    int detectOddParadromicPatterns(void);

    std::string dataString;
    std::map<std::string, int> paradromicPatterns;
    struct longestPattern longestPattern;
                                                               
};

ParadromicPatterns::ParadromicPatterns()=default;
ParadromicPatterns::~ParadromicPatterns()=default;

int ParadromicPatterns::myStringCompare(const char * str1, const char * str2)
{
    while (*str1 != '\0' && (*str1 == *str2)) {
            str1++;
            str2++;
    }
    return (int) (*str1 - *str2);
}

int ParadromicPatterns::detectSameLetterPatterns()
{
    char *fwp, *bwp;
    int numberOfPattern = 0;
    int forwardCheck;
    int i, strlength = dataString.length();
    char *charStr = new char[dataString.length() + 1];

    // Convert to C string or use std::string::iterator, like:
//    std::string::iterator cppfwp, cppbwp;
//    cppfwp = cppbwp = dataString.begin();
//    cppbwp++;
    std::strcpy(charStr, dataString.c_str());
    fwp = &charStr[0];                                          // Set the pointers to point to letters
    bwp = &charStr[1];

    for(i=0; i < strlength-1; i++)                              // Walk through the string
    {
        if(*fwp == *bwp)
        {
            forwardCheck = 0;
            numberOfPattern++;

            while(bwp != &charStr[strlength-1])
            {
                if(*fwp == *bwp)
                {
                    forwardCheck++;
                    bwp++;
                }
                else
                    break;
            }
            bwp--;

            int length = bwp - fwp + 1;
            if(length > longestPattern.size)
            {
                strncpy(longestPattern.longPatt, fwp, length);
                longestPattern.size = length;
            }

            char tmp[BUFF_SIZE];

            strncpy(tmp, fwp, length);
            tmp[length] = '\0';
            std::string cctmp(tmp);

            if(paradromicPatterns.find(cctmp) == paradromicPatterns.end())
                paradromicPatterns.insert(std::pair<std::string,int>(cctmp, length) );

            if(forwardCheck != 0)
            {
                fwp += forwardCheck;
                bwp++;
                i += forwardCheck;
            }
        }
        fwp++;
        bwp++;
    }
    return numberOfPattern;
}

int ParadromicPatterns::detectEvenParadromicPatterns()
{
    char *fwp, *bwp;
    int numberOfPattern = 0;
    int expand;
    int i, strlength = dataString.length();
    char *charStr = new char[dataString.length() + 1];

    // Convert to C string or use std::string::iterator, like:
//    std::string::iterator cppfwp, cppbwp;
//    cppfwp = cppbwp = dataString.begin();
//    cppbwp++;
    std::strcpy(charStr, dataString.c_str());
    fwp = &charStr[0];                                                // Set the pointers to point to letters
    bwp = &charStr[1];

    for(i=0; i < strlength-1; i++)                                    // Walk through the string
    {
        if(*fwp == *bwp)
        {
            expand = 0;
            numberOfPattern++;

            while(fwp != &charStr[0] &&  bwp != &charStr[strlength-1])
            {
                fwp--; bwp++;
                if(*fwp == *bwp)
                {
                    expand++;
                    if(fwp == &charStr[0] || bwp == &charStr[strlength-1])
                        break;
                }
                else
                {
                    fwp++; bwp--;
                    break;
                }
            }

            if(expand > 0)
            {
                int length = bwp - fwp + 1;
                {
                    if(length > longestPattern.size)
                    {
                        strncpy(longestPattern.longPatt, fwp, length);
                        longestPattern.longPatt[length]='\0';
                        longestPattern.size = length;
                    }

                    char tmp[BUFF_SIZE];
                    strncpy(tmp, fwp, length);
                    tmp[length] = '\0';
                    std::string cppstr(tmp);

                    if(paradromicPatterns.find(cppstr) == paradromicPatterns.end())
                        paradromicPatterns.emplace(cppstr, length);
                }
            }
            if(expand > 0)
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

int ParadromicPatterns::detectOddParadromicPatterns()
{
    char *fwp, *bwp;
    int numberOfPattern = 0;
    int expand;
    int i, strlength = dataString.length();
    char *charStr = new char[dataString.length() + 1];

    // Convert to C string or use std::string::iterator, like:
//    std::string::iterator cppfwp, cppbwp;
//    cppfwp = cppbwp = dataString.begin();
    std::strcpy(charStr, dataString.c_str());
    fwp = bwp = &charStr[0];                                            // Set the pointers to point to letters

    for(i=0; i < strlength-1; i++)                                      // Walk through the string
    {
        expand = 0;
        while(fwp != &charStr[0] &&  bwp != &charStr[strlength-1])
        {
            fwp--; bwp++;
            if(*fwp == *bwp)
            {
                expand++;
                if(fwp == &charStr[0] || bwp == &charStr[strlength-1])
                    break;
            }
            else
            {
                fwp++; bwp--;
                break;
            }
        }

        if(expand > 0)
        {
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
                if(length > longestPattern.size)
                {
                    strncpy(longestPattern.longPatt, fwp, length);
                    longestPattern.size = length;
                }

                char tmp[BUFF_SIZE];
                strncpy(tmp, fwp, length);
                tmp[length] ='\0';
                std::string cppstr(tmp);

                if(paradromicPatterns.find(cppstr) == paradromicPatterns.end())
                    paradromicPatterns.emplace(cppstr, length);
            }
            fwp += expand*2;
            i += expand;
            numberOfPattern++;
        }
        fwp++; bwp++;
    }

    return numberOfPattern;
}

void ParadromicPatterns::receiveData(void)
{
    std::cout << "Type in a string with paradromic patterns like: abcba, aabbbbaa, dddd :" << "\n";
    getline (std::cin, dataString);
}

void ParadromicPatterns::processData(void)
{
    if(!dataString.empty())
    {
        detectSameLetterPatterns();
        detectEvenParadromicPatterns();
        detectOddParadromicPatterns();
    }
    else
        std::cout << "User data is empty, nothig to process! " << "\n";
}

void ParadromicPatterns::displayData(void)
{

    if(!paradromicPatterns.empty())
    {
        std::cout << "Found " << paradromicPatterns.size() << " paradromic patterns:" << "\n";
        std::cout << dataString << "\n";
        for(auto &kv : paradromicPatterns)
            std::cout << "'" << kv.first << "'" <<"  ";
        std::cout << "\n\n";
    }
    std::cout << "The longest pattern is: '" << longestPattern.longPatt << "' Size=" << longestPattern.size << "\n";
}


int main()
{
    ParadromicPatterns pp;

    pp.receiveData();
    pp.processData();
    pp.displayData();

    return 0;
}
