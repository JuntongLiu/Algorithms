/* 
 *   Juntong Liu
 *  
 *              2020.09.17
 * ============================
 *
 *  File name: curve_breakpoint_process.c
 *  
 *  This program analyses a curve file and deduce a maximum slope deviation that can be applied to "merge" those adjacent curve sections that
 *  have smallest slope devication from each other, so that the total number of breakpoints describing the curve can be reduced to an acceptable level.
 *  The purpose to do this is because that some curve files might have a large number of breakpoints, but the maximum number of breakpoints that a device can 
 *  have is limited.
 *  So, if a curve, we have, has too many breakpoints, neighbor sections with acceptable deviations need to be merged to reduce the number of
 *  breakpoints.
 * 
 *  The processed curve will be saved into a file if needed. 
 *
 *  To compile and use this program:
 *
 *     1.) Open a terminal window and compile the program with gcc:
 *         
 *            gcc -Wall process_curve.c -o process_curve
 *
 *     2.) Run the program and following the prompts (to supply a curve file to be analysed, the results will be saved into a file if needed):
 *           
 *            ./process_curve
 * 
 *     3.) This program works on .340 format calabration curve files.
 *
 *  Please Note, part of this program is from an input/output control testing program. The .340 format curve file should has 7 header fields.
 *  
 *  This program was originally developed for ESS, but not in use anymore.
 *  
 *  Turn on the DEBUG, this program will print out more information.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>

#define DEBUG
#undef DEBUG
#ifdef DEBUG
#define dprintf(format, args...) printf(format, ##args)
#define DBGPRINT(fmt, ...)                            \
    do                                                \
    {                                                 \
        fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                __LINE__, __func__, __VA_ARGS__);     \
    } while (0)
#else
#define dprintf(format, args...)
#define DBGPRINT(fmt, ...)
#endif

typedef struct ELLNODE
{
    struct ELLNODE *next;
    struct ELLNODE *previous;
} ELLNODE;

#define ELLNODE_INIT \
    {                \
        NULL, NULL   \
    }

typedef struct ELLLIST
{
    ELLNODE node;
    int count;
} ELLLIST;

#define ELLLIST_INIT    \
    {                   \
        ELLNODE_INIT, 0 \
    }

typedef void (*FREEFUNC)(void *);

#define ellInit(PLIST)                                      \
    {                                                       \
        (PLIST)->node.next = (PLIST)->node.previous = NULL; \
        (PLIST)->count = 0;                                 \
    }
#define ellCount(PLIST) ((PLIST)->count)
#define ellFirst(PLIST) ((PLIST)->node.next)
#define ellLast(PLIST) ((PLIST)->node.previous)
#define ellNext(PNODE) ((PNODE)->next)
#define ellPrevious(PNODE) ((PNODE)->previous)
#define ellFree(PLIST) ellFree2(PLIST, free)

/**--------------------------------------------------------------------------------------------------------------------------------
 * @brief  Add a node to the list
 * @note   
 * @param  *pList: 
 * @param  *pNode: 
 * @retval None
 */
void ellAdd(ELLLIST *pList, ELLNODE *pNode)
{
    pNode->next = NULL;
    pNode->previous = pList->node.previous;

    if (pList->count)
        pList->node.previous->next = pNode;
    else
        pList->node.next = pNode;

    pList->node.previous = pNode;
    pList->count++;

    return;
}

/**--------------------------------------------------------------------------------------------------------------------------------
 * @brief  Free the heap memory
 * @note   
 * @param  *pList: 
 * @param  freeFunc: 
 * @retval None
 */
void ellFree2(ELLLIST *pList, FREEFUNC freeFunc)
{
    ELLNODE *nnode = pList->node.next;
    ELLNODE *pnode;

    while (nnode != NULL)
    {
        pnode = nnode;
        nnode = nnode->next;
        freeFunc(pnode);
    }
    pList->node.next = NULL;
    pList->node.previous = NULL;
    pList->count = 0;
}

void freeNodes(void *pnode)
{
    free(pnode);
}

/**--------------------------------------------------------------------------------------------------------------------------------
 * @brief  Pick out tokens from a string separated by delimiters(, tabs, whitespace, .....), and skip the extra whitespaces and tabs
 * @note   String must end with a \n or \r. The string will be chopped into tokens.
 * @param  **token:    a pointer array pointer to all the tokens
 * @param  *cp:        the string contain variable number whitespaces and TABs separated tokens
 * @param  *delimiter: string of chars as delimiter used by the function to pick out tokens
 * @retval number of tokens picked out
 */
