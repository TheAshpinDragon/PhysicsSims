#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

// maybe usefull: https://www.youtube.com/watch?v=Qyn64b4LNJ0
// LIGHT SIM??? https://www.youtube.com/watch?v=975r9a7FMqc

/*
// TIME UNTIL 0.5% DEVIATION IN MOMENTUM
//   0.0001(s) : 21.63 x 20 = 432.6(s)
//   0.00005(s): 46.45 x 20 = 929.0(s)

// TIME UNTIL 0.05% DEVIATION IN ENERGY
//   ~6(s) with no jerk
//   ~43(s) with jerk

*/

// TODO: Add the ability to create more complex systems: binarys (act like one object), multiple orbits (earth x sun, moon x earth)
// TODO: Find out why the % error occilates, it is especially noticable with multi-body systems
// TODO: Add more UI and the ability to create or modify systems
// TODO: Find a better aproximation of the motion of bodies (this works in 50 micro-second incraments, with jerk calculated with a 50 micro-second delay)
// TODO: Add collisions and momentum, possibly accrition or melding as well
// TODO: Extract some functions that may be used in the future into a header
// TODO: Create a scientific typedef, uses a char for sign and unit info, uint_fast64_t (18 digits of accuracy) for the mantisa, and int_fast16_t for the exponent, and various units to keep things managable

#define DIG(x)		((x >= 0) + (x >= 10) + (x >= 100))
#define SCREEN_SIZE olc::vi2d{ 512, 480 }
#define TWO_PI		(double)6.2831853071795865
#define PI			(double)3.1415926535897932
#define HALF_PI 	(double)1.5707963267948966
#define G			(double)0.0000000000667428

namespace MATH {
	double atan2(double y, double x)
	{
		if (x == 0.0f)
		{
			if (y == 0.0f) return 0.0f;
			return y > 0.0f ? HALF_PI : -HALF_PI;
		}

		if(y == 0.0f) return x > 0.0f ? 0 : PI;

		double out = atan(y / x);

		if (x < 0.0f) out += PI;
		if (y < 0.0f && x > 0.0f) out += TWO_PI;

		return out;
	}
}

std::string to_upper(std::string in) { for (auto& c : in) c = toupper(c); return in; }

std::string to_sci(double in, int accuracy)
{
	// If it's already in the bounds of the scientific notation, skip
	if (in < 10.0 && in > 1.0 || in == 0) return std::to_string(in).substr(0,accuracy + 2);

	// Count the number of digits
	int digits = 0;
	if(in > 10.0) // Greater than ten, devide
		while (in > 1.0) {
			in /= 10; digits++;
			if (isnan(in)) return "NAN";
			if (isinf(in)) return (in < 0 ? "-INF" : "+INF");
		}
	else if(in < 1.0) // Less than one, multiply
		while (in < 0.1) {
			in *= 10; digits--;
			if (isnan(in)) return "NAN";
			if (isinf(in)) return (in < 0 ? "-INF" : "+INF");
		}

	// If applying the scientific notation makes the string longer, skip
	//if (digits + accuracy + 1 < accuracy + 2) return std::to_string(in * pow(10, digits)).substr(0, digits + accuracy + 2);

	// Put into scientific notation
	return std::to_string(in).substr(0, accuracy + 2) + "e" + std::to_string(digits);
}

class SpaceSim : public olc::PixelGameEngine
{
public:
	SpaceSim()
	{
		sAppName = "Spaaaace!!!";
	}

public:

	struct ObjPosInfo;
	class PointObject;
	struct Counter;
	class BigCounter;

	struct ObjPosInfo {
		std::string name{};
		olc::vd2d pos{}, vel{}, acc{}, jerk{};

		ObjPosInfo() = default;

		ObjPosInfo(PointObject* copy)
			: name{ copy->getName() }, pos{ copy->getPos() }, vel{ copy->getVel() }, acc{}, jerk{} {}

