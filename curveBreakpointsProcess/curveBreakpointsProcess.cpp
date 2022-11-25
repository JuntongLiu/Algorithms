/*
 * Juntong Liu
 *            2022.10.25
 * 
 * curveBreakpointProcess.cpp    (C++ implementation)
 * 
 * This program analyses a curve file and deduce a maximum slope deviation that can be applied to "merge" those adjacent curve sections that
 * have smallest slope devication from each other, so that the total number of breakpoints describing the curve can be reduced to an acceptable level.
 * The purpose to do this is because that some curve files might have a large number of breakpoints, but the maximum number of breakpoints that a device can 
 * have is limited (for example, LakeShore240 can have 200 maximum).
 * So, if a curve, we have, has more than 200 breakpoints, neighbor sections with acceptable deviations need to be merged to reduce the number of
 * breakpoints.
 * 
 * This program requires C++17 or up to compile:
 *
 *  g++ -std=c++17 -Wall process_curve.cpp -o process_curve
 *  or: 
 * 	clang++ -std=c++17 -Wall process_curve.cpp -o process_curve
 *   
 * Run the program and following the prompts:
 * 
 *  ./process_curve
 * 
 */ 

#include <iostream>
#include <vector>
#include <iterator>
#include <filesystem>
#include <cstdio>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cmath>
//#include <cstring>
#include <fstream>
//#include <regex>   // for slice string. Not used anymore

#define DEVIATIONINC  0.007 // 0.0005            // deviation increase at each loop

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

/* We can use struct here, but, use class make thing simpler.
 * A function just use a constructor to create this class object and push it onto a list or vector make 
 * things much simple */
class BreakPoint
{
	public:
		double sensorUnit;
		double temp;

		BreakPoint(double r, double c):sensorUnit{r}, temp{c}
		{
		};
		~BreakPoint() = default;
		// we can have a function to set the data member. But for now, use constructor
		void setDat(double x, double y)
		{
			sensorUnit = x;
			temp = y;
		};
};

class ProcessCurve
{
    private:
	std::vector<std::string> curveHDR;
	std::vector<BreakPoint> origCurveBP;
	std::vector<BreakPoint> pickedCurveBP;
	std::string fileName;

    public:
	ProcessCurve() = default;
	~ProcessCurve() = default;
        int print_BP() const;
	int get_fileName();
	int parse_curve_file();     // parse the curve file and pickout header field and breakpoint and store them on curveHDR and vector.
	int process_curve_BP();     // do calculation to see if wee need to merge breakpoints to reduce number of the breakpoints.
       	int save_processed_BP();    // save processed breakpoints and header into a file.
};

// Pickout tokens from a string according to delimeters
void pickOutTokens(const std::string& str, 
		std::vector<std::string>& tokens, 
		const std::string& delims)
{
    std::size_t start = str.find_first_not_of(delims, 0), end = 0;
    while((end = str.find_first_of(delims, start)) != std::string::npos)
    {
        tokens.push_back(str.substr(start, end - start));
        start = str.find_first_not_of(delims, end);
    }
    if(start != std::string::npos)
        tokens.push_back(str.substr(start));
}

int ProcessCurve::print_BP() const
{
	int count{ 0 };
	//for(BreakPoint element : origCurveBP)
	for(auto element : origCurveBP)
	{
		std::cout << "original BP: the " << count << " element - r is: " << element.sensorUnit << std::endl;
		std::cout << "original BP: the " << count << " element - t is: " << element.temp << std::endl;
		count++;
	}

	std::cout << std::endl;
	count = 0;
		
	for(auto element : pickedCurveBP)
	{
		std::cout << "picked BP: the " << count << " element - sensorUnit " << element.sensorUnit << std::endl;
		std::cout << "picked BP: the " << count << " element - temperature " << element.temp << std::endl;
		count++;
	}

	return 0;
}