static int pickOutToken(char **token, char *cp, const char *delimiter)
{
    int i = 0;

    while (*cp == ' ' || *cp == '\t')
        cp++;
    while ((token[i] = strsep(&cp, delimiter)))
    {
        while (*cp == ' ' || *cp == '\t')
            cp++;
        if (*cp == '\0' || *cp == '\n' || *cp == '\r')
            break;
        i++;
    };
    return i + 1;
}

struct curve_header {
    char sensorModel[15];                  // This actually is the curve name in the Lakeshore 240 manual, Max length = 15
    char serialNumber[10];                 // serial number,
    unsigned short dataFormat;             // data format: 2 = V/K, 3 = Ω/K, 4 = log Ω/K
    double setPointLimit;                  // set point limit. It is curve temperature limimt, value in kelvin
    unsigned short temperatureCoefficient; // coefficient: 1 = negative, 2 = positive
    unsigned short numberBreakpoints;      // number of break points of the curve, max 200 the Model-240 can take
    char tempUnit;                         // K = kelvin, C = celsius, F = fahrenheit 
    //unsigned short input;                // The input module the curve will be uploaded to, 1-8
};

struct curve_header curveHeader;
ELLLIST breakPointList;
ELLNODE *pickedNodes[225];                 // pointer array for those breakpoints that been picked out after process
unsigned short numOfBreakpoints;
typedef struct breakPoint
{
    ELLNODE node;
    double sensorUnit;
    double temperature;
} breakPoint;

// enum HEAD{
//     MODEL, 
//     SERIAL, 
//     FORMAT, 
//     LIMIT, 
//     BRKPOINTS, 
//     COEFFICIENT, 
//     UNIT
// };
// Define header flag 
#define CRVHDR_MODEL        0x01     // Curve file header field: sensor model  
#define CRVHDR_NUMBER       0x02     // Curve file header field: sersor serial number
#define CRVHDR_FORMAT       0x04     // Curve file header field: data format
#define CRVHDR_LIMIT        0x08     // Curve file header field: setpoint limit
#define CRVHDR_COEFFICIENT  0x10     // Curve file header field: coefficient
#define CRVHDR_BREAKPOINTS  0x20     // Curve file header field: number of break points
#define CRVHDR_UNIT         0x40     // Curve file header field: 

//#define MAXNUMBRKPT  15   //200       // max number of breakpoints to pick out
#define DEVIATIONINC 0.0005            // deviation increase at each loop


/**--------------------------------------------------------------------------------------------------------------------------------
 * @brief  parse the curve file, build the curve header and pick out breakpoints to prepare
 *         to upload to the LakeShore240
 * @note   
 * @param fileName: curve file name
 * @retval 
 */
