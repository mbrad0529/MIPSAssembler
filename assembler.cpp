/**
* @file   assembler.cpp
* @author Matthew Bradley, (C) 2016
* @date   06/05/2016
* @brief  Simple MIPS assembler
*
* @section DESCRIPTION
* Implements an assembler for a subset of the MIPS assembly language.
* Two passes are used in order to enable label functionality
*
* Note: Instructions stored in a std::map<size_t, std::bitset<32>> where size_t is the line num and bitset is the binary instruction
* Labels stored in a std::map<std::string, size_t>, size_t is the line number they POINT to, std::string is the label name without the colon
* 
* For Future refactoring, look at doing function object implementation with containers as private data members?
* TODO:
* 
**********************************************************************/
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <bitset>
#include <map>
#include <boost/tokenizer.hpp>

// Calls ParseLabels, Add Data, and Display
void ParseInstructions(const std::string&); 

// Parses labels into a map with key as the label name without : and second as the line number pointed to by label
void ParseLabels(std::vector<std::string>&, std::map<std::string, size_t>&); 

// Adds the .data section after the instruction set has been parsed according to rules for handling .word and .space
void AddData(std::map<size_t, std::bitset<32>>&, std::map<std::string, size_t>&, const std::vector<std::string>&);

// Calls ParseHex on the 32 bit bitset of each command and prints results to std::cout
void Display(const std::map<size_t, std::bitset<32>>&);

// Parses 32 bit binary strings by separating into 4 bit substrings and parsing each substring, appending result
std::string ParseHex(const std::bitset<32>);

// Accepts register name as a string (ex $zero, $s1, $v0) and returns 5 bit binary string corresponding to its number
std::string GetReg(const std::string); 

// Uses label map and current Line Number (incremented by 1 in function body to serve as $pc which always points to next instruction) to calculate branch distance 
// returns result as 16 bit binary string
std::string GetBranch(size_t, const std::string,
	const std::map<std::string, size_t>&); 

// Returns 26 bit binary string with calculated address for Jumps 
std::string GetAddress(const std::string,
	const std::map<std::string, size_t>&); 

// Returns 16 bit binary string of offset value for lw and sw, accepts either labels or integers (passed as std::string) as arguments
std::string GetOffset(const std::string, const std::map<std::string, size_t>&);

// Returns true if string is a label, meaning it has a colon or dot, or is in the list of labels
bool isLabel(const std::string, const std::map<std::string, size_t>&); 

int main(int argc, char *argv[])
{
	std::ifstream ifs;

	if (argc != 2)
	{
		std::cout << "Error: No input file specified!\n";
		exit(1);
	}
	
	ifs.open(argv[1]);
	if (ifs.fail())
	{
		std::cout << "Error opening file: " << argv[1] << '\n';
		return 0;
	}

	std::string in(argv[1]);
	
	ParseInstructions(in);

	return 0;
}

