#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>

using namespace geode::prelude;

enum class ArtType {
	Background = 3029,
	Ground = 3030,
	Middleground = 3031
};

class $modify(LEB_LevelEditorLayer, LevelEditorLayer) {

	struct Fields {
		Ref<CCArray> artTriggersArray = CCArray::create();
		int currentMiddleground = -1;
	};

	void levelSettingsUpdated() {
		LevelEditorLayer::levelSettingsUpdated();
		resetCurrentArt();
	}

	void fixGroundScale() { // fix RobTop's zoom bug on ground change
		float scale = m_objectLayer->getScale();

		if (m_groundLayer) {
			m_groundLayer->scaleGround(scale);
		}
		if (m_groundLayer2) {
			m_groundLayer2->scaleGround(scale);
		}
	}

	float getObjectScreenX(GameObject* object) {
		auto scale = m_objectLayer->getScale();
		return
			(object->getPositionX() * scale) +
			m_objectLayer->getPositionX();
	}

	void updateArtTriggerType(ArtTriggerGameObject* trigger, float& largestXOfType, unsigned int& newArtType, bool& foundType) {
		float triggerX = m_playbackActive
			? trigger->getPositionX()
			: getObjectScreenX(trigger);
		float targetX = m_playbackActive
			? m_drawGridLayer->m_playbackX
			: CCDirector::get()->getWinSize().width / 2;

		if (
			trigger->m_isSpawnTriggered ||
			trigger->m_isTouchTriggered ||
			triggerX > targetX ||
			triggerX < largestXOfType

		) {
			return;
		}

		largestXOfType = triggerX;
		newArtType = trigger->m_artIndex;
		foundType = true;
	}

	void resetCurrentArt() {
		m_fields->currentMiddleground = -1;
	}

	void setArt(ArtType triggerID, int art) {

		switch (triggerID) {
			case ArtType::Background:
				swapBackground(art);
				break;
			case ArtType::Ground:
				swapGround(art);
				break;
			case ArtType::Middleground:
				if (art != m_fields->currentMiddleground) {
					swapMiddleground(art);
					m_fields->currentMiddleground = art;
				}
				break;
		}

		if (!m_previewMode) {
			if (m_background) m_background->setColor(ccc3(166, 166, 166)); // real number real number
			if (m_groundLayer) {
				m_groundLayer->updateGround01Color(ccc3(166, 166, 166));
				m_groundLayer->updateGround02Color(ccc3(166, 166, 166));
				if (m_groundLayer->m_lineSprite) m_groundLayer->m_lineSprite->setColor(ccc3(125, 125, 125));
			}
			if (m_groundLayer2) {
				m_groundLayer2->updateGround01Color(ccc3(166, 166, 166));
				m_groundLayer2->updateGround02Color(ccc3(166, 166, 166));
				if (m_groundLayer2->m_lineSprite) m_groundLayer2->m_lineSprite->setColor(ccc3(125, 125, 125));
			}
			if (m_middleground) {
				m_middleground->updateGroundColor(ccc3(100, 100, 100), true);
				m_middleground->updateGroundColor(ccc3(150, 150, 150), false);
			}
		}
	}

	void resetAllArt() {
		setArt(ArtType::Background, m_levelSettings->m_backgroundIndex);
		setArt(ArtType::Ground, m_levelSettings->m_groundIndex);
		setArt(ArtType::Middleground, m_levelSettings->m_middleGroundIndex);
	}

	void updateArtTriggersArray() {

		// exit if mod is diabled in settings
		auto enabled = Mod::get()->getSettingValue<bool>("enabled");
		if (enabled == false) {
			resetAllArt();
			return;
		}

		m_fields->artTriggersArray->removeAllObjects();
		resetCurrentArt();

		for (auto obj : CCArrayExt(m_objects)) {
			if (auto trigger = typeinfo_cast<ArtTriggerGameObject*>(obj)) {
				m_fields->artTriggersArray->addObject(trigger);
			}
		}
	}

	bool init(GJGameLevel* level, bool noUI) {
		if (!LevelEditorLayer::init(level, noUI)) return false;

		updateArtTriggersArray();
		return true;
	}