static int parse_curve(char *fileName)
{
    char *cp, *chp;
    char buff[512];
    int len;
    FILE *fp;
    char *tokens[8];
    int numToken;
    char *delimiter = " \t\n\r";
    bool inBPsection = false;
    bool isBP = true;
    unsigned int headFlag = 0;

    fp = fopen(fileName, "r+");
    if (!fp)
    {
        perror("File open error!\n");
        return -1;
    }

    while (fgets(buff, sizeof(buff), fp))
    {

        // If the first char is #, it is a comment line, and skip it
        cp = buff;
        // Skip the possibl whitespaces and TABs before # or newline
        while (*cp == '\t' || *cp == ' ')
            cp++;

        if (*cp == '#')
            continue;

        // skip the empty line
        if (strlen(cp) == 1 && !feof(fp) && *cp == '\n')
            continue;

        //DBGPRINT("fgets() read in: %s\n strlen() return: %d\n", buff, (int)strlen(buff));

        // Pick out the curve header
        if(headFlag < 127)    // B1111111 = 127
        {
            if ((cp = strstr(buff, "Number:")))                        // Serial Number:
            {
                char *chp = strchr(cp, ':');
                if (chp == NULL)
                {
                    printf("Curve header format error!\n");
                    return -1;
                }
                numToken = pickOutToken(tokens, cp, delimiter);
                if (numToken == 2)
                {
                    len = strlen(tokens[1]);
                    if (len > 10)
                        strncpy(curveHeader.serialNumber, tokens[1], 10);  // serial <= 10 chars
                    else
                        strncpy(curveHeader.serialNumber, tokens[1], len);
                    printf("Serial number: %s\n", tokens[1]);
                }
                else
                {
                    printf("Error! curve header error!\n");
                    return -1;
                }
                headFlag |= CRVHDR_NUMBER;
            }
            else if ((cp = strstr(buff, "Model:")))                    // Sensor Model
            {
                char *chp = strchr(cp, ':');
                if (chp == NULL)
                {
                    printf("Curve header format error!\n");
                    return -1;
                }
                numToken = pickOutToken(tokens, cp, delimiter);
                if (numToken == 2)
                {
                    len = strlen(tokens[1]);
                    if (len > 15)
                        strncpy(curveHeader.sensorModel, tokens[1], 15);   // model name <= 15 chars
                    else
                        strncpy(curveHeader.sensorModel, tokens[1], len);
                    printf("\nSensor Model: %s\n", tokens[1]);
                }
                else
                {
                    printf("Error! Curve header errro!\n");
                    return -1;
                }
                headFlag |= CRVHDR_MODEL;
            }
            else if ((cp = strstr(buff, "Limit:")))                  // Setpoint Limit
            {
                int setPointLimit;
                char *strp;
                char *chp = strchr(cp, ':');
                if (chp == NULL)
                {
                    printf("Curve header format error!\n");
                    return -1;
                }
                numToken = pickOutToken(tokens, cp, delimiter);
                if (numToken == 3)
                {
                    setPointLimit = strtod(tokens[1], &strp);
                    curveHeader.setPointLimit = setPointLimit;
                    printf("SetPoint Limit: %d\n", setPointLimit);
                }
                else
                {
                    printf("Error! Curve header error!");
                    return -1;
                }
                headFlag |= CRVHDR_LIMIT;
            }
            else if ((cp = strstr(buff, "Coefficient:")))        // coefficient
            {
                int coeff;
                char *strp;
                chp = strchr(cp, ':');
                if (chp == NULL)
                {
                    printf("Curve header format error!\n");
                    return -1;
                }
                numToken = pickOutToken(tokens, cp, delimiter);
                if (numToken == 3)
                {
                    coeff = strtod(tokens[1], &strp);
                    curveHeader.temperatureCoefficient = coeff;
                    printf("coefficient: %d\n", coeff);
                }
                else
                {
                    printf("Error! Curve header error!");
                    return -1;
                }
                headFlag |= CRVHDR_COEFFICIENT;
            }
            else if ((cp = strstr(buff, "Format:")))           // Data Format
            {
                int dataFmt;
                char *strp;
                chp = strchr(cp, ':');
                if (chp == NULL)
                {
                    printf("Curve header format error!\n");
                    return -1;
                }
                numToken = pickOutToken(tokens, cp, delimiter);
                if (numToken == 5)
                {
                    dataFmt = strtod(tokens[1], &strp);
                    curveHeader.dataFormat = dataFmt;
                    printf("Data format: %d\n", dataFmt);
                }
                else
                {
                    printf("Error! Curve header error!");
                    return -1;
                }
                headFlag |= CRVHDR_FORMAT;
            }
            else if ((cp = strstr(buff, "Unit:")))           // Temperature unit
            {
                char *chp = strchr(cp, ':');
                if (chp == NULL)
                {
                    printf("Curve header format error!\n");
                    return -1;
                }
                if (strstr(cp, "(K)"))
                    curveHeader.tempUnit = 'K';
                else if (strstr(cp, "(F)"))
                    curveHeader.tempUnit = 'F';
                else if (strstr(cp, "(C)"))
                    curveHeader.tempUnit = 'C';
                else if (strstr(cp, "(S)"))
                    curveHeader.tempUnit = 'S';
                else
                {
                    printf("Unknow temperature sensor unit!\n");
                    return -1;
                }
                printf("Temperature unit: %c\n", curveHeader.tempUnit);
                headFlag |= CRVHDR_UNIT;
            }
            else if ((cp = strstr(buff, "Breakpoints:")))    // Number of breakpoints
            {
                int numBRKPT;
                char *strp;
                chp = strchr(cp, ':');
                if (chp == NULL)
                {
                    printf("Curve header format error!\n");
                    return -1;
                }
                numToken = pickOutToken(tokens, cp, delimiter);
                if (numToken == 2)
                {
                    numBRKPT = strtod(tokens[1], &strp);
                    curveHeader.numberBreakpoints = numBRKPT;
                    printf("Number of breakpoints: %d\n", numBRKPT);
                }
                else
                {
                    printf("Error! Curve header error!");
                    return -1;
                };
                headFlag |= CRVHDR_BREAKPOINTS;
            }
        }
        else if(headFlag == 127)                                // Pick out the breakpoints
        {
            breakPoint *bpp;
            double sunit, temperat;
            char *strp;
            cp = buff;

            while(*cp)
            {
                //dprintf("%c", *cp);
                if(*cp != ' ' &&  *cp != '\t' && !isdigit(*cp) && *cp != '.' && *cp != '\n' && *cp != '\r')
                {   
                    isBP = false;
                    break;
                }
                else
                    cp++;
            }
            if(isBP == false && inBPsection == true)
            {
                printf("Breakpoint format error!!!!\n");
                return -1;
            }

            cp = buff;
            numToken = pickOutToken(tokens, cp, delimiter);
            if (numToken == 3)
            {
                sunit = strtod(tokens[1], &strp);
                temperat = strtod(tokens[2], &strp);
            }
            else if (numToken == 2)
            {
                sunit = strtod(tokens[0], &strp);
                temperat = strtod(tokens[1], &strp);
            }
            else
            {
                printf("Error! Curve break point error!");
                return -1;
            }

            //dprintf("BreakPoint sensor unit: \'%lf\'\n", sunit);
            //dprintf("BreakPoint Temperature: \'%lf\'\n", temperat);
            cp = malloc(sizeof(struct breakPoint));
            if (!cp)
            {
                printf("Error!! Cannot allocate memory!\n");
                return -1;
            }
            memset(cp, 0, sizeof(struct breakPoint));
            bpp = (struct breakPoint *)cp;
            bpp->sensorUnit = sunit;
            bpp->temperature = temperat;
            ellAdd(&breakPointList, &bpp->node);
            inBPsection = true;
        }
        if (feof(fp))
            break;
    }
    
    if(feof(fp))
    {
        printf("\nFile parsing done!\n");
        printf("Number of breakpoints in the curve is: %d\n", ellCount(&breakPointList));
        fclose(fp);
        return 0;
    }
    else if(ferror(fp))
    {
        perror("Error reading file!<--------------\n");
        fclose(fp);
        return -1;
    }    

    fclose(fp);
    return 0;
}


