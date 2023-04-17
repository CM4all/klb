// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "net/UniqueSocketDescriptor.hxx"

class IPVS {
	UniqueSocketDescriptor socket;

public:
	IPVS();

	void Flush();

	void AddService(const struct ip_vs_service_user &service);
	void DeleteService(const struct ip_vs_service_user &service);

	void AddDestination(const struct ip_vs_service_user &service,
			    const struct ip_vs_dest_user &destination);

	void EditDestination(const struct ip_vs_service_user &service,
			     const struct ip_vs_dest_user &destination);

	void DeleteDestination(const struct ip_vs_service_user &service,
			       const struct ip_vs_dest_user &destination);
};