		ObjPosInfo(std::string objName, olc::vd2d objPos, olc::vd2d objVel, olc::vd2d objAcc, olc::vd2d objJerk)
			: name{ objName }, pos{ objPos }, vel{ objVel }, acc{ objAcc }, jerk{ objJerk } {}

		bool checkNan()
		{
			if (isnan(pos.x) || isnan(pos.y))
				return true;

			if (isnan(vel.x) || isnan(vel.y))
				return true;

			if (isnan(acc.x) || isnan(acc.y))
				return true;

			if (isnan(jerk.x) || isnan(jerk.y))
				return true;

			return false;
		}
	};

	class PointObject {
		std::string name;
		olc::vd2d pos, vel, acc;
		ObjPosInfo buff;
		double radius, mass, gravParam;
		olc::Pixel color;
		bool calculate;

	public:
		PointObject(
			std::string objName,
			olc::vd2d position, olc::vd2d initVelocity,
			double objRadius, double objMass, double gravitationalParameter,
			olc::Pixel objColor = olc::WHITE, bool active = true)
			: name{ objName }, pos{ position }, vel{ initVelocity }, buff{ this }, radius{ objRadius }, gravParam{ gravitationalParameter }, mass{ objMass }, color{ objColor }, calculate{ active } {}

		// Draws the point according to the mass it has
		void drawSelf(olc::PixelGameEngine *pge, olc::vi2d offset, double scale = 1.0f)
		{
			pge->FillCircle(((pos * scale) + SCREEN_SIZE / 2) + offset, (int32_t)round(radius * scale), color);
			pge->DrawCircle(((pos * scale) + SCREEN_SIZE / 2 + offset), 1);
		}

		void setToBuff()
		{
			pos = buff.pos;
			vel = buff.vel;
			acc = buff.acc;
		}

		// Modifies velocity and position according to a given acceleration
		void update(double deltaTime, ObjPosInfo& target)
		{
			if (target.checkNan())
				throw;

			// x = x0 + vt + (0.5)at^2 + (0.2)jt^3
			target.pos += 
				(target.vel * (deltaTime)) + 
				(0.5 * target.acc * (deltaTime * deltaTime)) + 
				(0.2 * target.jerk * (deltaTime * deltaTime * deltaTime));

			// v = v0 + at + (0.5)jt^2
			target.vel += 
				(target.acc * (deltaTime)) + 
				(0.5 * target.jerk * (deltaTime * deltaTime));

			// a = a0 + jt
			target.acc += (target.jerk * (deltaTime));
		}

		// Cowell's Method, just uses Newton's equations
		olc::vd2d getInstantAttraction(std::vector<PointObject*>& objs)
		{
			olc::vd2d sumAcc;

			for (auto* obj : objs)
			{
				if (obj == this) continue;

				// Find the triangle components (dist = legs, radius = hypotinuse)
				olc::vd2d dist = obj->getBuffPos() - buff.pos;
				double radius = dist.mag();

				// We can get the force of gravity through newton's equation: Fg = G(m1m2) / r^2
				// We then use the triangle to determine the ratios for each axis: (x'-x) / r
				// And finaly find the acceleration with newton's: F = ma
				// Combine them all to calculate instantanious gravitational acceleration: g = Gm * ((x'-x) / (r^3))
				double accX = obj->getGravParam() * ((dist.x) / (radius * radius * radius));
				double accY = obj->getGravParam() * ((dist.y) / (radius * radius * radius));

				// Add to total
				sumAcc += {accX, accY};
			}

			return sumAcc;
		}

		void initAttraction(std::vector<PointObject*>& objs)
		{
			buff.acc = getInstantAttraction(objs);
			update(0.0000001f, buff);
		}

		void calcAttraction(float deltaTime, std::vector<PointObject*>& objs)
		{
			if (!calculate) return;

			olc::vd2d instAcc = getInstantAttraction(objs);

			// Update buffer
			buff.jerk = (instAcc - buff.acc) / deltaTime;
			update(deltaTime, buff);
		}

