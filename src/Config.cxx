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

#include "Config.hxx"
#include "io/FileLineParser.hxx"
#include "io/ConfigParser.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "net/Parser.hxx"
#include "lib/avahi/Check.hxx"

#include <string.h>

class KlbConfigParser final : public NestedConfigParser {
	Config &config;

	class Service final : public ConfigParser {
		KlbConfigParser &parent;
		ServiceConfig config;

	public:
		explicit Service(KlbConfigParser &_parent) noexcept
			:parent(_parent) {}

	protected:
		/* virtual methods from class ConfigParser */
		void ParseLine(FileLineParser &line) override;
		void Finish() override;
	};

public:
	explicit KlbConfigParser(Config &_config) noexcept
		:config(_config) {}

protected:
	/* virtual methods from class NestedConfigParser */
	void ParseLine2(FileLineParser &line) override;

	/* virtual methods from class ConfigParser */
	void Finish() override;

private:
	void CreateService(FileLineParser &line);
	void CreateControl(FileLineParser &line);
};

void
KlbConfigParser::Service::ParseLine(FileLineParser &line)
{
	const char *word = line.ExpectWord();

	if (strcmp(word, "bind") == 0) {
		if (config.bind_address.IsDefined())
			throw LineParser::Error("Bind address already specified");

		auto bind_address =
			ParseSocketAddress(line.ExpectValueAndEnd(), 0, true);
		if (bind_address.GetFamily() != AF_INET)
			throw LineParser::Error("Bind address must be IPv4");

		config.bind_address = IPv4Address{bind_address};
		if (config.bind_address.GetPort() == 0)
			throw LineParser::Error("Bind address must have a port");
	} else if (strcmp(word, "scheduler") == 0) {
		config.scheduler = line.ExpectValueAndEnd();
	} else if (strcmp(word, "zeroconf_service") == 0 ||
		   /* old option name: */ strcmp(word, "zeroconf_type") == 0) {
		config.zeroconf_service = MakeZeroconfServiceType(line.ExpectValueAndEnd(),
								  "_tcp");
	} else if (strcmp(word, "zeroconf_domain") == 0) {
		if (!config.zeroconf_domain.empty())
			throw LineParser::Error("Duplicate zeroconf_domain");

		config.zeroconf_domain = line.ExpectValueAndEnd();
	} else if (strcmp(word, "zeroconf_interface") == 0) {
		config.zeroconf_interface = line.ExpectValueAndEnd();
	} else
		throw LineParser::Error("Unknown option");
}

void
KlbConfigParser::Service::Finish()
{
	if (!config.bind_address.IsDefined())
		throw LineParser::Error("Service has no bind address");

	parent.config.services.emplace_front(std::move(config));

	ConfigParser::Finish();
}

inline void
KlbConfigParser::CreateService(FileLineParser &line)
{
	line.ExpectSymbolAndEol('{');

	SetChild(std::make_unique<Service>(*this));
}

void
KlbConfigParser::ParseLine2(FileLineParser &line)
{
	const char *word = line.ExpectWord();

	if (strcmp(word, "service") == 0)
		CreateService(line);
	else
		throw LineParser::Error("Unknown option");
}

void
KlbConfigParser::Finish()
{
	if (config.services.empty())
		throw LineParser::Error("No service configured");

	NestedConfigParser::Finish();
}

Config
LoadConfigFile(const char *path)
{
	Config config;
	KlbConfigParser parser(config);
	VariableConfigParser v_parser(parser);
	CommentConfigParser parser2(v_parser);
	IncludeConfigParser parser3(path, parser2);

	ParseConfigFile(path, parser3);

	return config;
}