/* Parse curve file and pickout all the breakpoints to prepare for further process. */
int ProcessCurve::parse_curve_file()
{
	bool bppstarted = false;    // if breakpoint process has started or not 
	const std::string delim = " ";
	const std::string delim_comment = "#";
	std::size_t found_pos;

    	dprintf("ProcessCurv::process_curv_BP() is called.\n");

    	// open the file for read
    	std::ifstream inf{fileName.c_str()};
    	if(!inf)
    	{
    		std::cerr << "Unable to open file: " << fileName << std::endl;
    		return -1;
   	}

   	// read and process the curve file
   	while(inf)
   	{
		std::string strInput;
		std::getline(inf, strInput);

		/* If a line begin with #, it is a comment line, skip it.
		 * It is suggested that we should not covert a std::string into a raw char array, instead
		 * We convert it into a char vector: */
		std::vector<char> cVect(strInput.begin(), strInput.end());
#if 0
      		char *cp = &cVect[0];
      		//auto cp = std::begin(strInput);    // JT, this works also !
		dprintf("DEBUG:::=============>: Read and process the curve file...\n");
      		while(*cp == ' ' || *cp == '\t')
         		cp++;
      		if(*cp == '#')
         		continue;
#endif
      		// skip empty lines
    		if(strInput.length() == 0)
		{
        		continue;
		}

		// skip comment lines. It can be: "# xxx", " # xxx" or " #"
		int firstnws = strInput.find_first_not_of(delim, 0);          // skip front whitespace whitespace
		int pos1 = strInput.find_first_of(delim_comment, firstnws);
		if(pos1 == firstnws)
		{
			continue;
		}

		/* After we use string vector for curve header, we do not need to process header in this program, and just
		* push all header fields into the vector. */
		if(((found_pos = strInput.find(":")) != std::string::npos) && bppstarted == false)
		{
			curveHDR.push_back(strInput); 
			continue;
		}
		else if(((found_pos = strInput.find(":")) != std::string::npos) && bppstarted == true)
		{
			std::cerr << "Wrong curve format!" << std::endl;
			return -1;
		};
		dprintf("DEBUGG:: =======> read in string: %s\n", strInput.c_str());

		// Now, headers is done, we mark it to make sure that there should be no header field in middle of breakpoint section.
		if(!bppstarted)
			bppstarted = true;

		std::vector<std::string>tokens;

		// Some customer tool add sequence number in the BP section. So, we need to figure out if there is a sequence number
		// in the BP section. Pick out those tokens
		pickOutTokens(strInput, tokens, delim);

		if(tokens.size() == 3)
		{                       // 3 tokens means there is sequence number	
			double r = atof(tokens[1].c_str());
			double t = atof(tokens[2].c_str());
			dprintf("DEBUG:: double: r = %f\n", r);
			BreakPoint bp{r, t};
			origCurveBP.push_back(bp);
		}
		else if(tokens.size() == 2)
		{                       // 2 tokens, no sequence number
			double r = atof(tokens[0].c_str());
			double t = atof(tokens[1].c_str());
			BreakPoint bp{r, t};
			origCurveBP.push_back(bp); 					
		}
	}

	dprintf("DEBUGG:::===> Process curve file is done. Number of BPs is: %ld\n", origCurveBP.size());

	return 0;
}

int ProcessCurve::get_fileName()
{
	std::cout << "Enter the curve file name you want to process: " << std::endl;
	std::getline(std::cin >> std::ws, fileName);
	if(!std::filesystem::exists(fileName))
	{
		std::cout << "File: " << fileName << " dose not exist!" << std::endl;
		return -1;
	}
	else
		std::cout << "OK, file: " << fileName << " exist!" << std::endl;

   	// convert std string into C string:
   	printf("Covert fileName from STD string to C string: %s\n", fileName.c_str());
	
   	return 0;
}


