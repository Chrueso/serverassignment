#pragma once
#include "NetworkManager.h"

// Simple Service Locator approach

class NetworkLocator
{
private:
	static NetworkManager* m_service;

public:
	NetworkLocator() = delete;
	~NetworkLocator() = delete;

	static void provide(NetworkManager* service);
	static NetworkManager& get();
};
