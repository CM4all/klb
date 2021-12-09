/*
 * Copyright 2021 CM4all GmbH
 * All rights reserved.
 *
 * author: Max Kellermann <mk@cm4all.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "lib/avahi/Explorer.hxx"
#include "lib/avahi/ExplorerListener.hxx"
#include "io/Logger.hxx"

#include <map>

#include <linux/ip_vs.h>

struct ServiceConfig;
class IPVS;

class Service final
	: Avahi::ServiceExplorerListener
{
	const Logger logger;

	const struct ip_vs_service_user service;

	Avahi::ServiceExplorer explorer;

	IPVS &ipvs;

	using DestinationMap = std::map<std::string, struct ip_vs_dest_user>;
	DestinationMap destinations;

	struct CompareAddressPort {
		constexpr bool operator()(const struct ip_vs_dest_user &a,
					  const struct ip_vs_dest_user &b) const noexcept
		{
			if (a.addr != b.addr)
				return a.addr < b.addr;

			return a.port < b.port;
		}
	};

	/**
	 * Tracks which addresses are registered as destinations in
	 * the kernel; the value points into the "destinations" map.
	 */
	std::map<struct ip_vs_dest_user, DestinationMap::const_iterator,
		 CompareAddressPort> addresses;

public:
	Service(Avahi::Client &avahi_client,
		Avahi::ErrorHandler &avahi_error_handler,
		IPVS &_ipvs,
		const ServiceConfig &config);

	~Service() noexcept;

private:
	/* virtual methods from class AvahiServiceExplorerListener */
	void OnAvahiNewObject(const std::string &key,
			      SocketAddress address) noexcept override;
	void OnAvahiRemoveObject(const std::string &key) noexcept override;
};