int ProcessCurve::process_curve_BP()
{
	std::string line;
	class BreakPoint *start1BP{}, *end1BP{}, *start2BP{}, *end2BP{};    // TODO: use 3: startBP, middleBP, endBP
	//std::vector<BreakPoint>::iterator itr = origCurveBP.begin();
	float deviat = 0;               
   	double tang1=0, tang2=0, deltT=0;
   	int loops = 0;
	int origIndex = 0;
	std::string numBPs;
	size_t numOfBreakpoints;
	size_t numPickedBP = 0;

	int vsize = origCurveBP.size();
	if(vsize < 3)
	{
		std::cout << "Too few break points!" << std::endl;
		return -1;
	} 

try_again:

	std::cout << "How many break points you want to have/keep?" << std::endl;
	std::getline(std::cin >> std::ws, numBPs);
	numOfBreakpoints = atoi(numBPs.c_str());
	// we control the number fall in between 20 - 200;

	/* The number can not be reduced to less than the following:
     	 * NumberOfBPs / 2 + 1
	 *  If we have 31 BPs,   we got 31/2 + 2 = 17
	 */ 
	if(numOfBreakpoints < (origCurveBP.size() / 2 + 2))
	{
		std::cout << "Number of breakpoints should not less than " << origCurveBP.size() / 2 + 2 <<  std::endl;
		goto try_again;
	}

	if(numOfBreakpoints > origCurveBP.size())
	{
		std::cout << "Curve breakpoints is OK, no need to do further process!" << std::endl;
		return 1;
	}

	dprintf("DEBUG::: 1 =====> size of origCurveBP: %ld\n", origCurveBP.size());

calculate:

	origIndex = 0; 
    	loops++;
	dprintf("DEBUGG::: 2 =====> LOOP: %d; %s\n", loops, " +++++++++++++");

	// at least there are 3 breakpoints
	start1BP = &origCurveBP[origIndex];                            // [0] points to the first BP
	
	pickedCurveBP.push_back(origCurveBP[origIndex]);               // [0], the first BP should always be picked.
	origIndex++;
	end1BP = &origCurveBP[origIndex];                              // [1], next 
	dprintf("DEBUGG::::: 3 ======> end1BP->temp = %f\n", end1BP->temp); 
    	start2BP = end1BP;                                             // start breakpoint of second section
   
	origIndex++;
	end2BP = &origCurveBP[origIndex];                              // [2], we have a end breakpoint of second section if it is not the end of the curve table

	while(1)
	{
		dprintf("DEBUGG::: 4 start1BP = %f, end1BP = %f\n", start1BP->temp, end1BP->temp);
		dprintf("DEBUGG::: 5 start2BP = %f, end2BP = %f\n", start2BP->temp, end2BP->temp);
		// we calculate the tangent of the two sections
		tang1 = (end1BP->temp - start1BP->temp) / (end1BP->sensorUnit - start1BP->sensorUnit); 
		tang2 = (end2BP->temp - start2BP->temp) / (end2BP->sensorUnit - start2BP->sensorUnit);
		std::cout << "tang1: " << tang1 << std::endl; 

		printf("Tang2 = %f, tang1 = %f\n", tang2, tang1);
		printf("Deviation = %f\n", deviat);
		
		// calculate the delta and compare it with the allowed deviation. 4 cases:  
		if((tang1 > 0 && tang2 > 0) || (tang1 < 0 && tang2 < 0))  // both sections asending or decending
		{
			deltT = fabs(tang1 - tang2);
			dprintf("DEBUG::: 6 ====> deltT = %f, origIndex = %d\n", deltT, origIndex);
			if(deltT > deviat)
			{  // can not merge 
				pickedCurveBP.push_back(*start2BP);
				dprintf("DEBUG::: 7 ====> 1 picked temperature: %f\n", start2BP->temp); 
				start1BP = start2BP;          // no merge, just continue
				end1BP = end2BP;
				start2BP = end2BP;
				origIndex++;
				if(origIndex < vsize)
				{
					end2BP = &origCurveBP[origIndex];
					continue;
				}
				else {
					// reached end
					pickedCurveBP.push_back(*start2BP);
					break;
				}
			}
			else if(deltT <= deviat)
			{   // can merge this 2 sectons
				std::cout << "DEBUG::: ===> merged a section <===" << std::endl;
				start1BP = end2BP;
				pickedCurveBP.push_back(*start1BP);       // always pick the start1 BP, and need to advance fro end2BP
				origIndex++;                              // for end1BP = start2BP
				if(origIndex < vsize)
				{   // if there is more BP
					end1BP = &origCurveBP[origIndex];
					start2BP = end1BP;
					origIndex++;                          // for end2BP
					// more BP to process?
					if(origIndex < vsize)
					{         
						end2BP = &origCurveBP[origIndex]; // ...yes
						continue;
					}
					else{                
						pickedCurveBP.push_back(*start2BP);
						break;
					}
				}
				else {                                    // no, pick the end1BP = start2BP and break
					break;
				}
			}
		}
		else  // one section asending and another section desending 
		{
			deltT = fabs(tang1) + abs(tang2);
			std::cout << "DEBUG::: 8 ===> deltT = " << deltT << "  origIndex = " << origIndex << std::endl; 
			if(deltT > deviat)
			{   // can not merge this 2 sections
				pickedCurveBP.push_back(*start2BP);
				std::cout << "DEBUG::: 9 ====> 2 picked temperature: " << start2BP->temp << std::endl; 
				start1BP = start2BP;         // no merge, just continue
				end1BP = end2BP;
				start2BP = end2BP;
				origIndex++;
				if(origIndex < vsize)
				{
					end2BP = &origCurveBP[origIndex];
					continue;
				}
				else {
					// reached end
					pickedCurveBP.push_back(*start2BP);
					break;
				}
			}
			else{    // we can merge this 2 section
				start1BP = end2BP;
				pickedCurveBP.push_back(*start1BP);        // always pick the start1 BP, and need to advance fro end2BP
				origIndex++;                               // for end1BP = start2BP
				if(origIndex < vsize )
				{             // if there is more BP
					end1BP = &origCurveBP[origIndex];
					start2BP = end1BP;
					origIndex++;                          // for end2BP
					if(origIndex < vsize)
					{         // more BP to process?
						end2BP = &origCurveBP[origIndex]; // ...yes
						continue;
					}
					else{                
						pickedCurveBP.push_back(*start2BP);
						break;
					}
				}
				else {                                   // no, pick the end1BP = start2BP and break
					break;
				}
			}
		}

	}  // end of while(1)
   
	if(numPickedBP == pickedCurveBP.size())
		return 0;
	else
		numPickedBP = pickedCurveBP.size();
	/* now the curve is processed with an allowed deviation, we check to see how many breakpoints have been picked out. If it is still 
   	* too many than the user wanted, we increase the allowed deviation and jump to the outside loop and continue to process.
   	* if the number of picked out breakpoint is equal or smaller than the user wanted, then we stop loop and problem solved. */
	dprintf("DEBUG :::: pickedCurveBP size() = %ld\n", pickedCurveBP.size());
	
	if(pickedCurveBP.size() > numOfBreakpoints )
	{
    	deviat += DEVIATIONINC;     // If there are still too many breakpoints, we increase the allowed deviation gradually
      	dprintf("Change Deviation to ==>:: %f for next loop\n", deviat);

		// we clean the pickedCurveBP vector and prepare for another try.
		pickedCurveBP.clear();

		// The deviat set 0 at the begining, and it +DEVIATIONINC make it always > 0. Here is just a check, we can safely remove it.
        goto calculate;
   }
   else
   {
        printf("\n\n\n################################################################################################\n");
        printf("\n=======>>>: Total %d loops completed to get the results.  \n", loops);
		printf("Number breakpoints picked out is: %ld\n", pickedCurveBP.size());
        //printf("=======>>>: To reduce the breakpoints from %d to %d, the maximum deviation is %f <<<=========\n\n", ellCount(&breakPointList), numOfBreakpoints, deviat);
        printf("#################################################################################################\n\n");
   }
    

   return 0;
}


int main()
{
	ProcessCurve pc;
	pc.get_fileName();
	pc.parse_curve_file();
	pc.process_curve_BP();
	pc.print_BP();

	return 0;
}	
