#include "System.h"
#include "Utility.h"

ECS::QuadTreeSystem::QuadTreeSystem()
: ECS::System(0)
, duplicationCheck(true)
, showGrid(true)
, collisionResolve(true)
, collisionChecksCount(0)
, collisionCheckWithOutRepeatCount(0)
, lastTrackingEntityID(ECS::INVALID_E_ID)
{}

void ECS::QuadTreeSystem::initQuadTree(cocos2d::Rect& boundary)
{
	// Init quadtree with initial boundary
	this->quadTree = std::unique_ptr<QuadTree>(new QuadTree(boundary, 0));
}

void ECS::QuadTreeSystem::update(const float delta, std::vector<ECS::Entity*>& entities)
{
	this->rebuildQuadTree(entities);
	this->updateEntityPosition(delta, entities);
	this->checkCollision(entities);
}

void ECS::QuadTreeSystem::rebuildQuadTree(std::vector<ECS::Entity*>& entities)
{
	auto m = ECS::Manager::getInstance();
	// Clear quad tree
	this->quadTree->clear();

	for (auto entity : entities)
	{
		// Re-insert to quadtree
		auto spriteComp = m->getComponent<ECS::Sprite>(entity);
		this->quadTree->insert(entity, spriteComp->sprite->getBoundingBox());
	}
}

void ECS::QuadTreeSystem::updateEntityPosition(const float delta, std::vector<ECS::Entity*>& entities)
{
	auto m = ECS::Manager::getInstance();

	for (auto entity : entities)
	{
		// Get entity's component
		auto spriteComp = m->getComponent<ECS::Sprite>(entity);
		auto qTreeObjComp = m->getComponent<ECS::QTreeData>(entity);
		auto dirVecComp = m->getComponent<ECS::DirectionVector>(entity);

		// Update new position based on direction, speed and time
		cocos2d::Vec2 movedDistance = dirVecComp->dirVec * qTreeObjComp->speed * delta;
		auto newPos = spriteComp->sprite->getPosition() + movedDistance;
		spriteComp->sprite->setPosition(newPos);

		// Check if entity is still in boundary

		bool inBoundary = Utility::containsRect(this->displayBoundary, spriteComp->sprite->getBoundingBox());
		if (!inBoundary)
		{
			// out of boundary
			bool flipX = false;
			bool flipY = false;
			checkBoundary(*spriteComp, flipX, flipY);

			if (flipX || flipY)
			{
				flipDirVec(flipX, flipY, dirVecComp->dirVec);
			}
		}

		if (duplicationCheck)
		{
			// Reset look up table to 0 if duplication check is enabled
			qTreeObjComp->visitied.clear();
		}

		// Reset color to white
		spriteComp->sprite->setColor(cocos2d::Color3B::WHITE);
	}

	this->drawQuadTreelines();
}

void ECS::QuadTreeSystem::checkBoundary(ECS::Sprite& spriteComp, bool& flipX, bool& flipY)
{
	// Get boundary
	const auto bb = spriteComp.sprite->getBoundingBox();

	if (bb.getMinX() < displayBoundary.getMinX())
	{
		// out left. push entity back to boundary
		float diff = displayBoundary.getMinX() - bb.getMinX();
		auto curPos = spriteComp.sprite->getPosition();
		curPos.x += diff + 0.1f;
		spriteComp.sprite->setPosition(curPos);
		flipX = true;
	}
	else if (bb.getMaxX() > displayBoundary.getMaxX())
	{
		// out right. push entity back to boundary
		float diff = bb.getMaxX() - displayBoundary.getMaxX();
		auto curPos = spriteComp.sprite->getPosition();
		curPos.x -= diff + 0.1f;
		spriteComp.sprite->setPosition(curPos);
		flipX = true;
	}

	if (bb.getMinY() < displayBoundary.getMinY())
	{
		// out bottom.
		float diff = displayBoundary.getMinY() - bb.getMinY();
		auto curPos = spriteComp.sprite->getPosition();
		curPos.y += diff + 0.1f;
		spriteComp.sprite->setPosition(curPos);
		flipY = true;
	}
	else if (bb.getMaxY() > displayBoundary.getMaxY())
	{
		// out right. push entity back to boundary
		float diff = bb.getMaxY() - displayBoundary.getMaxY();
		auto curPos = spriteComp.sprite->getPosition();
		curPos.y -= diff + 0.1f;
		spriteComp.sprite->setPosition(curPos);
		flipY = true;
	}

}