/**--------------------------------------------------------------------------------------------------------------------------------
 * @brief  After the curve file is parsed. The results are saved in a list and a structure to facilitate the following curve uploading procedure. 
 *         This function print out the parsed results from the in-memory list and a structure to see if it is OK.
 *         This function has been added an aditional check to make sure that the number of breakpoints should not exceed the maximum 200 and the 
 *         sensor units are in ascending order.
 * @note   
 * @retval  0 = OK, 
 *         -1 = Not OK, must stop curve uploading
 */

static int print_parse_result(ELLLIST *list)
{
    ELLNODE *node;
    struct breakPoint *bpp;
    struct breakPoint *bpp2;

    dprintf("\n\n=============== Curve Header: ===============\n");
    dprintf("Sensor Model: %s\n", curveHeader.sensorModel);
    dprintf("Serial Number; %s\n", curveHeader.serialNumber);
    dprintf("Data Format: %d\n", curveHeader.dataFormat);
    dprintf("SetPoint Limit: %f\n", curveHeader.setPointLimit);
    dprintf("Coefficient: %d\n", curveHeader.temperatureCoefficient);
    dprintf("Temperature Unit: %c\n", curveHeader.tempUnit);
    dprintf("Number of breakpoints: %d\n", curveHeader.numberBreakpoints);
    dprintf("===============================================\n\n");

    node = ellFirst(list);
    for (int i = 1; i < ellCount(list) + 1; i++)
    {
        bpp = (struct breakPoint *)node;
        bpp2 = (struct breakPoint *)node->next;
        dprintf("Breakpoint %i sensor unit: %f\n", i, bpp->sensorUnit);
        dprintf("Breakpoint %i temperature: %f\n\n", i, bpp->temperature);
        if(bpp2)
        {
            if(bpp2->sensorUnit < bpp->sensorUnit)
            {
                printf("\n############ ERROR in curve!!! Sensor units must be in increasing order ############\n\n");
                return -1;
            }
        }
        node = ellNext(node);
    }

    // Check to see if the number of breakpoints is right under 200 limit
    if (ellCount(list) > 200)
    {
        printf("Error, the number of breakpoints can not bigger than 200!\n");
        return -1;
    }

    return 0;
}

