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

#include "IPVS.hxx"
#include "net/SocketError.hxx"

#include <linux/ip_vs.h>
#include <netinet/in.h>

IPVS::IPVS()
{
	if (!socket.Create(AF_INET, SOCK_RAW, IPPROTO_RAW))
		throw MakeSocketError("Failed to create raw socket");

	struct ip_vs_getinfo info;
	if (socket.GetOption(IPPROTO_IP, IP_VS_SO_GET_INFO,
			     &info, sizeof(info)) == 0) {
		const auto e = GetSocketError();
		if (e == ENOPROTOOPT)
			throw MakeSocketError(e, "Kernel does not support IPVS (CONFIG_IP_VS)");
		else
			throw MakeSocketError(e, "IP_VS_SO_GET_INFO failed");
	}
}

void
IPVS::Flush()
{
	if (!socket.SetOption(IPPROTO_IP, IP_VS_SO_SET_FLUSH,
			      nullptr, 0))
		throw MakeSocketError("IP_VS_SO_SET_FLUSH failed");
}

void
IPVS::AddService(const struct ip_vs_service_user &service)
{
	if (!socket.SetOption(IPPROTO_IP, IP_VS_SO_SET_ADD,
			     &service, sizeof(service)))
		throw MakeSocketError("IP_VS_SO_SET_ADD failed");
}

void
IPVS::DeleteService(const struct ip_vs_service_user &service)
{
	if (!socket.SetOption(IPPROTO_IP, IP_VS_SO_SET_DEL,
			     &service, sizeof(service)))
		throw MakeSocketError("IP_VS_SO_DEL_ADD failed");
}

void
IPVS::AddDestination(const struct ip_vs_service_user &service,
		     const struct ip_vs_dest_user &destination)
{
	struct {
		struct ip_vs_service_user service;
		struct ip_vs_dest_user destination;
	} payload = {service, destination};

	if (!socket.SetOption(IPPROTO_IP, IP_VS_SO_SET_ADDDEST,
			      &payload, sizeof(payload)))
		throw MakeSocketError("IP_VS_SO_SET_ADDDEST failed");
}

void
IPVS::EditDestination(const struct ip_vs_service_user &service,
		      const struct ip_vs_dest_user &destination)
{
	struct {
		struct ip_vs_service_user service;
		struct ip_vs_dest_user destination;
	} payload = {service, destination};

	if (!socket.SetOption(IPPROTO_IP, IP_VS_SO_SET_EDITDEST,
			      &payload, sizeof(payload)))
		throw MakeSocketError("IP_VS_SO_SET_EDITDEST failed");
}

void
IPVS::DeleteDestination(const struct ip_vs_service_user &service,
			const struct ip_vs_dest_user &destination)
{
	struct {
		struct ip_vs_service_user service;
		struct ip_vs_dest_user destination;
	} payload = {service, destination};

	if (!socket.SetOption(IPPROTO_IP, IP_VS_SO_SET_DELDEST,
			      &payload, sizeof(payload)))
		throw MakeSocketError("IP_VS_SO_SET_DELDEST failed");
}
