// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "IPVS.hxx"
#include "io/Logger.hxx"
#include "lib/avahi/Client.hxx"
#include "lib/avahi/ErrorHandler.hxx"
#include "event/Loop.hxx"
#include "event/ShutdownListener.hxx"

struct Config;
class Service;

class Instance final
	: Avahi::ErrorHandler
{
	EventLoop event_loop;
	ShutdownListener shutdown_listener{
		event_loop,
		BIND_THIS_METHOD(OnShutdown),
	};

	Avahi::Client avahi_client{event_loop, *this};

	IPVS ipvs;

	std::forward_list<Service> services;

public:
	RootLogger logger;

	explicit Instance(const Config &config);
	~Instance() noexcept;

	auto &GetEventLoop() noexcept {
		return event_loop;
	}

private:
	void OnShutdown() noexcept;

	/* virtual methods from class Avahi::ErrorHandler */
	bool OnAvahiError(std::exception_ptr e) noexcept override;
};