void ParseInstructions(const std::string& in)
{
	boost::char_separator<char> delim(", ()");
	typedef boost::tokenizer< boost::char_separator< char > > tokenizer;

	size_t lineNum = 0;
	std::string bin;
	std::string rs;
	std::string rd;
	std::string rt;
	std::string imm;
	std::ifstream ifs(in);

	std::vector<std::string> sourceCode;
	std::map<size_t, std::bitset<32>> instructions;

	/* reads data from file, skipping if it is a label */
	do
	{
		std::string line;
		std::ws(ifs);
		std::getline(ifs, line);
		sourceCode.push_back(line);
	} while (ifs.eof() == 0);

	std::vector<size_t> lineNums;
	std::map<std::string, size_t> labels;

	ParseLabels(sourceCode, labels);

	for (size_t i = 0; i < sourceCode.size(); ++i)
	{
		std::string s = sourceCode[i];
		
		if (!isLabel(s, labels))
		{
			tokenizer tokens(s, delim);
			tokenizer::iterator iter = tokens.begin();

			if (iter != tokens.end())
			{

				if (*iter == "syscall") // syscall
				{
					std::bitset<32> b(std::string("1100"));
					instructions.emplace(lineNum, b);
					++lineNum;
				}
				else if (s[0] == 'j') // jump
				{
			        bin = "000010";

					++iter;
					bin += GetAddress(*iter, labels);

					std::bitset<32> b(bin);
					instructions.emplace(lineNum, b);
					++lineNum;
				}
				else if (*iter == "addiu") // addiu
				{
					
					bin = "001001";

					rt = *(++iter);
					rs = *(++iter);
					imm = *(++iter); 

					bin += GetReg(rs);
					bin += GetReg(rt);

					std::bitset<16> j(std::stoi(imm));
					
					bin += j.to_string();

					std::bitset<32> b(bin);
					instructions.emplace(lineNum, b);
					++lineNum;
				} // end addiu
				else if (*iter == "addu") // addu
				{
					bin = "000000";

					rd = *(++iter);
					rs = *(++iter);
					rt = *(++iter);

					bin += GetReg(rs);
					bin += GetReg(rt);
					bin += GetReg(rd);

					bin += "00000";
					bin += "100001";

					std::bitset<32> b(bin);
					instructions.emplace(lineNum, b);
					++lineNum;
				}
				else if (*iter == "and") // and
				{
					bin = "000000";

					rd = *(++iter);
					rs = *(++iter);
					rt = *(++iter);

					bin += GetReg(rs);
					bin += GetReg(rt);
					bin += GetReg(rd);

					bin += "00000";
					bin += "100100";

					std::bitset<32> b(bin);
					instructions.emplace(lineNum, b);
					++lineNum;
				}
				else if (*iter == "beq")
				{
					bin = "000100";

					rs = *(++iter);
					rt = *(++iter);
					imm = *(++iter);

					bin += GetReg(rs);
					bin += GetReg(rt);
					bin += GetBranch(lineNum, imm, labels);

					std::bitset<32> b(bin);
					instructions.emplace(lineNum, b);
					++lineNum;
				} // end beq
				else if (*iter == "bne")
				{
					bin = "000101";

					rs = *(++iter);
					rt = *(++iter);
					imm = *(++iter);

					bin += GetReg(rs);
					bin += GetReg(rt);
					bin += GetBranch(lineNum, imm, labels);

					std::bitset<32> b(bin);
					instructions.emplace(lineNum, b);
					++lineNum;
				} // end bne
				else if (*iter == "div") // div
				{
					bin = "000000";

					rs = *(++iter);
					rt = *(++iter);

					bin += GetReg(rs);
					bin += GetReg(rt);
					bin += "0000000000";
					bin += "011010";

					std::bitset<32> b(bin);
					instructions.emplace(lineNum, b);
					++lineNum;
				} // end div
				else if (*iter == "lw") // lw
				{
					bin = "100011";

					rt = *(++iter);
					imm = *(++iter);
					rs = *(++iter);

					bin += GetReg(rs);
					bin += GetReg(rt);
					bin += GetOffset(imm, labels);

					std::bitset<32> b(bin);
					instructions.emplace(lineNum, b);
					++lineNum;
				} // end lw
				else if (*iter == "mfhi") // mfhi
				{
					bin = "0000000000000000";

					rd = *(++iter);

					bin += GetReg(rd);
					bin += "00000010000";

					std::bitset<32> b(bin);
					instructions.emplace(lineNum, b);
					++lineNum;
				} // end mfhi
				else if (*iter == "mflo") // mflo
				{
					bin = "0000000000000000";

					rd = *(++iter);

					bin += GetReg(rd);
					bin += "00000010010";

					std::bitset<32> b(bin);
					instructions.emplace(lineNum, b);
					++lineNum;
				} // end mflo
				else if (*iter == "mult") // mult
				{
					bin = "000000";

					rs = *(++iter);
					rt = *(++iter);

					bin += GetReg(rs);
					bin += GetReg(rt);
					bin += "0000000000011000";

					std::bitset<32> b(bin);
					instructions.emplace(lineNum, b);
					++lineNum;
				} // end mult
				else if (*iter == "or") // or
				{
					bin = "000000";

					rd = *(++iter);
					rs = *(++iter);
					rt = *(++iter);

					bin += GetReg(rs);
					bin += GetReg(rt);
					bin += GetReg(rd);
					bin += "00000100101";

					std::bitset<32> b(bin);
					instructions.emplace(lineNum, b);
					++lineNum;
				} // end or
				else if (*iter == "slt") // slt
				{
					bin = "000000";

					rd = *(++iter);
					rs = *(++iter);
					rt = *(++iter);

					bin += GetReg(rs);
					bin += GetReg(rt);
					bin += GetReg(rd);
					bin += "00000101010";

					std::bitset<32> b(bin);
					instructions.emplace(lineNum, b);
					++lineNum;
				} // end slt
				else if (*iter == "subu") // subu
				{
					bin = "000000";

					rd = *(++iter);
					rs = *(++iter);
					rt = *(++iter);

					bin += GetReg(rs);
					bin += GetReg(rt);
					bin += GetReg(rd);
					bin += "00000100011";

					std::bitset<32> b(bin);
					instructions.emplace(lineNum, b);
					++lineNum;
				} // end subu
				else if (*iter == "sw") //sw
				{
					bin = "101011";

					rt = *(++iter);
					imm = *(++iter);
					rs = *(++iter);

					bin += GetReg(rs);
					bin += GetReg(rt);
					bin += GetOffset(imm, labels);

					std::bitset<32> b(bin);
					instructions.emplace(lineNum, b);
					++lineNum;
				} // end sw
				else
				{
					std::cout << "Error! Bad instruction on line " << lineNum << '\n';
					++lineNum;
				}
			} // end if
		} //end if
	} // end
	
	AddData(instructions, labels, sourceCode);

	Display(instructions);

	ifs.close();
} // end ParseInstructions

