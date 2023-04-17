// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "Instance.hxx"
#include "Config.hxx"
#include "Service.hxx"
#include "util/PrintException.hxx"

#include <stdexcept>

Instance::Instance(const Config &config)
{
	ipvs.Flush();

	Avahi::ErrorHandler &avahi_error_handler = *this;
	for (const auto &i : config.services)
		services.emplace_front(avahi_client, avahi_error_handler,
				       ipvs, i);

	shutdown_listener.Enable();
}

Instance::~Instance() noexcept = default;

void
Instance::OnShutdown() noexcept
{
	event_loop.Break();
}

bool
Instance::OnAvahiError(std::exception_ptr e) noexcept
{
	PrintException(e);
	return true;
}
