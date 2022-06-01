
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

namespace fbd {
	class trnVec {};

	class rotVec {};

	class diagram {};

	class graph {};

	class freeBody {
		olc::vf2d pos;
		fbd::trnVec vel, acc;
		fbd::rotVec omega, alpha;

		fbd::diagram forceDiagram;

		
	};
}

int main()
{
	std::cout << "Hello world!";
	
	return 1;
}