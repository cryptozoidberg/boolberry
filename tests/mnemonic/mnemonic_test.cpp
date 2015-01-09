#include "mnemonic-encoding.h"
#include <vector>
#include <ctime>
#include <cassert>

using namespace std;
using namespace crypto::mnemonic_encoding;

vector<unsigned char> getRandomBinary(unsigned length)
{
	vector<unsigned char> res;
	for(unsigned i = 0; i < length; ++i)
	{
		res.push_back(rand());	
	}
	return res;
}

#define REPEAT_COUNT 100000

int main(int argc, char *argv[]) 
{
	srand(time(0));
	for(unsigned i = 0; i < REPEAT_COUNT; ++i)
	{
		vector<unsigned char> bin = getRandomBinary(4 * (i % 15 + 1));
		assert(bin == text2binary(binary2text(bin)));
	}
	return 0;
}