void ECS::QuadTreeSystem::flipDirVec(const bool flipX, const bool flipY, cocos2d::Vec2& dirVec)
{
	if (flipX)
	{
		// Flip x direction
		dirVec.x *= -1.0f;
		if (!flipY)
		{
			// For y, make it random for fun
			dirVec.y *= Utility::Random::random_minus_1_1();
		}
	}

	if (flipY)
	{
		dirVec.y *= -1.0f;
		if (!flipX)
		{
			// For x, make if random for fun
			dirVec.x *= Utility::Random::random_minus_1_1();
		}
	}
}

void ECS::QuadTreeSystem::checkCollision(std::vector<ECS::Entity*>& entities)
{
	// Reset counters
	this->collisionChecksCount = 0;
	this->collisionCheckWithOutRepeatCount = 0;

	auto m = ECS::Manager::getInstance();

	for (auto entity : entities)
	{
		// Get entity's component
		auto entitySpriteComp = m->getComponent<ECS::Sprite>(entity);
		auto entityQTreeObjectComp = m->getComponent<ECS::QTreeData>(entity);
		auto entityDirVecComp = m->getComponent<ECS::DirectionVector>(entity);

		// Get entity's bounding box
		auto bb = entitySpriteComp->sprite->getBoundingBox();

		// Query near entities
		std::list<Entity*> neighbors;
		this->quadTree->queryAllEntities(bb, neighbors);

		// Skip if there is no other entities nearby
		if (neighbors.empty())
		{
			continue;
		}

		// Iterate near entities
		for (auto nearEntity : neighbors)
		{
			// Get components
			auto nearEntityQTreeObjectComp = m->getComponent<ECS::QTreeData>(nearEntity);
			auto nearEntitySpriteComp = m->getComponent<ECS::Sprite>(nearEntity);
			auto nearEntityDirVecComp = m->getComponent<ECS::DirectionVector>(nearEntity);

			// Increment collision check count. 
			this->collisionChecksCount++;

			if (duplicationCheck)
			{
				// Duplication check enabled. Skip if comparing same entities
				if (entity->getId() != nearEntity->getId())
				{
					// Mark near entitiy as 'visitied'
					entityQTreeObjectComp->visitied.insert(nearEntity->getId());

					// See if near entitiy already checked collision with this entity
					auto find_it = nearEntityQTreeObjectComp->visitied.find(entity->getId());
					bool alreadyChecked = find_it != nearEntityQTreeObjectComp->visitied.end() ? true : false;

					if (!alreadyChecked)
					{
						// colliding entitiy havent visited entity yet.
						nearEntityQTreeObjectComp->visitied.insert(entity->getId());

						// Check collision
						if (entitySpriteComp->sprite->getBoundingBox().intersectsRect(nearEntitySpriteComp->sprite->getBoundingBox()))
						{
							// Color colliding entity with red
							entitySpriteComp->sprite->setColor(cocos2d::Color3B::RED);
							nearEntitySpriteComp->sprite->setColor(cocos2d::Color3B::RED);

							if (collisionResolve)
							{
								// Resolve collision
								resolveCollisions(*entitySpriteComp, *nearEntitySpriteComp, *entityDirVecComp, *nearEntityDirVecComp);
							}
						}
						// Increment counter.
						this->collisionCheckWithOutRepeatCount++;
					}
					//else, near entity already checked collision with this entity
				}
				//else, comparing same entities.
			}
			else
			{
				// Doesn't check duplication, skip if comparing same entity
				if (entity->getId() != nearEntity->getId())
				{
					// Get both bounding box
					auto eBB = entitySpriteComp->sprite->getBoundingBox();
					auto nBB = nearEntitySpriteComp->sprite->getBoundingBox();

					if (eBB.intersectsRect(nBB))
					{
						// Color colliding entity with red
						entitySpriteComp->sprite->setColor(cocos2d::Color3B::RED);
						nearEntitySpriteComp->sprite->setColor(cocos2d::Color3B::RED);

						if (collisionResolve)
						{
							// Resolve collision
							resolveCollisions(*entitySpriteComp, *nearEntitySpriteComp, *entityDirVecComp, *nearEntityDirVecComp);
						}
					}
				}
			}

			if (entityQTreeObjectComp->tracking)
			{
				// Mark near entity color with green if entity is tracking
				nearEntitySpriteComp->sprite->setColor(cocos2d::Color3B::GREEN);
			}

		}

		if (entityQTreeObjectComp->tracking)
		{
			// if entity is tracking, mark color with blue
			entitySpriteComp->sprite->setColor(cocos2d::Color3B::BLUE);
		}
	}
}

