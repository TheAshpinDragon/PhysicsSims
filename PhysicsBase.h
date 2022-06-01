#pragma once

#include "olcPixelGameEngine.h"

namespace phy {

#define FLIP olc::vf2d(1, -1)
#define COMNULL olc::vd2d( std::numeric_limits<float>::max(), std::numeric_limits<float>::max() )

	namespace TYPES
	{
		const int RECT = 0;
		const int CIRC = 1;
	}

	enum class ShapeType
	{
		RECT = 0,
		CIRC = 1,
		POLY = 2
	};

	struct shape {
	public:
		std::vector<olc::vi2d> rawVerticies, adjustedVerticies;
		olc::Decal* shapeDecal, * outlineDecal;
		ShapeType type;

	private:
		void generateRectangle(olc::vf2d size)
		{

			//verticies.push_back();
		}

		void generatePolygon(float radius, int sides) {}

		void generateShapeDecal() {}

	public:
		/*
		REGULAR POLYGON AND CIRCLE CONSTRUCTOR
		 * radius: The distance to each point
		 * sides: Number of sides:
			~ 1 = Circle (default)
			~ 3 = Equallateral triangle
			~ 4 = Square
			~ 5+ = Regular pollygons
		 */
		shape(float radius, int sides = 1)
		{
			if (sides <= 0 || sides == 2) throw("ERROR: Invalid polygon sides");
			if (radius < 0.01f) throw("ERROR: Invalid polygon radious");

			generatePolygon(radius, sides);
		}

		/*
		RECTANGLE CONSTRUCTOR
		 * size: Rectangle dimensions
		*/
		shape(olc::vf2d size)
		{
			if (size.x < 0.01f || size.y < 0.01f) throw("ERROR: Invalid rectangle dimentions");

			generateRectangle(size);
		}

		void updateVerticies(float rotation)
		{

		}
	};

	struct ColliderBase;
	struct object;

	struct pos1d {
		float	pos = 0.0f;
		float	vel = 0.0f;
		float	acc = 0.0f;

		pos1d() = default;

		pos1d(float _pos, float _vel = 0.0f, float _acc = 0.0f)
			: pos{ _pos }, vel{ _vel }, acc{ _acc } {}

		void update(float fElapsedTime)
		{
			float	_vel = vel + acc * fElapsedTime;
			float	_pos = pos + vel * fElapsedTime + .5 * acc * fElapsedTime * fElapsedTime;

			vel = _vel;
			pos = _pos;
		}

		pos1d advance(float fTime)
		{
			float	_acc = acc;
			float	_vel = vel + acc * fTime;
			float	_pos = pos + vel * fTime + .5 * acc * fTime * fTime;

			return pos1d(_pos, _vel, _acc);
		}
		pos1d& operator= (const pos1d& rhs)
		{
			pos = rhs.pos;
			vel = rhs.vel;
			acc = rhs.acc;

			return *this;
		}

		pos1d  operator- (const pos1d& rhs) { return pos1d(pos - rhs.pos, vel - rhs.vel, acc - rhs.acc); }
		pos1d  operator+ (const pos1d& rhs) { return pos1d(pos + rhs.pos, vel + rhs.vel, acc + rhs.acc); }

		const std::string to_String() const
		{
			return
				"  POS: " + std::to_string(pos) + " m\n"
				"  VEL: " + std::to_string(vel) + " m/s\n"
				"  ACC: " + std::to_string(acc) + " m/s^2";
		}

		friend std::ostream& operator << (std::ostream& _stream, const pos1d& _out)
		{
			_stream << _out.to_String();
			return _stream;
		}
	};

	struct pos2d {
		olc::vf2d	pos = { 0.0f,0.0f };
		olc::vf2d	vel = { 0.0f,0.0f };
		olc::vf2d	acc = { 0.0f,0.0f };

		pos2d() = default;

		//pos2d(olc::vf2d _pos, olc::vf2d _vel = { 0.0f, 0.0f }, olc::vf2d _acc = { 0.0f, 0.0f })
		//	: pos(_pos), vel(_vel), acc(_acc) {}

		pos2d(olc::vf2d _pos, olc::vf2d _vel = { 0.0f, 0.0f }, olc::vf2d _acc = { 0.0f, 0.0f })
			: pos{ _pos }, vel{ _vel }, acc{ _acc } {}

		// = = = = FUNCTIONS = = = = //

		void set(pos2d set)
		{
			acc = set.acc;
			vel = set.vel;
			pos = set.pos;
		}

		void setX(pos2d set)
		{
			acc.x = set.acc.x;
			vel.x = set.vel.x;
			pos.x = set.pos.x;
		}

		void setY(pos2d set)
		{
			acc.y = set.acc.y;
			vel.y = set.vel.y;
			pos.y = set.pos.y;
		}

		// Update X & Y of self
		void update(float fElapsedTime)
		{
			olc::vf2d	_vel = vel + acc * fElapsedTime;
			olc::vf2d	_pos = pos + vel * fElapsedTime + .5 * acc * abs(fElapsedTime) * fElapsedTime;

			vel = _vel;
			pos = _pos;
		}

		// Update just X of self
		void updateX(float fElapsedTime)
		{
			float	_vel = vel.x + acc.x * fElapsedTime;
			float	_pos = pos.x + vel.x * fElapsedTime + .5 * acc.x * abs(fElapsedTime) * fElapsedTime;

			vel.x = _vel;
			pos.x = _pos;
		}