	void updateEditor(float dt) {
		LevelEditorLayer::updateEditor(dt);

		bool isPlaytesting = static_cast<int>(m_playbackMode) == 1;
		if (isPlaytesting) return;
		if (
			!m_fields->artTriggersArray ||
			!m_objects ||
			m_objects->count() == 0 ||
			m_fields->artTriggersArray->count() == 0
		) {
			setArt(ArtType::Background, m_levelSettings->m_backgroundIndex);
			setArt(ArtType::Ground, m_levelSettings->m_groundIndex);
			setArt(ArtType::Middleground, m_levelSettings->m_middleGroundIndex);
			return;
		}

		fixGroundScale(); // fix RobTop's zoom bug on ground change

		float largestBGTriggerX = -FLT_MAX, largestGTriggerX = -FLT_MAX, largestMGTriggerX = -FLT_MAX;
		unsigned int newBackground = 0, newGround = 0, newMiddleground = 0;
		bool foundBackground = false, foundGround = false, foundMiddleground = false;

		for (auto artTrigger : CCArrayExt<ArtTriggerGameObject>(m_fields->artTriggersArray)) {

			switch(static_cast<ArtType>(artTrigger->m_objectID)) {
				case ArtType::Background:
					updateArtTriggerType(artTrigger, largestBGTriggerX, newBackground, foundBackground);
					break;
				case ArtType::Ground:
					updateArtTriggerType(artTrigger, largestGTriggerX, newGround, foundGround);
					break;		
				case ArtType::Middleground:
					updateArtTriggerType(artTrigger, largestMGTriggerX, newMiddleground, foundMiddleground);
					break;	
			}
		}

		if (!foundBackground)
			setArt(ArtType::Background, m_levelSettings->m_backgroundIndex);
		else
			setArt(ArtType::Background, newBackground);

		if (!foundGround)
			setArt(ArtType::Ground, m_levelSettings->m_groundIndex);
		else
			setArt(ArtType::Ground, newGround);

		if (!foundMiddleground)
			setArt(ArtType::Middleground, m_levelSettings->m_middleGroundIndex);
		else
			setArt(ArtType::Middleground, newMiddleground);
	}

	GameObject* createObject(int key, CCPoint position, bool noUndo) {
		GameObject* ret = LevelEditorLayer::createObject(key, position, noUndo);
		if (
			key == (int)ArtType::Background ||
			key == (int)ArtType::Ground ||
			key == (int)ArtType::Middleground
		) updateArtTriggersArray();
		return ret;
	}

	void onStopPlaytest() {
		LevelEditorLayer::onStopPlaytest();
		resetCurrentArt();
	}

	void swapBackground(int background) {
		int fixedBackround = background < 1 ? 1 : background; // clamp lowest background to 1 to prevent buggy behavior
		LevelEditorLayer::swapBackground(fixedBackround);
	}

	void swapGround(int ground) { 
		int fixedGround = ground < 1 ? 1 : ground; // clamp lowest ground to 1 to prevent buggy behavior
		LevelEditorLayer::swapGround(fixedGround);
	}

	void swapMiddleground(int middleground) { // special case for middleground to avoid bugs when set to 0
		
		LevelEditorLayer::swapMiddleground(middleground);
		if (middleground > 0 && !m_middleground) {
			createMiddleground(middleground);
		}
	}
};

class $modify(EditorUI) {
	CCArray* pasteObjects(gd::string str, bool withColor, bool noUndo) {
		CCArray* ret = EditorUI::pasteObjects(str, withColor, noUndo);
		
		if (auto editor = static_cast<LEB_LevelEditorLayer*>(LEB_LevelEditorLayer::get())) {
			editor->updateArtTriggersArray();
		}
		return ret;
	}

	void onDeleteSelected(CCObject* sender) {
		EditorUI::onDeleteSelected(sender);

		if (auto editor = static_cast<LEB_LevelEditorLayer*>(LEB_LevelEditorLayer::get())) {
			editor->updateArtTriggersArray();
		}
	}
	
	void undoLastAction(CCObject* object) {
		EditorUI::undoLastAction(object);

		if (auto editor = static_cast<LEB_LevelEditorLayer*>(LEB_LevelEditorLayer::get())) {
			editor->updateArtTriggersArray();
		}
	}

	void redoLastAction(CCObject* sender) {
		EditorUI::redoLastAction(sender);

		if (auto editor = static_cast<LEB_LevelEditorLayer*>(LEB_LevelEditorLayer::get())) {
			editor->updateArtTriggersArray();
		}
	}

	void onStopPlaytest(CCObject* sender) {
		
		EditorUI::onStopPlaytest(sender);

		if (auto editor = static_cast<LEB_LevelEditorLayer*>(LEB_LevelEditorLayer::get())) {
			editor->resetCurrentArt();
		}		
	}

	void playtestStopped() {
		
		EditorUI::playtestStopped();

		if (auto editor = static_cast<LEB_LevelEditorLayer*>(LEB_LevelEditorLayer::get())) {
			editor->resetCurrentArt();
		}
	}
};

class $modify(EditorPauseLayer) {

	void onResume(CCObject* sender) {
		EditorPauseLayer::onResume(sender);

		if (auto editor = static_cast<LEB_LevelEditorLayer*>(LEB_LevelEditorLayer::get())) {
			editor->updateArtTriggersArray();
		}
	}
};