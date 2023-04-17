// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

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
