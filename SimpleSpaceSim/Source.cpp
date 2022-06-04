#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

// maybe usefull: https://www.youtube.com/watch?v=Qyn64b4LNJ0
// LIGHT SIM??? https://www.youtube.com/watch?v=975r9a7FMqc

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
	if (in < 10.0 && in > 1.0) return std::to_string(in).substr(0,accuracy + 2);
	int digits = 0;
	if(in > 10.0) while (in > 10.0) { in /= 10; digits++; }
	else if(in < 1.0) while (in < 1.0) { in *= 10; digits--; }
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
		olc::vd2d pos{}, vel{}, acc{}, jerk{};

		ObjPosInfo() = default;

		ObjPosInfo(PointObject* copy)
			: name{ copy->getName() }, pos{ copy->getPos() }, vel{ copy->getVel() }, acc{}, jerk{} {}

		ObjPosInfo(std::string objName, olc::vd2d objPos, olc::vd2d objVel, olc::vd2d objAcc, olc::vd2d objJerk)
			: name{ objName }, pos{ objPos }, vel{ objVel }, acc{ objAcc }, jerk{ objJerk } {}
	};

	class PointObject {
		std::string name;
		olc::vd2d pos, vel, acc;
		ObjPosInfo buff;
		double radius, mass;
		olc::Pixel color;
		bool calculate;

	public:
		PointObject(std::string objName, olc::vd2d position, olc::vd2d initVelocity, double objRadius, double objMass, olc::Pixel objColor = olc::WHITE, bool active = true)
			: name{ objName }, pos{ position }, vel{ initVelocity }, buff{ this }, radius{ objRadius }, mass{ objMass }, color{ objColor }, calculate{active} {}

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

		olc::vd2d getInstantAttraction(std::vector<PointObject*>& objs)
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

			return sumAcc;
		}

		void initAttraction(std::vector<PointObject*>& objs)
		{
			buff.acc = getInstantAttraction(objs);
			update(TICK, buff);
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
		double getMass() { return mass; }
		olc::vd2d getVel() { return vel; }
		void setVel(olc::vd2d in) { vel = in; buff = ObjPosInfo(this); }
		double getMomentum() { return sqrt((vel.x * vel.x) + (vel.y * vel.y)) * mass; }
		double getKeneticEnergy() { return 0.5 * mass * vel.mag() * vel.mag(); }
	};

	double bignum(double base, int power)
	{
		return base * pow(10.0, power);
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

	double calcOrbitUg(std::vector<PointObject*>& system, olc::vd2d systemCOM)
	{
		double totalNumerator = G, totalDenominator{0};
		for (auto* obj : system) { totalNumerator *= obj->getMass(); }
		for (auto* obj : system) { totalDenominator += calcRadius(obj, systemCOM); }
		return totalNumerator / totalDenominator;
	}

	double calcOrbitKe(std::vector<PointObject*>& system, olc::vd2d systemCOM)
	{
		double totalNumerator = G, totalDenominator{0};
		for (auto* obj : system) { totalNumerator *= obj->getMass(); }
		for (auto* obj : system) { totalDenominator += (calcRadius(obj, systemCOM) * 2); }
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
	olc::vi2d camOffset;
	float zoom = 0.0005;
	double initMomentum, initKe, initUg;

	bool OnUserCreate() override // 405400 * 1000
	{
		//objs.push_back(new PointObject("Earth1", { 12740, 0 }, {}, 6370, bignum(5.97, 22), olc::BLUE));
		//objs.push_back(new PointObject("Earth2", { -12740, 0 }, {}, 6370, bignum(5.97, 22), olc::GREY));

		//objs.push_back(new PointObject("Earth", { 0, 0 }, {}, 6370, bignum(5.97, 22), olc::BLUE));
		//objs.push_back(new PointObject("Moon ", { 25480, 0 }, {}, 1738, bignum(7.34, 20), olc::GREY)); // Apogee = 405400km, Perigee = 362600km

		objs.push_back(new PointObject("Earth ", { 0, 0 }, {0, -288.0846}, 6370, bignum(5.97, 22), olc::BLUE));
		//objs.push_back(new PointObject("Earth2", { 25480, 0 }, {}, 6370, bignum(5.97, 22), olc::BLUE));
		objs.push_back(new PointObject("Moon1 ", { 25480.0 * 2, 0 }, {0, 8462.9707}, 1738, bignum(7.34, 20), olc::GREY)); // Apogee = 405400km, Perigee = 362600km
		objs.push_back(new PointObject("Moon2 ", { 25480.0 * 12, 0 }, {0, 3584.5741}, 1738, bignum(7.34, 20), olc::GREY)); // Apogee = 405400km, Perigee = 362600km

		//setOrbitalSpeed(objs[0], objs);

		for (auto* obj : objs)
			obj->initAttraction(objs);

		for (auto* obj : objs)
			initMomentum = obj->getMomentum();
		
		olc::vd2d COM = calcCOM(objs);

		//std::cout << calcOrbitKe(objs, COM) << " " << calcVelocityKe(objs) << "\n";

		initUg = calcOrbitUg(objs, COM);
		initKe = calcVelocityKe(objs);//(G * objs[0]->getMass() * objs[1]->getMass()) / (25480.0 * 2.0);

		//std::cout << "Ke1: " << initKe << " Ke2: " << (.5 * objs[1]->getMass() * 12505.171 * 12505.171) << "\n";

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

		fElapsedTime *= 100;

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
			obj->setToBuff();

		camOffset = -calcCOM(objs) * zoom;

		for (auto* obj : objs)
		{
			obj->drawSelf(this, camOffset, zoom);
			totalMomentum += obj->getMomentum();
		}

		olc::vd2d COM = calcCOM(objs);
		olc::vd2d COMv = calcCOMVelocity(objs);

		double	R1 = calcRadius(objs[0], COM),
				R2 = calcRadius(objs[1], COM);

		FillCircle(((COM * zoom) + camOffset + SCREEN_SIZE / 2), 1, olc::YELLOW);
		//DrawCircle(((COM * zoom) + camOffset + SCREEN_SIZE / 2), (25480 * 2 * zoom));
		//DrawCircle(((COM * zoom) + camOffset + SCREEN_SIZE / 2), (25480 * 12 * zoom));

		totalUg = calcOrbitUg(objs, COM);
		totalKe = calcVelocityKe(objs) + calcCOMKe(objs, COMv);//(G * objs[0]->getMass() * objs[1]->getMass()) / (25480.0 * 2.0);

		//DrawStringDecal({ 0,0 * 2 }, objs[0]->getInfo(), olc::WHITE, olc::vf2d{ 0.25f, 1 } * 2);
		//DrawStringDecal({ 0,8 * 2 }, objs[1]->getInfo(), olc::WHITE, olc::vf2d{ 0.25f, 1 } * 2);

		//double momentumDeviationPercent = ((totalMomentum - initMomentum) / initMomentum) * 100;
		//DrawStringDecal({ 0,16 }, ("Momenum deviation: " + std::to_string(momentumDeviationPercent)) + "%", olc::WHITE, { 0.25f, 1 });
		DrawStringDecal({ 0,24 * 2 }, ("Ke: " + to_sci(totalKe, 4)) + "(N)", olc::WHITE, olc::vf2d{ 0.25f, 1 } * 2);
		DrawStringDecal({ 0,32 * 2 }, ("Ug: " + to_sci(totalUg, 4)) + "(N)", olc::WHITE, olc::vf2d{ 0.25f, 1 } * 2);
		DrawStringDecal({ 0,40 * 2 }, ("Es: " + to_sci(totalKe + totalUg, 4) + "(N) Diff: " + std::to_string((totalKe - initKe + totalUg - initUg) / (initKe + initUg) * 100) + "%"), olc::WHITE, olc::vf2d{ 0.25f, 1 } * 2);
		DrawStringDecal({ 0,48 * 2 }, "COM: " + COM.strCut(5), olc::WHITE, olc::vf2d{ 0.25f, 1 } *2);
		DrawStringDecal({ 0,56 * 2 }, "COMv: " + COMv.strCut(5), olc::WHITE, olc::vf2d{ 0.25f, 1 } *2);
		DrawStringDecal({ 0,64 * 2 }, "R1: " + std::to_string(R1) + " R2: " + std::to_string(R2), olc::WHITE, olc::vf2d{ 0.25f, 1 } *2);

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