void ParseLabels(std::vector<std::string>& src, std::map<std::string, size_t>& labels)
{
	size_t numLabels = 0;
	size_t dataLn = 0; // line number that .data is located on
	
	typedef boost::tokenizer < boost::char_separator< char > > tokenizer;
	boost::char_separator<char> delim(": ,");

	bool DATA = false;
	size_t distData = 0; // offset from Data or $gp

	for (size_t i = 0; i < src.size(); ++i)
	{
		std::string s = src[i];
		
		if (isLabel(s, labels))
		{
			tokenizer token(s, delim);
			tokenizer::iterator iter = token.begin();

			if (*iter == ".text")
			{
				
			}
			else if (DATA)
			{
				if (iter != token.end())
				{
					labels.emplace(*iter, (dataLn + distData));
					++iter;

					if (*iter == ".word")
					{
						++iter;

						for (; iter != token.end(); ++iter)
						{
							distData += 4;
						}
					}
					else if (*iter == ".space")
					{
						++iter;
						int stop = std::stoi(*iter);

						for (int i = 0; i < stop; ++i)
						{
							distData += 4;
						}
					}
				}
			}
			else
			{
				if (*iter == ".data")
				{
					DATA = true;
					dataLn = ((i - numLabels) - 1);
				}
				labels.emplace(*iter, ((i - numLabels) - 1)); // the - 1 accounts for counting from line 0, ensures the correct line is pointed to 
				++numLabels;
			}
		} // end if
	} // end for
} // end ParseLabels      

// checks if string contains a colon or dot, then it is obviously a label, if that fails checks whether it is in the list of labels
bool isLabel(const std::string s, const std::map<std::string, size_t>& labels)
{
	bool label = false;

	size_t loc = s.find(":");
	
	if (loc != std::string::npos)
	{
		label = true;
	}

	loc = s.find(".");

	if (loc != std::string::npos)
	{
		label = true;
	}

	if (!label)
	{
		std::map<std::string, size_t>::const_iterator iter = labels.find(s);

		if (iter != labels.end())
		{
			label = true;
		}
	}
	
	return label;
} // end isLabelResults

