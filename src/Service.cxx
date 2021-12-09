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

#include "Service.hxx"
#include "Config.hxx"
#include "IPVS.hxx"
#include "lib/avahi/Client.hxx"
#include "net/IPv4Address.hxx"
#include "system/Error.hxx"

#include <algorithm>
#include <cstdint>

#include <net/if.h>

static constexpr struct ip_vs_service_user
ToIpvsService(const IPv4Address addr, const std::string_view sched_name) noexcept
{
	struct ip_vs_service_user s{};
	s.protocol = IPPROTO_TCP;
	s.addr = addr.GetNumericAddressBE();
	s.port = addr.GetPortBE();
	s.netmask = ~uint_least32_t{};

	std::copy_n(sched_name.begin(),
		    std::min(sched_name.size(), std::size(s.sched_name) - 1),
		    s.sched_name);

	return s;
}

static constexpr struct ip_vs_dest_user
ToIpvsDestination(const IPv4Address addr) noexcept
{
	struct ip_vs_dest_user d{};
	d.addr = addr.GetNumericAddressBE();
	d.port = addr.GetPortBE();
	d.conn_flags = IP_VS_CONN_F_MASQ;
	d.weight = 1;
	return d;
}

static AvahiIfIndex
ResolveInterfaceName(const char *name)
{
	int i = if_nametoindex(name);
	if (i == 0)
		throw FormatErrno("Failed to find interface '%s'", name);

	return static_cast<AvahiIfIndex>(i);
}

Service::Service(Avahi::Client &avahi_client,
		 Avahi::ErrorHandler &avahi_error_handler,
		 IPVS &_ipvs,
		 const ServiceConfig &config)
	:logger("service " + config.zeroconf_service),
	 service(ToIpvsService(config.bind_address, config.scheduler)),
	 explorer(avahi_client, *this,
		  config.zeroconf_interface.empty()
		  ? AVAHI_IF_UNSPEC
		  : ResolveInterfaceName(config.zeroconf_interface.c_str()),
		  AVAHI_PROTO_INET, // TODO
		  config.zeroconf_service.c_str(),
		  config.zeroconf_domain.empty()
		  ? nullptr
		  : config.zeroconf_domain.c_str(),
		  avahi_error_handler),
	 ipvs(_ipvs)
{
	ipvs.AddService(service);
}

Service::~Service() noexcept
{
	try {
		ipvs.DeleteService(service);
	} catch (...) {
		logger(1, "Failed to delete service: ",
		       std::current_exception());
	}
}

void
Service::OnAvahiNewObject(const std::string &key,
			  SocketAddress address) noexcept
{
	assert(address.IsDefined());

	if (address.GetFamily() != AF_INET)
		/* ignore IPv6 (for now) */
		return;

	const auto &ipv4 = IPv4Address::Cast(address);

	const auto d = ToIpvsDestination(ipv4);

	if (auto i = addresses.find(d); i != addresses.end()) {
		/* the address/port combination exists already in the
		   kernel; delete the old one from the "destinations"
		   map, and also delete it from the kernel, because
		   parameters other than the address/port pair may
		   have changed */

		try {
			ipvs.DeleteDestination(service, i->first);
		} catch (...) {
			logger(1, "Failed to delete destination ",
			       key, ": ", std::current_exception());
		}

		destinations.erase(i->second);
		addresses.erase(i);
	}

	try {
		ipvs.AddDestination(service, d);
	} catch (...) {
		logger(1, "Failed to add destination ", key, ": ",
		       std::current_exception());
		return;
	}

	auto [i, _] = destinations.insert_or_assign(key, d);
	addresses.emplace(d, i);
}

void
Service::OnAvahiRemoveObject(const std::string &key) noexcept
{
	if (auto i = destinations.find(key); i != destinations.end()) {
		try {
			ipvs.DeleteDestination(service, i->second);
		} catch (...) {
			logger(1, "Failed to delete destination ",
			       key, ": ", std::current_exception());
		}

		auto j = addresses.find(i->second);
		assert(j != addresses.end());
		addresses.erase(j);

		destinations.erase(i);
	}
}
