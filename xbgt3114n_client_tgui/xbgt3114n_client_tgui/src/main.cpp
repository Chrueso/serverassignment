#include "NetworkLocator.h" // includes NetworkManager
#include "StateMachine.hpp"
#include "MenuState.h"

int main()
{
	// create _network manager_
	NetworkManager nm;
	// assign to _network locator_
	NetworkLocator::provide(&nm);

	sf::RenderWindow window(sf::VideoMode({ 1024, 768 }), "SFML TGUI", sf::Style::Titlebar | sf::Style::Close);

	// StateMachine and State classes are
	// adapted from https://github.com/eXpl0it3r/SmallGameEngine/tree/master
	StateMachine sm;
	sm.run(StateMachine::build<MenuState>(sm, window, true));

	sf::Clock clock;
	while (window.isOpen())
	{
		while (const std::optional event = window.pollEvent())
		{
			sm.handleEvent(*event);

			if (event->is<sf::Event::Closed>())
				window.close();
		}

		// update _network manager_
		nm.update();
		auto dt = clock.restart();

		sm.nextState();
		sm.update(dt);

		window.clear();

		sm.draw();

		window.display();
	}
}