void AddData(std::map<size_t, std::bitset<32>>& instructions, std::map<std::string, size_t>& labels
	, const std::vector<std::string>& src)
{
	 typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
	 boost::char_separator<char> delim(": ,");

	 std::map<std::string, size_t>::iterator it = labels.find(".data");
	 size_t dataLoc = it->second; // i = is where .data is stored
	 bool DATA = false; // gets set to true when .data is read in, needed to avoid dereferencing bad token
	
	 size_t lineNum = instructions.size() + 1;
	 
	for (size_t i = 0; i < src.size(); ++i)
	{
		tokenizer token(src[i], delim);
		tokenizer::iterator iter = token.begin();

		while (iter != token.end())
		{
			if (*iter == ".data")
			{
				DATA = true;
			}
			else if (*iter == ".text")
			{

			}
			else if (!isLabel(*iter, labels))
			{
				if (*iter == ".word")
				{
					++iter;
	
					for (; iter != token.end(); ++iter)
					{
						std::bitset<32> b(std::stoi(*iter));
						instructions.emplace(lineNum, b);
						++lineNum;
					}
				}
				else if (*iter == ".space")
				{
					int numBytes = std::stoi(*(++iter));
					for (int i = 0; i < numBytes; ++i)
					{
						std::bitset<32> b;
						instructions.emplace(lineNum, b);
						++lineNum;
					}
				}
				else if (DATA)
				{
					std::bitset<32> b(std::stoi(*iter));
					instructions.emplace(lineNum, b);
					++lineNum;
				}
			} // end else if;
			else if (isLabel(*iter, labels))
			{	
				std::string tmp = *iter;
				tmp.pop_back(); // get rid of colon so it can be found
				std::map<std::string, size_t>::iterator loc = labels.find(tmp);
				
				std::map<std::string, size_t>::const_iterator ic = labels.find(*iter);

				if (ic->second >= dataLoc) // checks whether the label is in the .data segment, dangerous if .data comes before .text
				{
					++iter;
					if (*iter == ".word")
					{
						++iter;
					
						if (iter != token.end())
						{
							std::bitset<32> b(std::stoi(*iter));
							instructions.emplace(lineNum, b);
							++lineNum;
						}
					}
					else if (*iter == ".space")
					{
						int numBytes = std::stoi(*(++iter));

						for (int i = 0; i < numBytes; ++i)
						{
							std::bitset<32> b;
							instructions.emplace(lineNum, b);	
							++lineNum;
						}
					}
				}
			}
			++iter;
		} // end while
	} // end for
} //end AddData

// 26 bit address for Jumps
std::string GetAddress(const std::string s, const std::map<std::string, size_t>& map)
{
	size_t loc = map.find(s)->second;
	std::bitset<26> b(loc);

	return b.to_string();
} // end GetAddress

// 16 bit branch for Branches
std::string GetBranch(size_t $pc, const std::string s, const std::map<std::string, size_t>& labels)
{
	++$pc; // since $pc always points at the next instruction, we need to increment by one
	int loc = (int)labels.find(s)->second;
	loc = loc - $pc;
	std::bitset<16> b(loc);

	return b.to_string();
} // end GetBranch