		// Adds to mass and maintains momentum
		void collide(PointObject& coll)
		{

		}

		std::string getInfo() // std::to_string(pos.mag())
		{
			return 
				to_upper(name) + " | "
				"{PO} | "
				"M: " + to_sci(mass, 2) + "(kg) | "
				"P: " + pos.strCut(9) + "(m) | "
				"V: " + vel.strCut(8) + "(m/s) | "
				"A: " + acc.strCut(8) + "(m/s^2) |";
		}

		std::string getName() { return name; }
		olc::vd2d getPos() { return pos; }
		olc::vd2d getBuffPos() { return buff.pos; }
		olc::vd2d getVel() { return vel; }
		void setVel(olc::vd2d in) { vel = in; buff = ObjPosInfo(this); }
		double getMass() { return mass; }
		double getGravParam() { return gravParam; }

		double getMomentum() { return sqrt((vel.x * vel.x) + (vel.y * vel.y)) * mass; }
		double getKeneticEnergy() { return 0.5 * mass * vel.mag() * vel.mag(); }
	};

	/*
	class BigCounter {
	public:
		BigCounter() {}

		void inc(float in)
		{
			carry = sec.inc(in);
			if (carry == 0) return;

			for(ICount i : counters)
			{
				i.inc(carry);
				if (carry == 0) return;
			}

			overflow += carry;
		}

		std::string fomatAllToStr(char devider = '\n')
		{
			std::string out;
			for (auto c = counters.rbegin(); c != counters.rend(); ++c)
			{
				out += c->str() + devider;
			}
		}

		std::string fomatGreatestToStr()
		{

		}

	private:
		enum class UNIT : uint_fast8_t {
			SEC_	= 1,
			MIN_	= 60,
			HOUR	= 60,
			DAY_	= 24,
			F_YR	= 73,
			YEAR	= 4,
			HUND	= 100,
			THOU	= 100,
			MILL	= 100,
			BILL	= 100,
			TRIL	= 100,
		};

		const std::map<UNIT, uint_fast8_t> UNIT_DIG_CNT = {
			{UNIT::SEC_, 1},
			{UNIT::MIN_, 2},
			{UNIT::HOUR, 2},
			{UNIT::DAY_, 2},
			{UNIT::F_YR, 2},
			{UNIT::YEAR, 1},
			{UNIT::HUND, 3},
			{UNIT::THOU, 3},
			{UNIT::MILL, 3},
			{UNIT::BILL, 3},
			{UNIT::TRIL, 3}
		};

		const std::map<UNIT, std::string> UNIT_STR = {
			{UNIT::SEC_, "Second"},
			{UNIT::MIN_, "Minute"},
			{UNIT::HOUR, "Hour"},
			{UNIT::DAY_, "Day"},
			{UNIT::F_YR, ""},
			{UNIT::YEAR, "Year"},
			{UNIT::HUND, "Hundred Years"},
			{UNIT::THOU, "Thousand Years"},
			{UNIT::MILL, "Million Years"},
			{UNIT::BILL, "Billion Years"},
			{UNIT::TRIL, "Trillion Years"}
		};

		const std::map<UNIT, std::string> UNIT_ABRV_STR = { // abbreviation
			{UNIT::SEC_, "Sec"},
			{UNIT::MIN_, "Min"},
			{UNIT::HOUR, "Hr"},
			{UNIT::DAY_, "Day"},
			{UNIT::F_YR, ""},
			{UNIT::YEAR, "Yr"},
			{UNIT::HUND, "Hund Yr"},
			{UNIT::THOU, "Thou Yr"},
			{UNIT::MILL, "Mill Yr"},
			{UNIT::BILL, "Bill Yr"},
			{UNIT::TRIL, "Tril Yr"}
		};

		template<class StoreType, class OuterType>
		struct Counter {
			StoreType num;
			UNIT unit;

			Counter(UNIT u) : unit{u}, num{0} {}

			OuterType inc(OuterType in)
			{
				OuterType out = ((OuterType)num + in) / (OuterType)unit;
				num = (StoreType)((num + in) - out);
				return out;
			}

			std::string str()
			{
				return std::string(UNIT_DIG_CNT[unit] - (DIG(num)), '0') + std::to_string(num) + UNIT_STR[unit];
			}
		};
		typedef Counter<uint_fast8_t, uint_fast64_t> ICount;
		typedef Counter<float, float> FCount;

		FCount sec = FCount(UNIT::SEC_);
		std::vector<ICount> counters = {
			ICount(UNIT::MIN_),
			ICount(UNIT::HOUR),
			ICount(UNIT::DAY_),
			ICount(UNIT::F_YR),
			ICount(UNIT::YEAR),
			ICount(UNIT::HUND),
			ICount(UNIT::THOU),
			ICount(UNIT::MILL),
			ICount(UNIT::BILL),
			ICount(UNIT::TRIL),
		};
		uint_fast64_t carry{0}, overflow{0};

	};
*/

