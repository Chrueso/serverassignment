#pragma once
#include "BaseState.h"
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>

class MenuState : public BaseState
{
	tgui::Gui gui; // important to be first

	tgui::Panel::Ptr panel;
	tgui::EditBox::Ptr nameBox;
	tgui::EditBox::Ptr ipBox1;
	tgui::EditBox::Ptr ipBox2;
	tgui::EditBox::Ptr ipBox3;
	tgui::EditBox::Ptr ipBox4;
	tgui::EditBox::Ptr portBox;
	tgui::Button::Ptr loginButton;

	void onInputBoxChanged();
	void onLoginPress();

protected:
	void handlePacket(sf::Packet& packet) override;

public:
	MenuState(StateMachine& machine, sf::RenderWindow& window, bool replace = true);

	void activate() override;
	void pause() override;
	void resume() override;
	void handleEvent(sf::Event event) override;
	void update(sf::Time deltaTime) override;
	void draw() override;
};
