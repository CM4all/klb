// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "net/IPv4Address.hxx"

#include <forward_list>
#include <string>

struct ServiceConfig {
	IPv4Address bind_address{};

	std::string scheduler{"rr"};

	std::string zeroconf_service;
	std::string zeroconf_domain;
	std::string zeroconf_interface;
};

struct Config {
	std::forward_list<ServiceConfig> services;
};

/**
 * Load and parse the specified configuration file.  Throws on error.
 */
Config
LoadConfigFile(const char *path);
