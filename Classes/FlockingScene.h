#ifndef FLOCKINGSCENE_H
#define FLOCKINGSCENE_H

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "QuadTree.h"
#include "ECS.h"
#include "CustomNode.h"
#include "Component.h"

class FlockingScene : public cocos2d::Scene
{
private:
	//default constructor
	FlockingScene() {};

	//Input Listeners
	cocos2d::EventListenerMouse* mouseInputListener;
	cocos2d::EventListenerKeyboard* keyInputListener;

	//Mouse events
	void onMouseMove(cocos2d::Event* event);
	void onMouseDown(cocos2d::Event* event);

	//keyboard events
	void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);

	//cocos2d virtuals
	virtual bool init() override;
	virtual void onEnter() override;
	virtual void update(float delta) override;
	virtual void onExit() override;

	void initInputListeners();
	void releaseInputListeners();

	// cocos2d
	cocos2d::Label* backLabel;
	cocos2d::Rect displayBoundary;
	cocos2d::Node* areaNode;
    cocos2d::Vec2 curMousePosition;
    DisplayBoundaryBoxNode* displayBoundaryBoxNode;
    LabelsNode* labelsNode;
    ButtonModifierNode* buttonModifierNode;
    
    // Enum for labels
    enum class CUSTOM_LABEL_INDEX
    {
        ENTITIES,
        WEIGHTS,
    };
    
	enum class USAGE_KEY
	{
        NONE,
		SPACE = 1,
		CLEAR,
		ADD_TEN,
		REMOVE_TEN,
        MAX_KEYBOARD_USAGE,
	};
    
    enum class USAGE_MOUSE_OVER_AND_KEY
    {
        NONE,
        ADD_OBSTACLE,
        REMOVE_OBSTACLE,
        MAX_MOUSE_HOVER_AND_KEY_USAGE,
    };
    
    enum class USAGE_MOUSE
    {
        NONE,
        ADD_ONE,
        TRACK,
        REMOVE_ONE,
        ADD_OBSTACLE,
        REMOVE_OBSTACLE,
        MAX_MOUSE_USAGE
    };
    
    enum class WEIGHT_INDEX
    {
        ALIGNMENT,
        COHESION,
        SEPARATION,
        AVOID,
    };

	enum class ACTION_TAG
	{
		ALIGNMENT_LEFT,
		ALIGNMENT_RIGHT,
		COHESION_LEFT,
		COHESION_RIGHT,
		SEPARATION_LEFT,
		SEPARATION_RIGHT,
		AVOID_LEFT,
		AVOID_RIGHT
	};
    
    enum Z_ORDER
    {
        BOID,
        OBSTACLE,
        BOX
    };

	// Quadtree to optimize collision check
	QuadTree* quadTree;

	// Flags
	bool pause;

	// Entities
	std::list<ECS::Entity*> entities;

	// RangeChecker
	cocos2d::Sprite* rangeChecker;

	// Tracking
	int lastTrackingBoidId;

	// Initialize entities and quad tree
	void initEntitiesAndQTree();

	// Creates new entity with required components
	ECS::Entity* createNewEntity();
	ECS::Entity* createNewEntity(const cocos2d::Vec2& pos);
	ECS::Entity* createNewObstacleEntity(const cocos2d::Vec2& pos);

	// Reset QuadTree and remove inactive entities
	void resetQTreeAndPurge();
	// Update flocking algorithm
	void updateFlockingAlgorithm(const float delta);

	// Flocking algorithm
	/**
	*	Get Alignment vector
	*	Iterate through near boids and checks distance.
	*	If distance between boid and near boids are close enough, 
	*	sum near boids' direction vector and get average then normalize
	*/
	const cocos2d::Vec2 getAlignment(Entity* boid, std::list<Entity*>& nearBoids);
	/**
	*	Get Cohesion
	*	Iterate thorugh near boids and checks distance.
	*	If distance between boid and near boids are close enough,
	*	sum near boids' position vector and get average then nomarlize.
	*	In this case, we want direction vector not position, so compute
	*	direction vector based on boid's position
	*/
	const cocos2d::Vec2 getCohesion(Entity* boid, std::list<Entity*>& nearBoids);
	/**
	*	Get Separation
	*	Iterate through near boids and checks distance
	*	If distance between boid and near boids are close enough,
	*	sum the distance between boid and near boids and get average, 
	*	negate the direction, then normalize. 
	*/
	const cocos2d::Vec2 getSeparation(Entity* boid, std::list<Entity*>& nearBoids);
	/**
	*	Get Avoid
	*	This isn't core flocking algorithm. Alignment, Cohesion and
	*	Separation are the core but this is a simple addition to
	*	algorithm to make boid avoid obstacles.
	*	It's similar to Separation, but weighted by distance
	*/
	const cocos2d::Vec2 getAvoid(Entity* boid, std::list<Entity*>& nearAvoids);
    
    // Button click call back
    void onButtonPressed(cocos2d::Ref* sender);
public:
	//simple creator func
	static FlockingScene* createScene();

	//default destructor
	~FlockingScene() {};

	//Cocos2d Macro
	CREATE_FUNC(FlockingScene);
};

#endif