/**--------------------------------------------------------------------------------------------------------------------------------
 * @brief  Save the processed curve into a file.
 * @note   
 * @retval 
 */
 static int save_pickedBreakpoints(int pickedBP)
 {
    char fileName[256];
    FILE *fp;
    char instr[6];
    char lines[128];
    int len;
    
    // Those break points picked out can be saved into a file
    printf("\nDo you want to save the picked breakpoints into a file(Y/N)? ");
    if(!fgets(instr, 6, stdin))
    {
        perror("fgets error:");
        exit(0);
    }
    if (instr[0] != 'Y' && instr[0] != 'y' && instr[0] != 'N' && instr[0] != 'n')
    {
        printf("Wrong answer! The answer must be \"Y\", \"y\", \"N\" or \"n\" ! \n");
        return -1;
    }

    if (instr[0] == 'N' || instr[0] == 'n')
    {
        printf("\nThe processed curve has not been saved!\n\n");
        return 0;
    }

    // Give a file name to save the processed curve break points
    printf("Type in a file name to save the processed curve: ");
    if(!fgets(fileName, 256, stdin))
    {
        perror("fgets() error!!");
        return -1;
    }
    len = strlen(fileName);
    //fileName[len - 1] = '\0';
    fileName[len] = '\0';

    // open the file for write
    if(!(fp = fopen(fileName, "wa+")))
    {
        perror("Error open file!");
        return -1;
    }

    // Start to construct lines to write into the file to save the processed curve
    bzero(lines, sizeof(lines));
    snprintf(lines, 128, "%c%c",  '\n', '\n');
    if(fputs(lines, fp) == EOF)
    {
        perror("Error write to file!");
        fclose(fp);
        return -1;
    }

    // form and write a comment into the file
    snprintf(lines, 128, "%s", "# Curve with reduced number of breakpoints to fit into LakeShore240");
    if(fputs(lines, fp) == EOF)
    {
        perror("Error write to file!");
        fclose(fp);
        return -1;
    }
    
    // create 2 blank lines
    bzero(lines, sizeof(lines));
    snprintf(lines, 128, "%c%c",  '\n', '\n');
    if(fputs(lines, fp) == EOF)
    {
        perror("Error write to file!");
        fclose(fp);
        return -1;
    }

    // save Sensor Model: curve header field into file
    snprintf(lines, 128, "%s%c%s%c", "Sensor Model:", ' ', curveHeader.sensorModel, '\n');
    if(fputs(lines, fp) == EOF)
    {
        perror("Error write to file!");
        fclose(fp);
        return -1;
    }

    // save Serial Number: curve header field into file
    snprintf(lines, 128, "%s%c%s%c", "Serial Number:", ' ', curveHeader.serialNumber, '\n');
    if(fputs(lines, fp) == EOF)
    {
        perror("Error write to file!");
        fclose(fp);
        return -1;
    }

    // save Data Format: curve header field into file
    snprintf(lines, 128, "%s%c%d%c", "Data Format:", ' ', curveHeader.dataFormat, '\n');
    if(fputs(lines, fp) == EOF)
    {
        perror("Error write to file!");
        fclose(fp);
        return -1;
    }    

    // save Setpoint limit: header field into file
    snprintf(lines, 128, "%s%c%f%c", "Setpoint Limit:", ' ', curveHeader.setPointLimit, '\n');
    if(fputs(lines, fp) == EOF)
    {
        perror("Error write to file!");
        fclose(fp);
        return -1;
    }

    // save Temperature Coefficient header field into file
    snprintf(lines, 128, "%s%c%d%c", "Temperature Coefficient:", ' ', curveHeader.temperatureCoefficient, '\n');
    if(fputs(lines, fp) == EOF)
    {
        perror("Error write to file!");
        fclose(fp);
        return -1;
    }

    // save Number of Breakpoints: header field into file
    snprintf(lines, 128, "%s%c%d%c", "Number of Breakpoints:", ' ', curveHeader.numberBreakpoints, '\n');
    if(fputs(lines, fp) == EOF)
    {
        perror("Error write to file!");
        fclose(fp);
        return -1;
    }

    // save Temperature Unit: header field into file
    snprintf(lines, 128, "%s%c%c%c", "Temperature Unit:", ' ', curveHeader.tempUnit, '\n');
    if(fputs(lines, fp) == EOF)
    {
        perror("Error write to file!");
        fclose(fp);
        return -1;
    }

    // create 2 blank lines in the file
    bzero(lines, sizeof(lines));
    snprintf(lines, 128, "%c%c",  '\n', '\n');
    if(fputs(lines, fp) == EOF)
    {
        perror("Error write to file!");
        fclose(fp);
        return -1;
    }

    // save all the valid break points into the file
    for(int i = 0; i < pickedBP; i++)
    {
        snprintf(lines, 128, "%d%c%f%c%f%c", i+1, '\t', ((struct breakPoint *)pickedNodes[i])->sensorUnit, '\t', ((struct breakPoint *)pickedNodes[i])->temperature, '\n');
        if(fputs(lines, fp) == EOF)
        {
            perror("Error write to file!");
            fclose(fp);
            return -1;
        }
    } 

    printf("\n\n################################################################\n");
    printf("\nProcessed Curve has been saved in file: %s\n\n", fileName);
    printf("################################################################\n\n");

    fclose(fp);

    return 0;
 }