void ECS::QuadTreeSystem::resolveCollisions(ECS::Sprite & entitySpriteComp, ECS::Sprite & nearEntitySpriteComp, ECS::DirectionVector& entityDirVecComp, ECS::DirectionVector& nearEntityDirVecComp)
{
	auto eBB = entitySpriteComp.sprite->getBoundingBox();
	auto nBB = nearEntitySpriteComp.sprite->getBoundingBox();

	auto bb = Utility::getIntersectingRect(eBB, nBB);

	auto ePos = entitySpriteComp.sprite->getPosition();
	auto nPos = nearEntitySpriteComp.sprite->getPosition();

	bool flipX = false;
	bool flipY = false;

	if (bb.size.width < bb.size.height)
	{
		// hit from left and right
		float halfWidth = bb.size.width * 0.5f + 0.1f;

		if (eBB.getMidX() < nBB.getMidX())
		{
			// entity is on left and near entity is on right
			ePos.x -= halfWidth;
			nPos.x += halfWidth;
		}
		else
		{
			ePos.x += halfWidth;
			nPos.x -= halfWidth;
		}

		flipX = true;
		flipY = Utility::Random::randomInt100() > 50 ? true : false;
	}
	else if (bb.size.width > bb.size.height)
	{
		// hit from top and bottom
		float halfHeight = bb.size.height * 0.5f + 0.1f;
		if (eBB.getMidY() < nBB.getMidY())
		{
			// entity is lower than near entity
			ePos.x -= halfHeight;
			nPos.x += halfHeight;
		}
		else
		{
			ePos.x += halfHeight;
			nPos.x -= halfHeight;
		}

		flipX = Utility::Random::randomInt100() > 50 ? true : false;
		flipY = true;
	}
	// Else, Diagonally hit. Happens really rarely. just ignore.
	entitySpriteComp.sprite->setPosition(ePos);
	nearEntitySpriteComp.sprite->setPosition(nPos);

	flipDirVec(flipX, flipY, entityDirVecComp.dirVec);
	flipDirVec(flipX, flipY, nearEntityDirVecComp.dirVec);
}

void ECS::QuadTreeSystem::drawQuadTreelines()
{
	QuadTree::lineDrawNode->clear();
	if (showGrid)
	{
		// Show grids
		this->quadTree->showLines();
	}
}

