#pragma once

#include "PhysicsBase.h"

// Physics components that focus on translational 2D physics
namespace tp {
	struct FreeBody : public phy::FreeBodyBase {

		std::map<int, ObjVecBase*> vectors;

	};

	struct Collider : public phy::ColliderBase {
		
	};
};