/**--------------------------------------------------------------------------------------------------------------------------------
 * @brief  There are curve files that contain many breakpoints/data-pairs. But Model-240 can only has 200 breakpoints.
 *         For this reason, this function will calculate and merge those neighbor sections with the smallest deviation, so that 
 *         the total number of breakpoints can be reduced to an acceptable level.
 *         
 * @note   
 * @retval None
 */
static int process_curve(void)
{
    /* these 4 pointers, point to 2 sections of the curve. If the 2 sections are conjunctional, end1BP = start2BP;
     * if the 2 sections are not conjunctional,  end1BP not equal to start2BP */
    struct breakPoint *start1BP, *end1BP, *start2BP, *end2BP;
    float deviat = 0;               
    double tang1, tang2, deltT;
    int index;            // index for the picked breakpoints
    int loops = 0;

loop:

    index = 1;
    loops++;

    bzero(pickedNodes, sizeof(pickedNodes));

    start1BP = (struct breakPoint *)ellFirst(&breakPointList);   // start breakpoint of first section, take first break point
    pickedNodes[0] = &start1BP->node;                            // first BP always ok
    end1BP = (struct breakPoint *)ellNext(&start1BP->node);      // end breakpoint of first section, 

    start2BP = end1BP;                                           // start breakpoint of second section
    if(start2BP != NULL)
        end2BP = (struct breakPoint *)ellNext(&start2BP->node);  // we have a end breakpoint of second section if it is not the end of the curve table
    else
        end2BP = NULL;

    dprintf("\n\n============================ Start of Loop %d ======================== \n", loops);
    dprintf("#########Start1BP->sensorUnit=%f, temperature=%f; Start2BP->sensorUnit=%f, temperature=%f############\n", 
            start1BP->sensorUnit, start1BP->temperature, start2BP->sensorUnit, start2BP->temperature);
    
    /* We loop until all the sections of the curve have be processed. At the end, if the number of breakpoints picked out
     * are still bigger than the number of breakpoints that the user wanted, we increase the deviation, and jump to the outer 
     * loop and continue to do the next round of process, .... and so on until we get the right number of breakpoints */
    while (1)
    {
        if(end2BP == NULL)   // this means that we reached the end of the curve
        {
            printf("end2BP = NULL <<<======= Tang2 = 0;\n");
            tang2 = 0;
            if(start2BP != NULL)
            {
                pickedNodes[index] = &start2BP->node;
                break;
            }
            else if(start2BP == NULL)
                break;
        }
        else if(end2BP != NULL)  // we still at the middle of the curve
        {
            // if the first section and the second section are both asending
            if(end1BP->temperature - start1BP->temperature >= 0 && end2BP->temperature - start2BP->temperature >= 0)
            {   
                dprintf("\n==> Both sections ascending.....\n");
                // we calculate the tangent of the two sections
                tang1 = (end1BP->temperature - start1BP->temperature) / (end1BP->sensorUnit - start1BP->sensorUnit); 
                tang2 = (end2BP->temperature - start2BP->temperature) / (end2BP->sensorUnit - start2BP->sensorUnit);

                DBGPRINT("start1BP->temp = %f, end1BP->tmp = %f, start1BP->sensorUnit = %f, end1BP->sensorUnit =%f\n", 
                           start1BP->temperature, end1BP->temperature, start1BP->sensorUnit, end1BP->sensorUnit);
                DBGPRINT("start2BP->temp = %f, end2BP->tmp = %f, start2BP->sensorUnit = %f, end2BP->sensorUnit =%f\n", 
                           start2BP->temperature, end2BP->temperature, start2BP->sensorUnit, end2BP->sensorUnit);           
            }
            else if(end1BP->temperature - start1BP->temperature < 0 && end2BP->temperature - start2BP->temperature < 0)
            {   // if the first and second sections are both desending
                dprintf("==> Both sections descending....\n");
                // we calculate the tangent of the 2 sections
                tang1 = (start1BP->temperature - end1BP->temperature) / (end1BP->sensorUnit - start1BP->sensorUnit);
                tang2 = (start2BP->temperature - end2BP->temperature) / (end2BP->sensorUnit - start2BP->sensorUnit);
            }
            else if(end1BP->temperature - start1BP->temperature >= 0 && end2BP->temperature - start2BP->temperature < 0)
            {   // firs section is acending, but second section is desending
                dprintf("==> Section 1 ascending, section 2 descending....\n");
                // we calculate the tangent of the 2 sections
                tang1 = (end1BP->temperature - start1BP->temperature) / (end1BP->sensorUnit - start1BP->sensorUnit);
                tang2 = (start2BP->temperature - end2BP->temperature) / (end2BP->sensorUnit - start2BP->sensorUnit);
                // we can pick out one breakpoint, because the 2 section are 
                pickedNodes[index] = &start2BP->node;
                index++;
                //pickedNodes[index] = &end2BP->node;
                //start1BP = end2BP;
                start1BP = start2BP;                                    // use the first breakpoint of second section as a start point
                // decide the end1BP
                end1BP = (struct breakPoint *)ellNext(&start1BP->node);
                if(end1BP)
                {
                    start2BP = end1BP;
                    end2BP = (struct breakPoint *)ellNext(&start2BP->node);
                }
                else
                    end2BP = NULL;
                
                continue;
            }
            else if(end1BP->temperature - start1BP->temperature < 0 && end2BP->temperature - start2BP->temperature >= 0)
            {   // first section is decending and second section is asending
                dprintf("==> Section 1 descending, section 2 ascending.....\n");
                // calculate the tangent
                tang1 = (start1BP->temperature - end1BP->temperature) / (end1BP->sensorUnit - start1BP->sensorUnit);
                tang2 = (start2BP->temperature - end2BP->temperature) / (end2BP->sensorUnit - start2BP->sensorUnit);
                // we can pick out a breakpoint since the 2 sections have opposite slope
                pickedNodes[index] = &start2BP->node;
                index++;
                //pickedNodes[index] = &end2BP->node;
                //start1BP = end2BP;
                start1BP = start2BP;
                end1BP = (struct breakPoint *)ellNext(&start1BP->node);
                if(end1BP)
                {
                    start2BP = end1BP;
                    end2BP = (struct breakPoint *)ellNext(&start2BP->node);
                }
                else
                    end2BP = NULL;
                
                continue;
            }
 
            dprintf("Tang2 = %f, tang1 = %f\n", tang2, tang1);
            dprintf("Deviation = %f\n", deviat);
            
            // calculate the the delta and compare it with the allowed deviation
            if(tang2 > tang1)
                deltT = tang2 - tang1;
            else 
                deltT = tang1 - tang2;

            dprintf("DEBUG: DeltT = %f ------------------\n", deltT);

            // if the delta is smaller than the allowed deviation, these to sections are smooth enough, we move on to next section
            if(deltT < deviat)  
            {
                DBGPRINT("==>> deltT < deviat, index = %d, this section smooth enough -->: continue to check next section!\n", index);   
                start2BP = end2BP;
                if(start2BP)
                    end2BP = (struct breakPoint *)ellNext(&start2BP->node);
            }
            else if(deltT >= deviat)   // else, we pick out the start point of section 2 and continue, because the 2 sections have opposite slope
            {
                printf("==>> deltT >= deviat, index = %d ------- tangent1 = %f ---tangent2 = %f -------\n", index, tang1, tang2);
                // should pick the start2BP, and also use it as a new start point
                pickedNodes[index] = &start2BP->node;
                index++;
                start1BP = start2BP;
                end1BP = (struct breakPoint *)ellNext(&start1BP->node);
                start2BP = end1BP;
                if(start2BP)
                    end2BP = (struct breakPoint *)ellNext(&start2BP->node);
                else
                    end2BP = NULL;
            }
        }
    }   // end of while(1)

    /* now the curve is processed with an allowed deviation, we check to see how many breakpoints have been picked out. If it is still 
     * too many than the user wanted, we increase the allowed deviation and jump to the outside loop and continue to process.
     * if the number of picked out breakpoint is equal or smaller than the user wanted, then we stop loop and problem solved. */
    if(index > numOfBreakpoints - 1)    // MAXNUMBRKPT)
    {
        deviat += DEVIATIONINC;     // If there are still too many breakpoints, we increase the allowed deviation gradually
        DBGPRINT("After this loop %d, %d breakpoints have been picked out\n", loops, index+1);
        printf("Change Deviation to ==>:: %f for next loop\n", deviat);

        if(deviat <= 0)
            goto done;
        else
            goto loop;
    }
    else
    {
        printf("\n\n\n################################################################################################\n");
        printf("\n=======>>>: Total %d loops completed to get the results.  \n", loops);
        printf("=======>>>: To reduce the breakpoints from %d to %d, the maximum deviation is %f <<<=========\n\n", ellCount(&breakPointList), numOfBreakpoints, deviat);
        printf("#################################################################################################\n\n");
    }
    
done:

    return index + 1;
}