bool ECS::QuadTreeSystem::updateMouseDown(const int mouseButton, const cocos2d::Vec2& point)
{		
	// In display boundary
	auto bb = cocos2d::Rect();
	bb.origin = cocos2d::Vec2(point.x - 5.0f, point.y - 5.0f);
	bb.size = cocos2d::Vec2(10.0f, 10.0f);

	std::list<Entity*> nearEntities;

	// Query near entities
	this->quadTree->queryAllEntities(bb, nearEntities);

	if (!nearEntities.empty())
	{
		for (auto entity : nearEntities)
		{
			auto entitySpriteComp = entity->getComponent<ECS::Sprite>();

			if (entitySpriteComp->sprite->getBoundingBox().containsPoint(point))
			{
				if (mouseButton == 0)
				{
					auto entityQTreeObjectComp = entity->getComponent<QTreeData>();

					if (this->lastTrackingEntityID == entity->getId())
					{
						// Already tracking same entity. Disable tracking.
						entityQTreeObjectComp->tracking = false;
						this->lastTrackingEntityID = ECS::INVALID_E_ID;

						for (auto e : nearEntities)
						{
							auto spriteComp = e->getComponent<ECS::Sprite>();
							spriteComp->sprite->setColor(cocos2d::Color3B::WHITE);
						}

						return true;
					}

					// New entity to track
					entityQTreeObjectComp->tracking = true;

					if (this->lastTrackingEntityID != ECS::INVALID_E_ID)
					{
						// There is entity we are tracking already. Disable it.
						ECS::Entity* prevTrackingEntity = ECS::Manager::getInstance()->getEntityById(this->lastTrackingEntityID);
						prevTrackingEntity->getComponent<ECS::QTreeData>()->tracking = false;
						prevTrackingEntity->getComponent<ECS::Sprite>()->sprite->setColor(cocos2d::Color3B::WHITE);

						std::vector<ECS::Entity*> entities;
						ECS::Manager::getInstance()->getAllEntitiesInPool(entities, "QT");
						for (auto e : entities)
						{
							auto spriteComp = e->getComponent<ECS::Sprite>();
							spriteComp->sprite->setColor(cocos2d::Color3B::WHITE);
						}
					}

					this->lastTrackingEntityID = entity->getId();

					for (auto e : nearEntities)
					{
						auto spriteComp = e->getComponent<ECS::Sprite>();
						spriteComp->sprite->setColor(cocos2d::Color3B::GREEN);
					}

					entitySpriteComp->sprite->setColor(cocos2d::Color3B::BLUE);

					return true;
				}
				else if (mouseButton == 1)
				{
					auto dataComp = entity->getComponent<ECS::QTreeData>();
					if (dataComp->tracking)
					{
						this->lastTrackingEntityID = ECS::INVALID_E_ID;


						for (auto e : nearEntities)
						{
							auto spriteComp = e->getComponent<ECS::Sprite>();
							spriteComp->sprite->setColor(cocos2d::Color3B::WHITE);
						}
					}
					entity->kill();
					return true;
				}
			}
		}
	}

	return false;
}








ECS::FlockingSystem::FlockingSystem()
: ECS::System(0)
, smoothSteering(true)
, lastTrackingBoidId(ECS::INVALID_E_ID)
{}

void ECS::FlockingSystem::update(const float delta, std::vector<ECS::Entity*>& entities)
{
	this->rebuildQuadTree(entities);
	this->updateFlockingAlgorithm(delta, entities);
}

void ECS::FlockingSystem::initQuadTree(cocos2d::Rect& boundary)
{
	// Init quadtree with initial boundary
	this->quadTree = std::unique_ptr<QuadTree>(new QuadTree(boundary, 0));
}

void ECS::FlockingSystem::rebuildQuadTree(std::vector<ECS::Entity*>& entities)
{
	auto m = ECS::Manager::getInstance();
	// Clear quad tree
	this->quadTree->clear();

	for (auto entity : entities)
	{
		// Re-insert to quadtree
		auto spriteComp = m->getComponent<ECS::Sprite>(entity);
		this->quadTree->insert(entity, spriteComp->sprite->getBoundingBox());
	}
}

