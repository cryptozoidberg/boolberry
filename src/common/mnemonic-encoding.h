#include <string>
#include <vector>

namespace tools
{
	namespace mnemonic_encoding
	{  
		std::vector<unsigned char> text2binary(const std::string& text);
		std::string binary2text(const std::vector<unsigned char>& binary);
	}
}
