#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define SCREEN_SIZE olc::vi2d{ 256, 240 }
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

class SpaceSim : public olc::PixelGameEngine
{
public:
	SpaceSim()
	{
		sAppName = "Spaaaace!!!";
	}

public:

	class PointObject {
		std::string name;
		olc::vd2d pos, vel, acc;
		double radius, mass;
		olc::Pixel color;

	public:
		PointObject(std::string objName, olc::vd2d position, olc::vd2d initVelocity, double objRadius, double objMass, olc::Pixel objColor = olc::WHITE)
			: name{ objName }, pos{ position }, vel{ initVelocity }, radius{ objRadius }, mass{ objMass }, color{ objColor } {}

		// Draws the point according to the mass it has
		void drawSelf(olc::PixelGameEngine *pge, olc::vi2d offset, double scale = 1.0f)
		{
			pge->FillCircle(((olc::vi2d)(pos * scale) + SCREEN_SIZE / 2) + offset, (int32_t)round(radius * scale), color);
		}

		// Modifies velocity and position according to a given acceleration
		void updatePos(float deltaTime, olc::vd2d newAcc)
		{
			//std::cout << "New pos: " << (pos + (vel) * (deltaTime)+(0.5f) * (newAcc) * (deltaTime * deltaTime)) << std::endl;

			pos = pos + (vel) * (double)(deltaTime) + (0.5f) * (newAcc) * ((double)deltaTime * (double)deltaTime);
			vel = vel + (newAcc) * (deltaTime);
			acc = newAcc;
		}

		// Adds to mass and maintains momentum
		void collide(PointObject& coll)
		{
			
		}

		olc::vd2d getPos()
		{
			return pos;
		}

		double getMass()
		{
			return mass;
		}

		olc::vd2d getVel()
		{
			return vel;
		}

		double getMomentum() 
		{
			return vel.mag() * mass;
		}

		olc::vd2d getAttraction(PointObject& obj)
		{
			olc::vd2d dist = obj.getPos() - pos;
			double radius = dist.mag();
			//double angle = MATH::atan2(dist.y, dist.x);

			//double accMag = -G * (mass / (radius * radius));

			//olc::vd2d out = olc::vd2d{ accMag * cos(angle), accMag * sin(angle) };

			/*if(false)
			std::cout << "PointObj: getAttraction"	<< "\n"
					  << "   dist:   " << dist		<< "\n"
					  << "   radius: " << radius	<< "\n"
					  << "   angle:  " << angle		<< "\n"
					  << "   accMag: " << accMag	<< "\n"
					  << "   out   : " << out		<< "\n\n";*/

			double accX = -(G * mass) * ((dist.x) / (radius * radius * radius));
			double accY = -(G * mass) * ((dist.y) / (radius * radius * radius));

			olc::vd2d out = olc::vd2d{accX, accY};

			return out;
		}

		std::string getInfo() // std::to_string(pos.mag())
		{
			return 
				to_upper(name) + " | "
				"{PO} | "
				"M: " + std::to_string(mass).substr(0, 4) + "(kg) | "
				"P: " + pos.strCut(9) + "(m) | "
				"V: " + vel.strCut(5) + "(m/s) | "
				"A: " + acc.strCut(8) + "(m/s^2) |";
		}
	};

	std::vector<PointObject*> objs;
	olc::vi2d camOffset;

	double bignum(double base, int power)
	{
		return base * pow(10.0, power);
	}

	bool OnUserCreate() override // 405400 * 1000
	{
		objs.push_back(new PointObject("Earth", { 0, 0 }	   , { 0, 0 }	 , 6370, bignum(5.97, 22), olc::BLUE));
		objs.push_back(new PointObject("Moon ", { 25480, 0 }, { 0, 12504.91 }, 1738, bignum(7.34, 20), olc::GREY)); // Apogee = 405400km, Perigee = 362600km


		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		//Clear(olc::BLACK);


		if (GetKey(olc::UP).bHeld)
			camOffset.y += 1;
		else if (GetKey(olc::DOWN).bHeld)
			camOffset.y -= 1;

		if (GetKey(olc::RIGHT).bHeld)
			camOffset.x -= 1;
		else if (GetKey(olc::LEFT).bHeld)
			camOffset.x += 1;

		for (auto *obj : objs)
		{
			olc::vd2d totalAcc = { 0, 0 };
			
			for (auto *o : objs) { if(o != obj) totalAcc += o->getAttraction(*obj); }

			std::cout << "Acc: " << totalAcc << " vel: " << obj->getVel() << std::endl;

			if(obj == objs[1])
				obj->updatePos(fElapsedTime, totalAcc);

			obj->drawSelf(this, camOffset, 0.002);
		}

		//DrawStringDecal({ 0,0 }, objs[0]->getInfo(), olc::WHITE, { 0.25f, 1 });
		//DrawStringDecal({ 0,8 }, objs[1]->getInfo(), olc::WHITE, { 0.25f, 1 });
		//DrawStringDecal({ 0,16 }, ("System momentum: " + std::to_string(objs[0]->getMomentum() + objs[1]->getMomentum())), olc::WHITE, { 0.25f, 1 });

		return true;
	}
};

int main()
{
	SpaceSim sim;
	if (sim.Construct(SCREEN_SIZE.x, SCREEN_SIZE.y, 4, 4, false, false))
		sim.Start();
	return 0;
}