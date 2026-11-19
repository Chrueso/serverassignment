#include "MenuState.h"
#include "PacketType.h"
#include "GameState.h"

MenuState::MenuState(StateMachine& machine, sf::RenderWindow& window, bool replace)
	: BaseState{ machine, window, replace }, gui(window)
{
	gui.loadWidgetsFromFile("../assets/form_menu.txt");

	panel = gui.get<tgui::Panel>("LoginPanel");
	nameBox = gui.get<tgui::EditBox>("UsernameBox");
	portBox = gui.get<tgui::EditBox>("PortBox");
	ipBox1 = gui.get<tgui::EditBox>("IPBox1");
	ipBox2 = gui.get<tgui::EditBox>("IPBox2");
	ipBox3 = gui.get<tgui::EditBox>("IPBox3");
	ipBox4 = gui.get<tgui::EditBox>("IPBox4");
	loginButton = gui.get<tgui::Button>("LoginButton");

	ipBox1->setText("26");
	ipBox2->setText("113");
	ipBox3->setText("211");
	ipBox4->setText("220");
	portBox->setText("53000");

	nameBox->onTextChange(&MenuState::onInputBoxChanged, this);
	portBox->onTextChange(&MenuState::onInputBoxChanged, this);
	ipBox1->onTextChange(&MenuState::onInputBoxChanged, this);
	ipBox2->onTextChange(&MenuState::onInputBoxChanged, this);
	ipBox3->onTextChange(&MenuState::onInputBoxChanged, this);
	ipBox4->onTextChange(&MenuState::onInputBoxChanged, this);
	loginButton->setEnabled(false);

	loginButton->onPress(&MenuState::onLoginPress, this);
}

void MenuState::onInputBoxChanged()
{
	// forces some login requirement before allowing join
	// this is client-sided, so server technically should recheck
	// in this case, only allow join button if name length is at least 4
	bool ipFilled = ipBox1->getText().size() > 0 && ipBox2->getText().size() > 0 && ipBox3->getText().size() > 0 && ipBox4->getText().size() > 0;
	bool portFilled = portBox->getText().size() > 0;
	bool nameFilled = nameBox->getText().size() >= 4;
	loginButton->setEnabled(ipFilled && portFilled && nameFilled);
}

void MenuState::onLoginPress()
{
	panel->setVisible(false);

	// for now, immediately go to next scene
	// in proper implementation, you should:
	// 1. get network manager from network locator
	// 2. try to connect to the ip:port
	// 3. send client join request
	// 4. handle packet receive via subscription (see below)
	//    and switch scene if successfully join.

	auto& nm = NetworkLocator::get();
	sf::IpAddress ip(
		std::stoi(ipBox1->getText().toStdString()),
		std::stoi(ipBox2->getText().toStdString()),
		std::stoi(ipBox3->getText().toStdString()),
		std::stoi(ipBox4->getText().toStdString())
	);

	unsigned short pot = std::stoi(portBox->getText().toStdString());

	if (nm.connect(ip, pot)) {
		std::cout << "Connected to server" << std::endl;
		std::string name = nameBox->getText().toStdString();
		sf::Packet outPacket;

		outPacket << PacketType::ClientJoinRequest << name << false;
		nm.sendPacket(outPacket);
	
	}
	else {
		std::cout << "Error" << std::endl;
		panel->setVisible(true);
	}

	
}

void MenuState::handlePacket(sf::Packet& packet)
{

	PacketType type;
	packet >> type;

	switch (type) {
	case PacketType::ServerJoinResponse:
		std::cout << "Server accepted" << std::endl;
		int clientId;
		packet >> clientId;
		NetworkLocator::get().setClientId(clientId);
		m_next = StateMachine::build<GameState>(m_machine, m_window, true);
		break;
	case PacketType::ServerJoinRejected:
		std::cout << "Server rejected" << std::endl;
		break;
	}

}

void MenuState::activate()
{
	// needed for base state to subscribe to network manager
	// for handlePacket to be triggered when receiving packets
	BaseState::activate();
}

void MenuState::pause() {}

void MenuState::resume() {}

void MenuState::handleEvent(sf::Event event)
{
	// needed otherwise any interactions on gui elements
	// are ignored
	gui.handleEvent(event);
}

void MenuState::update(sf::Time deltaTime)
{
}

void MenuState::draw()
{
	gui.draw();
}