std::string GetReg(const std::string s)
{
	if (s[1] == 'z')
	{
		std::bitset<5> b(0);
		return b.to_string();
	}
	else if (s[1] == 'a')
	{
		int i = std::stoi(std::string(1, s[2]));
		if (s[2] == 't')
		{
			std::bitset<5> b(1);
			return b.to_string();
		}

		else if (i < 4)
		{
			std::bitset<5> b(i + 4);
			return b.to_string();
		}
	}
	else if (s[1] == 't')
	{
		int i = std::stoi(std::string(1, s[2]));
		if (i < 8)
		{
			std::bitset<5> b(i + 8);
			return b.to_string();
		}
		else if (i == 8 || i == 9)
		{
			std::bitset<5> b(i + 16);
			return b.to_string();
		}
	}
	else if (s[1] == 'v')
	{
		int i = std::stoi(std::string(1, s[2]));
		std::bitset<5> b(i + 2);
		return b.to_string();
	}
	else if (s[1] == 's')
	{
		int i = std::stoi(std::string(1, s[2]));

		if (s[2] == 'p')
		{
			std::bitset<5> b(29);
			return b.to_string();
		}
		else if (i < 8)
		{
			std::bitset<5> b(i + 16);	
			return b.to_string();
		}
	}
	else if (s[1] == 'k')
	{
		int i = std::stoi(std::string(1, s[2]));

		std::bitset<5> b(i + 6);
		return b.to_string();
	}
	else if (s[1] == 'g')
	{
		std::bitset<5> b(28);
		return b.to_string();
	}
	else if (s[1] == 'f')
	{
		std::bitset<5> b(30);
		return b.to_string();
	}
	else if (s[1] == 'r')
	{
		std::bitset<5> b(31);
		return b.to_string();
	}
	else
	{
		std::cout << "Invalid register: " << s << '\n';
		exit(EXIT_FAILURE);
	}
	return "11111"; // never gets to here anyways because of else, just putting this in so compiler will shut up.
} // end GetReg

// 16 bit offset for lw & sw
std::string GetOffset(const std::string s, const std::map<std::string, size_t>& map)
{
	std::map<std::string, size_t>::const_iterator it = map.find(s);
	if (it != map.end())
	{
		size_t i = map.find(s)->second;
		size_t t = map.find(".data")->second;
		int o = i - t;
		std::bitset<16> b(o);
		return b.to_string();
	}
	else // offset is an integer
	{
		size_t o = std::stoi(s);
		std::bitset<16> b(o);
		return b.to_string();
	}
	return std::bitset<16>(0).to_string();
}
void Display(const std::map<size_t, std::bitset<32>>& bits)
{
	std::cout << '\n';

	for (std::map<size_t, std::bitset<32>>::const_iterator i = bits.begin(); i != bits.end(); ++i)
	{
		std::cout << ParseHex(i->second) << '\n';
	}
} // end Display

std::string ParseHex(const std::bitset<32> bits)
{
	std::string out;
	std::string bitstring = bits.to_string();

	for (size_t i = 0; i < bitstring.length(); i += 4)
	{
		std::string test;
	
		for (size_t j = i; j < (i + 4); ++j)
		{
			test += bitstring[j];
		}

		if (test.compare("0000") == 0)
		{
			out += "0";
		}
		else if (test.compare("0001") == 0)
		{
			out += "1";
		}
		else if (test.compare("0010") == 0)
		{
			out += "2";
		}
		else if (test.compare("0011") == 0)
		{
			out += "3";
		}
		else if (test.compare("0100") == 0)
		{
			out += "4";
		}
		else if (test.compare("0101") == 0)
		{
			out += "5";
		}
		else if (test.compare("0110") == 0)
		{
			out += "6";
		}
		else if (test.compare("0111") == 0)
		{
			out += "7";
		}
		else if (test.compare("1000") == 0)
		{
			out += "8";
		}
		else if (test.compare("1001") == 0)
		{
			out += "9";
		}
		else if (test.compare("1010") == 0)
		{
			out += "a";
		}
		else if (test.compare("1011") == 0)
		{
			out += "b";
		}
		else if (test.compare("1100") == 0)
		{
			out += "c";
		}
		else if (test.compare("1101") == 0)
		{
			out += "d";
		}
		else if (test.compare("1110") == 0)
		{
			out += "e";
		}
		else if (test.compare("1111") == 0)
		{
			out += "f";
		}
	} // end for

	return out;
} // end ParseHex