	class SimpleBigCounter {
	private:
		enum class STORAGE_CONSTS : uint_fast64_t {
			//18446744073707280000,	// = 584,942,417,355 years

			LOW_MAX			= 15768000000000000000, // Equivalent to 500,000,000,000 years, also the base of high in seconds
			LOW_PER_YEAR	= 31536000,				// Equivalent to one year in seconds
			LOW_REM			= 2678744073709551615,	// Equivalent to 2^64 - LOW_MAX - 1
			HIGH_IN_YEARS	= 500000000000			// The base of high in years
		};

		enum UNIT : uint_fast64_t {
			SEC_ = 1,
			MIN_ = 60,
			HOUR = 60,
			DAY_ = 24,
			YEAR = 365,
			HUND = 1000,
			THOU = 1000,
			MILL = 1000,
			BILL = 1000,
			TRIL = 1000,
		};

		const std::map<UNIT, std::string> UNIT_STR = {
			{UNIT::SEC_, "Seconds"},
			{UNIT::MIN_, "Minutes"},
			{UNIT::HOUR, "Hours"},
			{UNIT::DAY_, "Days"},
			{UNIT::YEAR, "Years"},
			{UNIT::HUND, "Hundred Years"},
			{UNIT::THOU, "Thousand Years"},
			{UNIT::MILL, "Million Years"},
			{UNIT::BILL, "Billion Years"},
			{UNIT::TRIL, "Trillion Years"}
		};

		const std::map<UNIT, uint_fast8_t> UNIT_DIG_CNT = {
			{UNIT::SEC_, 1},
			{UNIT::MIN_, 2},
			{UNIT::HOUR, 2},
			{UNIT::DAY_, 2},
			{UNIT::YEAR, 3},
			{UNIT::HUND, 3},
			{UNIT::THOU, 3},
			{UNIT::MILL, 3},
			{UNIT::BILL, 3},
			{UNIT::TRIL, 3}
		};

		uint_fast64_t high{0}, low{0};
		float underflow;

	public:
		void inc(float in)
		{
			uint_fast64_t carry = (uint_fast64_t)(underflow + in);
			underflow = fmod(underflow + in, 1);

			if (carry < (uint_fast64_t)STORAGE_CONSTS::LOW_MAX - low)
				low += carry;
			else
			{
				low = carry - ((uint_fast64_t)STORAGE_CONSTS::LOW_MAX - low);
				high++;
			}
		}

