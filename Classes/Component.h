#ifndef COMPONENT_H
#define COMPONENT_H

#include "cocos2d.h"
#include "ECS.h"

/**
*	Note
*
*	Since this project is just for visualization, I declared everything
*	in public field to make easier to access from outside. Not considering
*	any encapsulation or whatnot.
*/

namespace ECS
{
	enum COMPONENT_ID
	{
		DIRECTION_VECTOR = 0,
		SPRITE,
		QTREE_DATA,
		FLOCKING_DATA,
		CIRCLE_PACKING_DATA,
	};


	class DirectionVector : public Component
	{
	public:
		DirectionVector();
		~DirectionVector() = default;
		DirectionVector(DirectionVector const&) = delete;
		void operator=(DirectionVector const&) = delete;

		// Direction vector
		cocos2d::Vec2 dirVec;
		// Smooth steer
		bool smoothSteer;
		// Sets new direction vector
		void setNewDirVec();
		// Get angle from direction vector. 0 starts from right side (1, 0).
		const float getAngle();
	};

	class Sprite : public Component
	{
	public:
		Sprite(cocos2d::Node& parent, const std::string& spriteName);
		~Sprite();
		Sprite(Sprite const&) = delete;
		void operator=(Sprite const&) = delete;

		cocos2d::Sprite* sprite;

		// Rotate sprite. Wrap angle if it's less than 0 or greater than 360
		void rotateToDirVec(float angle);
		// Set position to random place in boundary
		void setRandomPosInBoundary(const cocos2d::Rect& boundary);
		// Wrap sprite's position to boundary
		void wrapPositionWithInBoundary(const cocos2d::Rect& boundary);
	};

	class QTreeData : public Component
	{
	public:
		QTreeData();
		~QTreeData();
		QTreeData(QTreeData const&) = delete;
		void operator=(QTreeData const&) = delete;

		std::vector<int> visitied;
		float speed;
		bool tracking;
	};

	class FlockingData : public Component
	{
	public:
		enum class TYPE
		{
			BOID,
			OBSTACLE
		};

	public:
		FlockingData(const TYPE type);
		~FlockingData() = default;
		FlockingData(FlockingData const&) = delete;
		void operator=(FlockingData const&) = delete;
		
		static float movementSpeed;
		static float steerSpeed;
		bool tracking;

		static float SIGHT_RADIUS;
		static float COHENSION_WEIGHT;
		static float ALIGNMENT_WEIGHT;
		static float SEPARATION_WEIGHT;
		static float AVOID_RADIUS;
		static float AVOID_WEIGHT;

		TYPE type;
	};

	class CirclePackingData : public Component
	{
	public:
		CirclePackingData(const cocos2d::Vec2& position, const float radius = CirclePackingData::initialRadius, const cocos2d::Color4F color = cocos2d::Color4F::WHITE);
		~CirclePackingData() = default;
		CirclePackingData(CirclePackingData const&) = delete;
		void operator=(CirclePackingData const&) = delete;

		cocos2d::Vec2 position;
		float radius;
		cocos2d::Color4F color;

		bool growing;
		bool alive;
		int id;

		static int idCounter;

		static float maxRadius;
		static float growthSpeed;
		static float initialRadius;

		void update(const float delta);
		void activate(const cocos2d::Vec2& position, const float radius, const cocos2d::Color4F color);
		void deactivate();
	};
}

#endif