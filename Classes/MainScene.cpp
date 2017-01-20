#include "MainScene.h"
#include "QTreeScene.h"
#include "FlockingScene.h"
#include "CirclePackingScene.h"
#include "Utility.h"

USING_NS_CC;

MainScene* MainScene::createScene()
{
	MainScene* newMainScene = MainScene::create();
	return newMainScene;
}

bool MainScene::init()
{
	if (!cocos2d::Scene::init())
	{
		return false;
	}
	
	auto ss = cocos2d::SpriteFrameCache::getInstance();
	ss->addSpriteFramesWithFile("spritesheets/spritesheet.plist");

	hoveringLableIndex = -1;

	Utility::Random::init();

	// Uncomment this to activate update(float) function
	//this->scheduleUpdate();

	std::string fontPath = "fonts/Rubik-Medium.ttf";

	auto titleLabel = cocos2d::Label::createWithTTF("Visualizations", fontPath, 50);
	winSize = cocos2d::Director::getInstance()->getVisibleSize();
	titleLabel->setPosition(cocos2d::Vec2(winSize.width * 0.5f, winSize.height - 50.0f));
	this->addChild(titleLabel);
	
	this->labels.push_back(cocos2d::Label::createWithTTF("Quad Tree", fontPath, 30));
	this->labels.push_back(cocos2d::Label::createWithTTF("Flocking", fontPath, 30));
	this->labels.push_back(cocos2d::Label::createWithTTF("Circle Packing", fontPath, 30));

	this->labels.push_back(cocos2d::Label::createWithTTF("EXIT(ESC)", fontPath, 30));
	
	int index = 0;
	cocos2d::Vec2 start = cocos2d::Vec2(winSize.width * 0.5f, winSize.height - 120.0f);
	for (auto label : this->labels)
	{
		cocos2d::Vec2 pos = start;
		pos.y -= (index * 40.0f);
		label->setPosition(pos);
		this->addChild(label);
		index++;
	}

	return true;
}

void MainScene::onEnter()
{
	cocos2d::Scene::onEnter();
	initInputListeners();
}

void MainScene::update(float delta)
{
	 
}

void MainScene::checkMouseOver(const cocos2d::Vec2 mousePos)
{
	int index = 0;
	for (auto label : this->labels)
	{
		if (label->getBoundingBox().containsPoint(mousePos))
		{
			if (this->hoveringLableIndex == -1)
			{
				// wasn't hovering any menu.
				this->hoveringLableIndex = index;
				label->setScale(1.2f);
				label->setString("> " + label->getString() + " <");
				return;
			}
			else if(this->hoveringLableIndex == index)
			{
				// Hovering same. return
				return;
			}
			else
			{
				// Was hovering something, and wasn't the same
				this->labels.at(this->hoveringLableIndex)->setScale(1.0f);
				std::string labelStr = this->labels.at(this->hoveringLableIndex)->getString();
				this->labels.at(this->hoveringLableIndex)->setString(labelStr.substr(2, labelStr.size() - 4));

				label->setScale(1.2f);
				hoveringLableIndex = index;
				label->setString("> " + label->getString() + " <");
				return;
			}
		}
		else
		{
			if (label->getScale() > 1.0f)
			{
				label->setScale(1.0f);
			}
		}
		index++;
	}

	// If reach here, mouse wasn't hovering any.
	if (this->hoveringLableIndex != -1)
	{
		std::string labelStr = this->labels.at(this->hoveringLableIndex)->getString();
		this->labels.at(this->hoveringLableIndex)->setString(labelStr.substr(2, labelStr.size() - 4));
		this->hoveringLableIndex = -1;
	}
}

void MainScene::initInputListeners()
{
	this->mouseInputListener = EventListenerMouse::create();
	this->mouseInputListener->onMouseMove = CC_CALLBACK_1(MainScene::onMouseMove, this);
	this->mouseInputListener->onMouseDown = CC_CALLBACK_1(MainScene::onMouseDown, this);
	this->mouseInputListener->onMouseUp = CC_CALLBACK_1(MainScene::onMouseUp, this);
	this->mouseInputListener->onMouseScroll = CC_CALLBACK_1(MainScene::onMouseScroll, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(this->mouseInputListener, this);

	this->keyInputListener = EventListenerKeyboard::create();
	this->keyInputListener->onKeyPressed = CC_CALLBACK_2(MainScene::onKeyPressed, this);
	this->keyInputListener->onKeyReleased = CC_CALLBACK_2(MainScene::onKeyReleased, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(this->keyInputListener, this);
}

void MainScene::onMouseMove(cocos2d::Event* event) 
{
	auto mouseEvent = static_cast<EventMouse*>(event);
	float x = mouseEvent->getCursorX();
	float y = mouseEvent->getCursorY();

	checkMouseOver(cocos2d::Vec2(x, y));
}

void MainScene::onMouseDown(cocos2d::Event* event) 
{
	auto mouseEvent = static_cast<EventMouse*>(event);
	//0 = left, 1 = right, 2 = middle
	int mouseButton = mouseEvent->getMouseButton();
	//float x = mouseEvent->getCursorX();
	//float y = mouseEvent->getCursorY();
	if (hoveringLableIndex == -1)
	{
		return;
	}
	else
	{
		if (mouseButton == 0)
		{
			switch (this->hoveringLableIndex)
			{
			case LABEL_INDEX::QUAD_TREE:
			{
				// Load quad tree scene
				cocos2d::Director::getInstance()->replaceScene(cocos2d::TransitionFade::create(0.5f, QTreeScene::create(), cocos2d::Color3B::BLACK));
			}
			break;
			case LABEL_INDEX::FLOCKING:
			{
				// Load quad tree scene
				cocos2d::Director::getInstance()->replaceScene(cocos2d::TransitionFade::create(0.5f, FlockingScene::create(), cocos2d::Color3B::BLACK));
			}
			break;
			case LABEL_INDEX::CIRCLE_PACKING:
			{
				// Load quad tree scene
				cocos2d::Director::getInstance()->replaceScene(cocos2d::TransitionFade::create(0.5f, CirclePackingScene::create(), cocos2d::Color3B::BLACK));
			}
			break;
			case LABEL_INDEX::EXIT:
			{
				cocos2d::Director::getInstance()->end();
			}
			break;
			default:
				break;
			}
		}
	}
}

void MainScene::onMouseUp(cocos2d::Event* event) 
{
	//auto mouseEvent = static_cast<EventMouse*>(event);
	//0 = left, 1 = right, 2 = middle
	//int mouseButton = mouseEvent->getMouseButton();
	//float x = mouseEvent->getCursorX();
	//float y = mouseEvent->getCursorY();
}

void MainScene::onMouseScroll(cocos2d::Event* event) 
{
	//auto mouseEvent = static_cast<EventMouse*>(event);
	//float x = mouseEvent->getScrollX();
	//float y = mouseEvent->getScrollY();
}

void MainScene::onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event) 
{
	if (keyCode == cocos2d::EventKeyboard::KeyCode::KEY_ESCAPE)
	{
		cocos2d::Director::getInstance()->end();
	}
}

void MainScene::onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event) 
{

}

void MainScene::releaseInputListeners()
{
	if(this->mouseInputListener != nullptr)
		_eventDispatcher->removeEventListener(this->mouseInputListener);
	if(this->keyInputListener != nullptr)
		_eventDispatcher->removeEventListener(this->keyInputListener);
}

void MainScene::onExit()
{
	cocos2d::Scene::onExit();
	releaseInputListeners();
}