		std::string all_str()
		{
			uint_fast64_t	lowTemp = low;
			bool carryHalfBillion = high % 2 == 1;

			uint_fast8_t	sec		= lowTemp % MIN_,
							min		= (lowTemp /= MIN_) % HOUR,
							hour	= (lowTemp /= HOUR) % DAY_;
			uint_fast16_t	day		= (lowTemp /= DAY_) % YEAR;
			uint_fast16_t	yearH	= (lowTemp /= YEAR) % HUND,
							yearT	= (lowTemp /= HUND) % THOU,
							yearM	= (lowTemp /= THOU) % MILL,
							yearB	= (lowTemp /= MILL) % TRIL + (carryHalfBillion ? 500 : 0);
			uint_fast64_t	yearTr	= high / 2;

			auto getDigStr = [this](UNIT u, uint_fast64_t num, char end, bool pad = true) {
				return std::string((UNIT_DIG_CNT.at(u) - DIG(num)) * pad, '0') + std::to_string(num) + end;
			};

			std::string out;

			if (yearTr)	out += getDigStr(TRIL, yearTr,	',', false);
			if (yearB)	out += getDigStr(TRIL, yearB,	',', yearTr > 0);
			if (yearM)	out += getDigStr(BILL, yearM,	',', yearB > 0);
			if (yearT)	out += getDigStr(MILL, yearT,	',', yearM > 0);
			if (yearH)	out += getDigStr(THOU, yearH,	'y', yearT > 0);
			if (day)	out += getDigStr(YEAR, day,		'd', yearH > 0);
			if (hour)	out += getDigStr(DAY_, hour,	'h', day > 0);
			if (min)	out += getDigStr(HOUR, min,		'm', hour > 0);
			if (sec)	out += getDigStr(MIN_, sec,		's', min > 0);

			return out;
		}
	};

	// General use

	olc::vd2d getVec(double mag, double rad)
	{
		return olc::vd2d{cos(rad) * mag, sin(rad) * mag};
	}

	double calcSemiMajor(PointObject* parent, PointObject* satallite)
	{
		return calcSemiMajor(
			satallite->getVel().mag(),
			(satallite->getPos() - parent->getPos()).mag(),
			parent->getGravParam());
	}

	double calcSemiMajor(double vel, double parentDist, double gravParam)
	{
		double	numerator = -(gravParam * parentDist),
				denominator = vel * vel * parentDist - 2 * gravParam,
				out = numerator / denominator;

		if (isnan(out)) return -1;

		return out;
	}

	olc::vd2d calcCOM(std::vector<PointObject*>& system)
	{
		double totalMass{0};
		olc::vd2d COM{};

		for (auto* obj : system) { totalMass += obj->getMass(); }
		for (auto* obj : system) { COM += (obj->getMass() * obj->getPos()) / totalMass; }

		return COM;
	}

	olc::vd2d calcCOMVelocity(std::vector<PointObject*>& system)
	{
		double totalMass{ 0 };
		olc::vd2d COMv{};

		for (auto* obj : system) { totalMass += obj->getMass(); }
		for (auto* obj : system) { COMv += (obj->getMass() * obj->getVel()) / totalMass; }

		return COMv;
	}

	double calcRadius(PointObject* obj, olc::vd2d pos)
	{
		return (pos - obj->getPos()).mag();
	}

	// Energy calculations

	double calcOrbitUg(PointObject* parent, std::vector<PointObject*>& system)
	{
		double totalNumerator = G, totalDenominator{0};
		for (auto* obj : system) { totalNumerator *= obj->getMass(); }
		for (auto* obj : system) { totalDenominator += calcSemiMajor(parent, obj); }
		return (totalNumerator / totalDenominator);
	}

	double calcOrbitKe(PointObject* parent, std::vector<PointObject*>& system)
	{
		double totalNumerator = G, totalDenominator{0};
		for (auto* obj : system) { totalNumerator *= obj->getMass(); }
		for (auto* obj : system) { totalDenominator += (calcSemiMajor(parent, obj) * 2); }
		return totalNumerator / totalDenominator;
	}

	double calcVelocityKe(std::vector<PointObject*>& system)
	{
		double totalKe{0};
		for (auto* obj : system) { totalKe += obj->getKeneticEnergy(); }
		return totalKe;
	}