/**--------------------------------------------------------------------------------------------------------------------------------
 * @brief  Main function
 * @note   
 * @retval 
 */
int main()
{
    char newCrvFileName[128];
    int len;
    unsigned short pickedBP;
    char instr[8];

    printf("\n#########################################################################################################\n\n");
    printf("This program will analyse the curve and deduce a maximum deviation that can be applied to \"merge\" some of\n");
    printf("the adjacent curve sections, so that the total number of breakpoints descriping the curve can be reduced to\n");
    printf("an LakeShore-240 acceptable level.\n\n");
    printf("#########################################################################################################\n\n");

    printf("\nWhich curve to process (give curve file name. Default location is current working directory)?: ");
    if (!fgets(newCrvFileName, 128, stdin))
    {
        perror("fgets error:");
        exit(0);
    }

    len = strlen(newCrvFileName);
    newCrvFileName[len - 1] = '\0';
    //newCrvFileName[len] = '\0';
    DBGPRINT("File to upload: \'%s\' \n", newCrvFileName);
    //stat(fileName, &curveStat);
    if (access(newCrvFileName, R_OK) != 0) // check to see if the file is exist
    {
        printf("Error! File does not exist!\n");
        perror("Error to access file\n");
        return -1;
    }

    // initial the list
    ellInit(&breakPointList);
    //ellInit(&slimList);

    // parse the curve file and pick out the curve file header information and breakpoints
    if (parse_curve(newCrvFileName) < 0)
    {
        printf("Error parse curve file!\n");
        return -1;
    }

    // ask the user, how many breakpoints he/she want the program to pick out
    printf("\nHow many breakpoints to pickout(<= 200)? ");
    if (!fgets(instr, 8, stdin))
    {
        perror("fgets error:");
        exit(0);
    }
    numOfBreakpoints = atoi(instr);
    if(numOfBreakpoints < 1 || numOfBreakpoints > 200)   // make sure that the number of breakpoint is in valid range
    {
    //   printf("ERROR, invalid input number. It must be 1 - 200\n");
    //   return -1;
        // if the user give an invalid value, we set it as 200, since 200 is the number LakeShore-200 can have
        numOfBreakpoints = 200;
    }

    // Make sure that the number of breakpoints > number of breakpoint the user want to pick out
    if (ellCount(&breakPointList) <= numOfBreakpoints)      
    {
        printf("\nNo need to process this curve! Total Number of breakpoints(%d) < Number of breakpoints to pickout!\n\n", numOfBreakpoints); // MAXNUMBRKPT);
        return -1;
    } 

    // print out the parse result, and do something checking    
    if (print_parse_result(&breakPointList) < 0)
    {
        printf("Invalid number of breakpoints! Curve uploading quited!\n");
        return -1;
    }

    //DBGPRINT("@@@@@@@@@@@@@@@@ Pick %d breakpoints\n", numOfBreakpoints);

    // process the curve breakpoints, and pick out the valid breakpoints
    if((pickedBP = process_curve()) < 0)
    {
        printf("Error picking breakpoints!\n");
        return -1;
    }

    for(int j = 1; j < pickedBP + 1; j++)
    {
        struct breakPoint *pickedBP = (struct breakPoint *)pickedNodes[j-1];
        if(pickedBP)
            printf("Picked breakpoint %d: sensorUnit = %f, temperature = %f\n", j,  pickedBP->sensorUnit, pickedBP->temperature);
    }

    save_pickedBreakpoints(pickedBP);

    // free the heap memory
    if(breakPointList.count > 0)
        ellFree2(&breakPointList, freeNodes);
    
    //if(slimList.count > 0)
    //    ellFree2(&slimList, freeNodes);

    return 0;

}