void ECS::FlockingSystem::updateFlockingAlgorithm(const float delta, std::vector<ECS::Entity*>& entities)
{
	auto m = ECS::Manager::getInstance();

	for (auto entity : entities)
	{
		auto spriteComp = m->getComponent<ECS::Sprite>(entity);
		spriteComp->sprite->setColor(cocos2d::Color3B::WHITE);
	}

	// iterate entities
	for (auto entity : entities)
	{
		auto entityFlockingObjComp = entity->getComponent<ECS::FlockingData>();
		auto entitySpriteComp = entity->getComponent<ECS::Sprite>();
		if (entityFlockingObjComp->type == ECS::FlockingData::TYPE::BOID)
		{
			// If entity is boid, update flocking algorithm
			std::list<Entity*> nearEntities;

			// Create query rect and query near entities
			cocos2d::Rect queryingArea = cocos2d::Rect::ZERO;
			float pad = ECS::FlockingData::SIGHT_RADIUS;
			queryingArea.origin = (entitySpriteComp->sprite->getPosition() - cocos2d::Vec2(pad, pad));
			queryingArea.size = cocos2d::Vec2(pad * 2.0f, pad * 2.0f);

			this->quadTree->queryAllEntities(queryingArea, nearEntities);

			std::list<Entity*> nearBoids;
			std::list<Entity*> nearAvoids;

			// Iterate near entieis
			for (auto nearEntity : nearEntities)
			{
				auto nearEntityFlockingObjComp = nearEntity->getComponent<ECS::FlockingData>();
				auto nearBoidSpriteComp = nearEntity->getComponent<ECS::Sprite>();
				auto entityPos = entitySpriteComp->sprite->getPosition();
				auto nearBoidPos = nearBoidSpriteComp->sprite->getPosition();
				float distance = nearBoidPos.distance(entityPos);
				if (nearEntityFlockingObjComp->type == ECS::FlockingData::TYPE::BOID)
				{
					// If near entity is boid, check distance
					if (distance <= ECS::FlockingData::SIGHT_RADIUS)
					{
						// Add near entity as near boid
						nearBoids.push_back(nearEntity);
						if (entityFlockingObjComp->tracking)
						{
							nearBoidSpriteComp->sprite->setColor(cocos2d::Color3B::GREEN);
						}
					}
				}
				else if (nearEntityFlockingObjComp->type == ECS::FlockingData::TYPE::OBSTACLE)
				{
					// If near entity is obstacle, check distance
					if (distance <= ECS::FlockingData::AVOID_RADIUS)
					{
						// Add near entity as near obstacle
						nearAvoids.push_back(nearEntity);
					}
				}
			}

			// Update direction vector
			auto entityDirVecComp = entity->getComponent<ECS::DirectionVector>();

			cocos2d::Vec2 finalVec = cocos2d::Vec2::ZERO;
			cocos2d::Vec2 avoidVec = cocos2d::Vec2::ZERO;
			if (!nearAvoids.empty())
			{
				// Apply avoid direction
				avoidVec = this->getAvoid(entity, nearAvoids);
				finalVec += (avoidVec * ECS::FlockingData::AVOID_WEIGHT);
			}

			if (!nearBoids.empty())
			{
				// Apply core 3 steer behavior.
				cocos2d::Vec2 alignmentVec = this->getAlignment(entity, nearBoids) * ECS::FlockingData::ALIGNMENT_WEIGHT;
				cocos2d::Vec2 cohesionVec = this->getCohesion(entity, nearBoids) * ECS::FlockingData::COHENSION_WEIGHT;
				cocos2d::Vec2 separationVec = this->getSeparation(entity, nearBoids) * ECS::FlockingData::SEPARATION_WEIGHT;
				finalVec += (alignmentVec + cohesionVec + separationVec);
			}

			// normalize and save
			finalVec.normalize();

			if (entityDirVecComp->smoothSteer)
			{
				// Steer boid's direction smooothly
				auto diffVec = finalVec - entityDirVecComp->dirVec;
				diffVec *= (delta * ECS::FlockingData::steerSpeed);
				entityDirVecComp->dirVec += diffVec;
			}
			else
			{
				// Steer instantly
				entityDirVecComp->dirVec = finalVec;
			}

			entityDirVecComp->dirVec.normalize();

			// update position
			auto movedDir = entityDirVecComp->dirVec * delta * ECS::FlockingData::movementSpeed;
			auto newPos = entitySpriteComp->sprite->getPosition() + movedDir;
			entitySpriteComp->sprite->setPosition(newPos);

			float angle = entityDirVecComp->getAngle();
			entitySpriteComp->sprite->setRotation(-angle);

			if (!this->displayBoundary.containsPoint(newPos))
			{
				// wrap position if boid is out of boundary
				entitySpriteComp->wrapPositionWithInBoundary(this->displayBoundary);
			}

			if (entityFlockingObjComp->tracking)
			{
				entitySpriteComp->sprite->setColor(cocos2d::Color3B::BLUE);
			}
		}
		else
		{
			entitySpriteComp->sprite->setColor(cocos2d::Color3B::RED);
		}
	}
}