	double calcCOMKe(std::vector<PointObject*>& system, olc::vd2d systemCOMVelocity)
	{
		double totalMass{ 0 };
		for (auto* obj : system) { totalMass += obj->getMass(); }
		return 0.5 * totalMass * systemCOMVelocity.mag() * systemCOMVelocity.mag();
	}

	// Orbit setup

	void setOrbitalSpeed(PointObject* parent, std::vector<PointObject*>& system) // , double eccentricity = 0
	{
		olc::vd2d COM = calcCOM(system);
		double	Rp = calcRadius(parent, COM),
				Rnp = 0,
				Rn = 0,
				Fgp = 0;
		
		std::cout << "COM: " << COM << "\nParent radius: " << Rp << " | ";

		for (auto* obj : system)
		{
			if (obj == parent) continue;
			Rnp = calcRadius(obj, parent->getPos());
			std::cout << obj->getName() << " radius: " << Rnp << " & ";
			Fgp += (G * obj->getMass()) / (Rnp * Rnp);
			std::cout << " Fgn: " << (G * obj->getMass()) / (Rnp * Rnp) << " | ";
		}
		
		double	Vp = sqrt(Rp * Fgp),
				Vn = 0,
				Mp = parent->getMass(),
				Pp = Mp * Vp;
		
		std::cout << "\nTotal Fg on parent: " << Fgp << "\nParent velocity: " << olc::vd2d{0, -Vp};

		for (auto* obj : system)
		{
			if (obj == parent) continue;
			Rnp = calcRadius(obj, parent->getPos());
			Rn = calcRadius(obj, COM);
			Vn = sqrt((G * Mp * Rn) / (Rnp * Rnp));
			obj->setVel(olc::vd2d{ 0, Vn });
			std::cout << obj->getName() << " velocity: " << obj->getVel() << " | ";
		}
		std::cout << std::endl;
		parent->setVel({0,-Vp});

		//double r = (parent->getPos() - satallite->getPos()).mag();
		//double vel = sqrt((G * satallite->getMass() * calcRadius(parent, COM)) / (r * r));
		//parent->setVel({0, vel});
		//satallite->setVel(-(parent->getVel() * parent->getMass()) / satallite->getMass());

		/*std::cout << "COM: " << COM <<
			" total radius: " << r << " parent radius: " << calcRadius(parent, COM) << " satallite radius: " << calcRadius(satallite, COM) <<
			" parent vel: " << parent->getVel() << " satallite vel: " << satallite->getVel() << "\n";*/
	}

	std::vector<PointObject*> objs;
	SimpleBigCounter time;
	olc::vi2d camOffset;
	float	zoomFactor = 0.02f,
			zoom = 1e-9,
			tick = 1.0f;
	float	simSpeed = 2000000;
	double initMomentum, initKe, initUg;
	bool   followCenterObj = false;

	void checkInput()
	{
		// Move camera
		if (GetKey(olc::UP).bHeld)
			camOffset.y += 1;
		else if (GetKey(olc::DOWN).bHeld)
			camOffset.y -= 1;

		if (GetKey(olc::RIGHT).bHeld)
			camOffset.x -= 1;
		else if (GetKey(olc::LEFT).bHeld)
			camOffset.x += 1;

		// Zoom
		if (GetMouseWheel() && 1.0f / zoom > 1.0f && zoom > 1e-30)
			zoom *= GetMouseWheel() > 0 ? 1.05f : 0.95f;

		// Sim speed
		if (GetKey(olc::EQUALS).bHeld && GetFPS() > 30)
			simSpeed *= 1.01;
		else if (GetKey(olc::MINUS).bHeld && simSpeed > 0.00001)
			simSpeed /= 1.01;

		//  Tic speed
		if (GetKey(olc::OEM_4).bPressed && tick > 0.00001)
			tick *= 1.1;
		else if (GetKey(olc::OEM_6).bPressed && GetFPS() > 30)
			tick /= 1.1;

		// Folow
		if (GetKey(olc::F).bPressed) followCenterObj = !followCenterObj;
	}

