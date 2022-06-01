#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define SCREEN_SIZE olc::vi2d{ 256, 240 }
#define TWO_PI		6.2831853072
#define PI			3.1415926535
#define HALF_PI 	1.5707963268
#define MASS_SCALE	(1000 * 1000 * 1000 * 1000 * 1000 * 1000 * 1000 * 1000 * 1000)
#define G			0.0000000000667428
#define G_ADJ		6.67428

namespace MATH {
	float atanf2(float y, float x)
	{
		if (x == 0.0f)
		{
			if (y == 0.0f) return 0.0f;
			return y > 0.0f ? HALF_PI : -HALF_PI;
		}

		if(y == 0.0f) return x > 0.0f ? 0 : PI;

		float out = atanf(y / x);

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
		olc::vf2d pos, vel, acc;
		float mass;

	public:
		PointObject(std::string objName, olc::vf2d position, olc::vf2d initVelocity, float objMass) : name{ objName }, pos { position }, vel{ initVelocity }, mass{ objMass } {}

		// Draws the point according to the mass it has
		void drawSelf(olc::PixelGameEngine *pge)
		{
			pge->FillCircle((olc::vi2d)pos + SCREEN_SIZE / 2, (int32_t)round(mass / 100));
		}

		// Modifies velocity and position according to a given acceleration
		void updatePos(float deltaTime, olc::vf2d newAcc)
		{
			//std::cout << "New pos: " << (pos + (vel) * (deltaTime)+(0.5f) * (newAcc) * (deltaTime * deltaTime)) << std::endl;

			pos = pos + (vel) * (deltaTime) + (0.5f) * (newAcc) * (deltaTime * deltaTime);
			vel = vel + (newAcc) * (deltaTime);
			acc = newAcc;
		}

		// Adds to mass and maintains momentum
		void collide(PointObject& coll)
		{
			
		}

		olc::vf2d getPos()
		{
			return pos;
		}

		float getMass()
		{
			return mass;
		}

		olc::vi2d getVel()
		{
			return vel;
		}

		olc::vf2d getAttraction(PointObject& obj)
		{
			olc::vf2d dist = obj.getPos() - pos;
			float radius = sqrtf((dist.x * dist.x) + (dist.y * dist.y));
			float angle = MATH::atanf2(dist.y, dist.x);

			double accMag = -(double)G_ADJ * (mass / (radius * radius)) * 100;

			olc::vf2d out = olc::vf2d{ (float)accMag * cos(angle), (float)accMag * sin(angle) };

			if(false)
			std::cout << "PointObj: getAttraction"	<< "\n"
					  << "   dist:   " << dist		<< "\n"
					  << "   radius: " << radius	<< "\n"
					  << "   angle:  " << angle		<< "\n"
					  << "   accMag: " << accMag	<< "\n"
					  << "   out   : " << out		<< "\n\n";

			return out;
		}

		std::string getInfo() // std::to_string(pos.mag())
		{
			return 
				to_upper(name) + " | "
				"{PO} | "
				"M: " + std::to_string(mass).substr(0, 4) + "(kg) | "
				"P: " + vel.strCut(5) + "(m) | "
				"V: " + vel.strCut(5) + "(m/s) | "
				"A: " + acc.strCut(5) + "(m/s^2) |";
		}
	};

	std::vector<PointObject*> objs;

	bool OnUserCreate() override
	{
		objs.push_back(new PointObject("Earth", { -40,0 }, { 0,40 }, 500.0f));
		objs.push_back(new PointObject("The Moon", { 40,0 }, { 0,-40 }, 500.0f));


		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		Clear(olc::BLACK);

		for (auto *obj : objs)
		{
			olc::vf2d totalAcc = { 0, 0 };
			
			for (auto *o : objs) { if(o != obj) totalAcc += o->getAttraction(*obj); }

			//std::cout << "Acc: " << totalAcc << " vel: " << obj->getVel() << std::endl;

			obj->updatePos(fElapsedTime, totalAcc);

			obj->drawSelf(this);
		}

		DrawStringDecal({ 0,0 }, objs[0]->getInfo(), olc::WHITE, {0.25f, 1});

		return true;
	}
};

int main()
{
	SpaceSim sim;
	if (sim.Construct(SCREEN_SIZE.x, SCREEN_SIZE.y, 4, 4, false, true))
		sim.Start();
	return 0;
}