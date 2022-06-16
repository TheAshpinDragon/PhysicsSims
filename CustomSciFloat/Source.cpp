#include <cmath>
#include <cstdint>
#include <string>
#include <iostream>
#include <streambuf>
#include <sstream>
#include <chrono>
#include <vector>
#include <list>
#include <thread>
#include <atomic>
#include <fstream>
#include <map>
#include <functional>
#include <algorithm>
#include <array>
#include <cstring>

struct sci {
	char type;
	uint_fast64_t mantisa;
	int_fast8_t exponent;

	sci(uint_fast64_t in) { }

	std::string to_string() { return "This is myUnit!"; }
};

sci operator"" _mu(uint_fast64_t x) { return sci(x); }

int main()
{
	auto a = 3e10;

	return 0;
}