	bool OnUserCreate() override
	{
		//objs.push_back(new PointObject("Earth1", { 12740, 0 }, {}, 6370, bignum(5.97, 22), olc::BLUE));
		//objs.push_back(new PointObject("Earth2", { -12740, 0 }, {}, 6370, bignum(5.97, 22), olc::GREY));

		//objs.push_back(new PointObject("Earth", { 0, 0 }, {}, 6370, bignum(5.97, 22), olc::BLUE));
		//objs.push_back(new PointObject("Moon ", { 25480, 0 }, {}, 1738, bignum(7.34, 20), olc::GREY)); // Apogee = 405400km, Perigee = 362600km

		//objs.push_back(new PointObject("Earth ", { 0, 0 }, {0, -288.0846}, 6370, bignum(5.97, 22), olc::BLUE));
		//objs.push_back(new PointObject("Earth2", { 25480, 0 }, {}, 6370, bignum(5.97, 22), olc::BLUE));
		//objs.push_back(new PointObject("Moon1 ", { 25480.0 * 2, 0 }, {0, 8462.9707}, 1738, bignum(7.34, 20), olc::GREY)); // Apogee = 405400km, Perigee = 362600km
		//objs.push_back(new PointObject("Moon2 ", { 25480.0 * 12, 0 }, {0, 3584.5741}, 1738, bignum(7.34, 20), olc::GREY)); // Apogee = 405400km, Perigee = 362600km

		//Perihelion:	46.0	107.5	147.1	206.7
		//Velocity:		47.4	35.0	29.8	24.1
		//Diamiter:		4879	12,104	12,756	6792
		//mass:			0.330	4.87	5.97	0.642

		//								name		position					velocity							diamiter	mass		gravParam			color
		objs.push_back(new PointObject("The Sun ",	getVec( 0.0, 0.0 ),			getVec(0, 0),						1.3927e9,	1.989e30,	1.32712440018e20,	olc::YELLOW));
		objs.push_back(new PointObject("Mercury ",	getVec( 46.00e9, 0.84436),	getVec(58.98e3, -HALF_PI + 0.84436),	4879.e3,	0.330e24,	2.2032e13,			olc::GREY));
		objs.push_back(new PointObject("Venus ",	getVec( 107.5e9, 1.34015),	getVec(35.26e3, -HALF_PI + 1.34015),	12104e3,	4.870e24,	3.24859e14,			olc::DARK_YELLOW));
		objs.push_back(new PointObject("Earth ",	getVec( 147.1e9, 0.31891),	getVec(30.29e3, -HALF_PI + 0.31891),	12756e3,	5.970e24,	3.986004418e14,		olc::BLUE));
		objs.push_back(new PointObject("Mars ",		getVec( 206.7e9, HALF_PI + 0.86685), getVec(26.50e3,  0.86685),	6792.e3,	0.642e24,	4.282837e13,		olc::DARK_RED));
		//objs.push_back(new PointObject("Rogue Sun ",	{ 2.0e12, 4.5e12 },		getVec(4e4, PI),					2.3927e9,	3.989e32,	3.32712440018e22,	olc::BLUE));
		//objs.push_back(new PointObject("Sag A* ",	{ 2.0e6, 4.5e12 },		getVec(4e3, PI),						4.03e10,	2.8e36,	1.82e26,	olc::DARK_BLUE));


		//setOrbitalSpeed(objs[0], objs);

		for (auto* obj : objs)
			obj->initAttraction(objs);

		for (auto* obj : objs)
			initMomentum = obj->getMomentum();
		
		olc::vd2d COM = calcCOM(objs);

		initUg = calcOrbitUg(objs[0], objs);
		initKe = calcOrbitKe(objs[0], objs);

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		Clear(olc::BLACK);
		DrawStringDecal({ 0,64 * 2 }, "S-M Axis: " + to_sci(calcSemiMajor(objs[0], objs[3]), 3), olc::WHITE, olc::vf2d{ 0.25f, 1 } *2);

		checkInput();

		fElapsedTime *= simSpeed;

		time.inc(fElapsedTime);

		for (int i = (int)ceilf(fElapsedTime / tick); i > 0; i--)
		{
			float dt = i != 1 ? tick : fmod(fElapsedTime, tick);

			if (dt == 0.0f) break;

			for (auto* obj : objs) obj->calcAttraction(dt, objs);

		}

		for (auto* obj : objs)
			obj->setToBuff();

		if(followCenterObj)
			camOffset = -objs[0]->getPos() * zoom;

		for (auto* obj : objs)
		{
			obj->drawSelf(this, camOffset, zoom);
		}

		double	totalKe = 0,
				totalUg = 0;

		olc::vd2d COM = calcCOM(objs);
		olc::vd2d COMv = calcCOMVelocity(objs);

		//FillCircle(((COM * zoom) + camOffset + SCREEN_SIZE / 2), 1, olc::YELLOW);

		totalUg = calcOrbitUg(objs[0], objs);
		totalKe = calcOrbitKe(objs[0], objs) + calcCOMKe(objs, COMv);

		//DrawStringDecal({ 0,0 * 2 }, objs[0]->getInfo(), olc::WHITE, olc::vf2d{ 0.25f, 1 } * 2);
		//DrawStringDecal({ 0,8 * 2 }, objs[1]->getInfo(), olc::WHITE, olc::vf2d{ 0.25f, 1 } * 2);

		//double momentumDeviationPercent = ((totalMomentum - initMomentum) / initMomentum) * 100;
		//DrawStringDecal({ 0,16 }, ("Momenum deviation: " + std::to_string(momentumDeviationPercent)) + "%", olc::WHITE, { 0.25f, 1 });
		DrawStringDecal({ 0,24 * 2 }, ("Ke: " + to_sci(totalKe, 4)) + "(N)", olc::WHITE, olc::vf2d{ 0.25f, 1 } * 2);
		DrawStringDecal({ 0,32 * 2 }, ("Ug: " + to_sci(totalUg, 4)) + "(N)", olc::WHITE, olc::vf2d{ 0.25f, 1 } * 2);
		DrawStringDecal({ 0,40 * 2 }, ("Es: " + to_sci(totalKe + totalUg, 4) + "(N) Diff: " + std::to_string((totalKe - initKe + totalUg - initUg) / (initKe + initUg) * 100) + "%"), olc::WHITE, olc::vf2d{ 0.25f, 1 } * 2);
		DrawStringDecal({ 0,48 * 2 }, "COM: " + COM.strCut(5), olc::WHITE, olc::vf2d{ 0.25f, 1 } *2);
		DrawStringDecal({ 0,56 * 2 }, "COMv: " + COMv.strCut(5), olc::WHITE, olc::vf2d{ 0.25f, 1 } *2);
		DrawStringDecal({ 0,(float)SCREEN_SIZE.y - 32 }, "TIME: " + time.all_str(), olc::WHITE, olc::vf2d{ 0.25f, 1 } *2);
		DrawStringDecal({ 0,(float)SCREEN_SIZE.y - 16 }, "ZOOM: " + to_sci(zoom, 4) + "\tSIM SPEED: " + std::to_string(simSpeed) + "\tTICK: " + std::to_string(tick), olc::WHITE, olc::vf2d{ 0.25f, 1 } *2);

		return true;
	}
};

int main()
{
	SpaceSim sim;
	if (sim.Construct(SCREEN_SIZE.x, SCREEN_SIZE.y, 2, 2, false, true))
		sim.Start();
	return 0;
}