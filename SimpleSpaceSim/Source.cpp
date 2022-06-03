#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

/*
// TIME UNTIL 0.5% DEVIATION IN MOMENTUM
//   0.0001(s) : 21.63 x 20 = 432.6(s)
//   0.00005(s): 46.45 x 20 = 929.0(s)

*/

#define SCREEN_SIZE olc::vi2d{ 512, 480 }
#define TWO_PI		(double)6.2831853071795865
#define PI			(double)3.1415926535897932
#define HALF_PI 	(double)1.5707963267948966
#define G			(double)0.0000000000667428
#define TICK		0.00005f

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
	int digits = 0;
	while (in > 10.0) { in /= 10; digits++; }
	return std::to_string(in).substr(0, accuracy + 2) + "e+" + std::to_string(digits);
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

	struct ObjPosInfo {
		std::string name{};
		olc::vd2d pos{}, vel{}, acc{};

		ObjPosInfo() = default;

		ObjPosInfo(PointObject* copy)
			: name{ copy->getName() }, pos{ copy->getPos() }, vel{ copy->getVel() }, acc{} {}

		ObjPosInfo(std::string objName, olc::vd2d objPos, olc::vd2d objVel, olc::vd2d objAcc)
			: name{ objName }, pos { objPos }, vel{ objVel }, acc{ objAcc } {}
	};

	class PointObject {
		std::string name;
		olc::vd2d pos, vel, acc;
		ObjPosInfo buff;
		double radius, mass;
		olc::Pixel color;

	public:
		PointObject(std::string objName, olc::vd2d position, olc::vd2d initVelocity, double objRadius, double objMass, olc::Pixel objColor = olc::WHITE)
			: name{ objName }, pos{ position }, vel{ initVelocity }, buff{ this }, radius{ objRadius }, mass{ objMass }, color{ objColor } {}

		// Draws the point according to the mass it has
		void drawSelf(olc::PixelGameEngine *pge, olc::vi2d offset, double scale = 1.0f)
		{
			pge->FillCircle(((olc::vi2d)(pos * scale) + SCREEN_SIZE / 2) + offset, (int32_t)round(radius * scale), color);
		}

		void setToBuff()
		{
			pos = buff.pos;
			vel = buff.vel;
			acc = buff.acc;
		}

		// Modifies velocity and position according to a given acceleration
		void update(float deltaTime, ObjPosInfo& target)
		{
			target.pos += (target.vel) * (deltaTime) + (0.5) * (target.acc) * ((double)deltaTime * (double)deltaTime);
			target.vel += (target.acc) * (deltaTime);
		}

		void calcAttraction(float deltaTime, std::vector<PointObject*>& objs)
		{
			olc::vd2d sumAcc;

			for (auto* obj : objs)
			{
				if (obj == this) continue;

				// Find the triangle components (dist = legs, radius = hypotinuse)
				olc::vd2d dist = obj->getBuffPos() - buff.pos;
				double radius = dist.mag();

				// Calculate instantanious gravitational acceleration: g = Gm * (x'-x) / (r^3)
				double accX = (G * obj->getMass()) * ((dist.x) / (radius * radius * radius));
				double accY = (G * obj->getMass()) * ((dist.y) / (radius * radius * radius));

				// Add to total
				sumAcc += {accX, accY};
			}

			// Update buffer
			buff.acc = sumAcc;
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
				"V: " + vel.strCut(5) + "(m/s) | "
				"A: " + acc.strCut(8) + "(m/s^2) |";
		}

		std::string getName() { return name; }
		olc::vd2d getPos() { return pos; }
		olc::vd2d getBuffPos() { return buff.pos; }
		double getMass() { return mass; }
		olc::vd2d getVel() { return vel; }
		double getMomentum() { return sqrt((vel.x * vel.x) + (vel.y * vel.y)) * mass; }
		double getKeneticEnergy() { return 0.5 * mass * vel.mag() * vel.mag(); }
	};

	std::vector<PointObject*> objs;
	olc::vi2d camOffset;
	float zoom = 0.008;
	double initMomentum, initKe, initUg;

	double bignum(double base, int power)
	{
		return base * pow(10.0, power);
	}

	bool OnUserCreate() override // 405400 * 1000
	{
		objs.push_back(new PointObject("Earth", { 0, 0 }	   , { 0, -153.74866 }	 , 6370, bignum(5.97, 22), olc::BLUE));
		objs.push_back(new PointObject("Moon ", { 25480, 0 }, { 0, 12505.171 }, 1738, bignum(7.34, 20), olc::GREY)); // Apogee = 405400km, Perigee = 362600km

		for (auto* obj : objs)
			initMomentum = obj->getMomentum();
		
		double	M1 = objs[0]->getMass(),
				M2 = objs[1]->getMass();

		olc::vd2d	P1 = objs[0]->getPos(),
					P2 = objs[1]->getPos();

		olc::vd2d COM = ((M1 * P1) + (M2 * P2)) / (M1 + M2);

		double	R1 = (COM - P1).mag(),
				R2 = (P2 - COM).mag();

		double	EarthUg = (G * M1 * M2) / (R1),
				MoonUg = (G * M1 * M2) / (R2);

		initUg = EarthUg + MoonUg;
		initKe = (G * objs[0]->getMass() * objs[1]->getMass()) / (25480.0 * 2.0);

		//std::cout << "Ke1: " << initKe << " Ke2: " << (.5 * objs[1]->getMass() * 12505.171 * 12505.171) << "\n";

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		Clear(olc::BLACK);

		if (GetKey(olc::UP).bHeld)
			camOffset.y += 1;
		else if (GetKey(olc::DOWN).bHeld)
			camOffset.y -= 1;

		if (GetKey(olc::RIGHT).bHeld)
			camOffset.x -= 1;
		else if (GetKey(olc::LEFT).bHeld)
			camOffset.x += 1;

		fElapsedTime *= 1;

		for (int i = (int)ceilf(fElapsedTime / TICK); i > 0; i--)
		{
			float dt = i != 1 ? TICK : fmod(fElapsedTime, TICK);

			for (auto* obj : objs)
				obj->calcAttraction(dt, objs);
		}

		double	totalMomentum = 0,
				totalKe = 0,
				totalUg = 0;

		for (auto* obj : objs)
		{
			obj->setToBuff();
			obj->drawSelf(this, camOffset, zoom);
			totalMomentum += obj->getMomentum();
			totalKe += obj->getKeneticEnergy();
		}

		double	M1 = objs[0]->getMass(),
				M2 = objs[1]->getMass();

		olc::vd2d	P1 = objs[0]->getPos(),
					P2 = objs[1]->getPos();

		olc::vd2d	V1 = objs[0]->getVel(),
					V2 = objs[1]->getVel();

		olc::vd2d COM = ((M1 * P1) + (M2 * P2)) / (M1 + M2);
		olc::vd2d COMv = ((M1 * V1) + (M2 * V2)) / (M1 + M2);

		DrawCircle(((P1 * zoom) + camOffset + SCREEN_SIZE / 2), 1);
		DrawCircle(((P2 * zoom) + camOffset + SCREEN_SIZE / 2), 1);
		FillCircle(((COM * zoom) + camOffset + SCREEN_SIZE / 2), 1, olc::YELLOW);

		double	R1 = (COM - P1).mag(),
				R2 = (P2 - COM).mag();

		double	EarthUg = (G * M1 * M2) / (R1),
				MoonUg = (G * M1 * M2) / (R2);

		double	SystemKe = 0.5 * (M1 + M2) * (COMv.mag() * COMv.mag());

		totalUg = EarthUg + MoonUg;
		totalKe += SystemKe;

		DrawStringDecal({ 0,0 * 2 }, objs[0]->getInfo(), olc::WHITE, olc::vf2d{ 0.25f, 1 } * 2);
		DrawStringDecal({ 0,8 * 2 }, objs[1]->getInfo(), olc::WHITE, olc::vf2d{ 0.25f, 1 } * 2);

		//double momentumDeviationPercent = ((totalMomentum - initMomentum) / initMomentum) * 100;
		//DrawStringDecal({ 0,16 }, ("Momenum deviation: " + std::to_string(momentumDeviationPercent)) + "%", olc::WHITE, { 0.25f, 1 });
		DrawStringDecal({ 0,24 * 2 }, ("Ke: " + to_sci(totalKe, 4)) + "(N)", olc::WHITE, olc::vf2d{ 0.25f, 1 } * 2);
		DrawStringDecal({ 0,32 * 2 }, ("Ug: " + to_sci(totalUg, 4)) + "(N)", olc::WHITE, olc::vf2d{ 0.25f, 1 } * 2);
		DrawStringDecal({ 0,40 * 2 }, ("Es: " + to_sci(totalKe + totalUg, 4) + "(N) Diff: " + std::to_string((totalKe - initKe + totalUg - initUg) / (initKe + initUg) * 100) + "%"), olc::WHITE, olc::vf2d{ 0.25f, 1 } * 2);
		DrawStringDecal({ 0,48 * 2 }, "COM: " + COM.strCut(5), olc::WHITE, olc::vf2d{ 0.25f, 1 } *2);
		DrawStringDecal({ 0,56 * 2 }, "COMv: " + COMv.strCut(5), olc::WHITE, olc::vf2d{ 0.25f, 1 } *2);


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