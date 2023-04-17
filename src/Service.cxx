// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "Service.hxx"
#include "Config.hxx"
#include "IPVS.hxx"
#include "lib/avahi/Client.hxx"
#include "net/IPv4Address.hxx"
#include "lib/fmt/SystemError.hxx"

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
		throw FmtErrno("Failed to find interface '{}'", name);

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
		   kernel; update it (just in case parameters other
		   than the address/port pair have changed) */

		try {
			ipvs.EditDestination(service, i->first);
		} catch (...) {
			logger(1, "Failed to edit destination ",
			       key, ": ", std::current_exception());
		}

		destinations.erase(i->second);

		auto [j, _] = destinations.insert_or_assign(key, d);
		i->second = j;
		return;
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