const cocos2d::Vec2 ECS::FlockingSystem::getAlignment(ECS::Entity* boid, std::list<ECS::Entity*>& nearBoids)
{
	cocos2d::Vec2 sumDirVec = cocos2d::Vec2::ZERO;

	const cocos2d::Vec2 boidPos = boid->getComponent<ECS::Sprite>()->sprite->getPosition();

	for (auto nearBoid : nearBoids)
	{
		auto dirVecComp = nearBoid->getComponent<ECS::DirectionVector>();
		sumDirVec += dirVecComp->dirVec;
	}

	const float countF = static_cast<float>(nearBoids.size());
	sumDirVec.x /= countF;
	sumDirVec.y /= countF;
	sumDirVec.normalize();
	return sumDirVec;
}

const cocos2d::Vec2 ECS::FlockingSystem::getCohesion(ECS::Entity* boid, std::list<ECS::Entity*>& nearBoids)
{
	cocos2d::Vec2 sumPosVec = cocos2d::Vec2::ZERO;

	const cocos2d::Vec2 boidPos = boid->getComponent<ECS::Sprite>()->sprite->getPosition();

	for (auto nearBoid : nearBoids)
	{
		auto nearBoidSpriteComp = nearBoid->getComponent<ECS::Sprite>();
		sumPosVec += nearBoidSpriteComp->sprite->getPosition();
	}

	const float countF = static_cast<float>(nearBoids.size());
	sumPosVec.x /= countF;
	sumPosVec.y /= countF;

	cocos2d::Vec2 cohesionVec = sumPosVec - boidPos;

	cohesionVec.normalize();

	return cohesionVec;
}

const cocos2d::Vec2 ECS::FlockingSystem::getSeparation(ECS::Entity* boid, std::list<ECS::Entity*>& nearBoids)
{
	cocos2d::Vec2 sumDistVec = cocos2d::Vec2::ZERO;

	const cocos2d::Vec2 boidPos = boid->getComponent<ECS::Sprite>()->sprite->getPosition();

	for (auto nearBoid : nearBoids)
	{
		auto nearBoidSpriteComp = nearBoid->getComponent<ECS::Sprite>();
		auto nearBoidPos = nearBoidSpriteComp->sprite->getPosition();
		auto distVec = nearBoidPos - boidPos;
		float distance = nearBoidPos.distance(boidPos);
		if (distance <= 0)
		{
			sumDistVec += distVec;
		}
		else
		{
			sumDistVec += (distVec * (1.0f / distance));
		}
	}

	const float countF = static_cast<float>(nearBoids.size());
	sumDistVec.x /= countF;
	sumDistVec.y /= countF;

	sumDistVec *= -1;

	sumDistVec.normalize();

	return sumDistVec;
}

const cocos2d::Vec2 ECS::FlockingSystem::getAvoid(ECS::Entity* boid, std::list<ECS::Entity*>& nearAvoids)
{
	int count = 0;
	cocos2d::Vec2 sumDistVec = cocos2d::Vec2::ZERO;

	const cocos2d::Vec2 boidPos = boid->getComponent<ECS::Sprite>()->sprite->getPosition();

	for (auto nearAvoid : nearAvoids)
	{
		auto nearAvoidSpriteComp = nearAvoid->getComponent<ECS::Sprite>();
		auto nearAvoidPos = nearAvoidSpriteComp->sprite->getPosition();
		cocos2d::Vec2 distVec = boidPos - nearAvoidPos;
		sumDistVec += distVec;
		count++;
	}

	if (count > 0)
	{
		const float countF = static_cast<float>(count);
		sumDistVec.x /= countF;
		sumDistVec.y /= countF;
		sumDistVec.normalize();
	}

	return sumDistVec;
}