		// Update just Y of self
		void updateY(float fElapsedTime)
		{
			float	_vel = vel.y + acc.y * fElapsedTime;
			float	_pos = pos.y + vel.y * fElapsedTime + .5 * acc.y * abs(fElapsedTime) * fElapsedTime;

			vel.y = _vel;
			pos.y = _pos;
		}

		// Return current values advanced by fTime
		pos2d advance(float fTime)
		{
			olc::vf2d	_acc = acc;
			olc::vf2d	_vel = vel + acc * fTime;
			olc::vf2d	_pos = pos + vel * fTime + .5 * acc * abs(fTime) * fTime;

			return pos2d(_pos, _vel, _acc);
		}

		// Return current values, with Just Y advanced by fTime
		pos2d advanceX(float fTime)
		{
			olc::vf2d	_acc = acc;
			olc::vf2d	_vel = { vel.x + acc.x * fTime, vel.y };
			olc::vf2d	_pos = { pos.x + vel.x * fTime + .5f * acc.x * abs(fTime) * fTime, pos.y };

			return pos2d(_pos, _vel, _acc);
		}

		// Return current values, with Just X advanced by fTime
		pos2d advanceY(float fTime)
		{
			olc::vf2d	_acc = acc;
			olc::vf2d	_vel = { vel.x, vel.y + acc.y * fTime };
			olc::vf2d	_pos = { pos.x, pos.y + vel.y * fTime + .5f * acc.y * abs(fTime) * fTime };

			return pos2d(_pos, _vel, _acc);
		}

		pos2d flip() { return pos2d(pos * FLIP, vel * FLIP, acc * FLIP); }

		pos2d offsetPos(olc::vf2d _offset) { return pos2d(pos - _offset, vel, acc); }

		pos2d& operator= (const pos2d& rhs)
		{
			pos = rhs.pos;
			vel = rhs.vel;
			acc = rhs.acc;

			return *this;
		}
		pos2d  operator- (const pos2d& rhs) { return pos2d(pos - rhs.pos, vel - rhs.vel, acc - rhs.acc); }
		pos2d  operator+ (const pos2d& rhs) { return pos2d(pos + rhs.pos, vel + rhs.vel, acc + rhs.acc); }

		const std::string to_String() const
		{
			return
				"  POS: " + pos.str() + " m\n"
				"  VEL: " + vel.str() + " m/s\n"
				"  ACC: " + acc.str() + " m/s^2";
		}

		friend std::ostream& operator << (std::ostream& _stream, const pos2d& _out)
		{
			_stream << _out.to_String();
			return _stream;
		}
	};

	struct ColliderBase {
		int type;

		// = = = Dimentions/Vertifies = = =
		// std::vector<olc::vi2d> verts;
		olc::vf2d size;

		// = = = Position = = = 
		pos2d		localityBuff;
		pos2d		locality;
		bool		yLastColl;
		bool		xLastColl;

		// = = = Rotation = = = 
		pos1d		rotationBuff;
		pos1d		rotation;

		// = = = Properties = = = 
		float		mass;
		olc::vf2d	COM;

		ColliderBase() = default;
		ColliderBase(int _type = TYPES::RECT, olc::vf2d _size = { 20.0f, 20.0f }, olc::vf2d _pos = { 0.0f, 0.0f }, float _rot = 0.0f, float _mass = 1.0f, olc::vf2d _COM = COMNULL)
			: type(_type), size(_size), locality(_pos, { 0.0f, 0.0f }, { 0.0f, 0.0f }), rotation(_rot, 0.0f, 0.0f), mass(_mass), COM(_COM == COMNULL ? _size / 2.0f : _COM) {}

		virtual void update(float fElapsedTime)
		{
		}

		virtual struct collisionReport {
			ColliderBase& colA, & colB;
			olc::vb2d collidingBefore, collidingAfter;
			pos2d systemLocality;
		};

		olc::vf2d solveQuadratic(float a, float b, float c)
		{
			if (a <= 0.000001f && a >= -0.000001f)
				return { 0,0 };

			olc::vf2d roots = solveSquareRoot((b * b) - 4 * a * c);

			olc::vf2d out = { ((-b) + roots.x) / (2 * a), ((-b) + roots.y) / (2 * a) };

			if (isnan(out.x))
				out.x = 0.0f;

			if (isnan(out.y))
				out.y = 0.0f;

			return out;
		}

		olc::vf2d solveSquareRoot(float radicand)
		{
			// Shortcut
			if (abs(radicand) == 0.0f)
				return { 0, 0 };

			// Return +- square root of radicand
			return { sqrt(radicand), -sqrt(radicand) };
		}

	};

	struct ObjVecBase : olc::vf2d {
	private:
		typedef olc::vf2d super;

	public:

		/*
		olc::vf2d posFromCenter;
		float durration;
		ImpulseChart iChart;
		Unit magnitudeUnit;
		*/

		ObjVecBase(olc::vf2d components) : super(components) {}
	};

	struct FreeBodyBase {
		
		virtual void update() {
			ObjVecBase v1 = olc::vf2d{0,0};
		}

		virtual void addVec(int key) {}

		virtual void delVec(int key) {}
	};
};