bool ECS::FlockingSystem::updateMouseDown(const int mouseButton, cocos2d::Vec2 & point)
{
	if (mouseButton == 0)
	{
		std::vector<ECS::Entity*> entities;
		ECS::Manager::getInstance()->getAllEntitiesInPool(entities, "FB");

		for (auto entity : entities)
		{
			auto spriteComp = entity->getComponent<ECS::Sprite>();
			if (spriteComp->sprite->getBoundingBox().containsPoint(point))
			{
				auto entityFlockingObjComp = entity->getComponent<ECS::FlockingData>();

				// Clicked boid sprite
				if (entity->getId() == this->lastTrackingBoidId)
				{
					// Already tracking this boid. Disble tracking
					entityFlockingObjComp->tracking = false;
					this->lastTrackingBoidId = -1;						
					
					for (auto e : entities)
					{
						auto spriteComp = e->getComponent<ECS::Sprite>();
						spriteComp->sprite->setColor(cocos2d::Color3B::WHITE);
					}

					return true;
				}

				// New entity to track
				entityFlockingObjComp->tracking = true;

				if (this->lastTrackingBoidId != ECS::INVALID_E_ID)
				{
					ECS::Entity* prevTrackingEntity = ECS::Manager::getInstance()->getEntityById(this->lastTrackingBoidId);
					prevTrackingEntity->getComponent<ECS::FlockingData>()->tracking = false;
					prevTrackingEntity->getComponent<ECS::Sprite>()->sprite->setColor(cocos2d::Color3B::WHITE);

					for (auto e : entities)
					{
						auto spriteComp = e->getComponent<ECS::Sprite>();
						spriteComp->sprite->setColor(cocos2d::Color3B::WHITE);
					}
				}

				// Set this entity to new tracking entity
				this->lastTrackingBoidId = entity->getId();

				// Create query rect and query near entities
				cocos2d::Rect queryingArea = cocos2d::Rect::ZERO;
				float pad = ECS::FlockingData::SIGHT_RADIUS;
				queryingArea.origin = (spriteComp->sprite->getPosition() - cocos2d::Vec2(pad, pad));
				queryingArea.size = cocos2d::Vec2(pad * 2.0f, pad * 2.0f);

				std::list<Entity*> nearEntities;
				this->quadTree->queryAllEntities(queryingArea, nearEntities);

				for (auto ne : nearEntities)
				{
					auto dataComp = ne->getComponent<ECS::FlockingData>();
					if (dataComp->type == ECS::FlockingData::TYPE::BOID)
					{
						auto neSpriteComp = ne->getComponent<ECS::Sprite>();
						if (neSpriteComp->sprite->getPosition().distance(spriteComp->sprite->getPosition()) <= ECS::FlockingData::SIGHT_RADIUS)
						{
							ne->getComponent<ECS::Sprite>()->sprite->setColor(cocos2d::Color3B::GREEN);
						}
					}
				}

				spriteComp->sprite->setColor(cocos2d::Color3B::BLUE);

				return true;
			}
		}

		return false;
	}
	else if (mouseButton == 1)
	{
		std::vector<ECS::Entity*> entities;
		ECS::Manager::getInstance()->getAllEntitiesInPool(entities, "FB");

		for (auto entity : entities)
		{
			auto spriteComp = entity->getComponent<ECS::Sprite>();
			if (spriteComp->sprite->getBoundingBox().containsPoint(point))
			{
				auto flockingObjComp = entity->getComponent<ECS::FlockingData>();
				if (flockingObjComp->tracking)
				{
					this->lastTrackingBoidId = ECS::INVALID_E_ID;
				}

				for (auto e : entities)
				{
					e->getComponent<ECS::Sprite>()->sprite->setColor(cocos2d::Color3B::WHITE);
				}

				entity->kill();


				return true;
			}
		}

		return false;
	}
	else if (mouseButton == 2)
	{
		std::vector<ECS::Entity*> entities;
		ECS::Manager::getInstance()->getAllEntitiesInPool(entities, "FO");

		for (auto entity : entities)
		{
			auto spriteComp = entity->getComponent<ECS::Sprite>();
			if (spriteComp->sprite->getPosition().distance(point) < 6.0f)
			{
				entity->kill();
				return true;
			}
		}

		return false;
	}
}