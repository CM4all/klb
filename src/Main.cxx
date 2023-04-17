// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "Instance.hxx"
#include "Config.hxx"
#include "util/PrintException.hxx"

#include <systemd/sd-daemon.h>

#include <stdlib.h>
#include <sys/signal.h>

static void
SetupProcess() noexcept
{
	signal(SIGPIPE, SIG_IGN);
}

static int
Run(const Config &config)
{
	Instance instance{config};

	/* tell systemd we're ready */
	sd_notify(0, "READY=1");

	instance.GetEventLoop().Run();
	return EXIT_SUCCESS;
}

int
main(int, char **) noexcept
try {
	const auto config = LoadConfigFile("/etc/cm4all/klb/klb.conf");

	SetupProcess();

	return Run(config);
} catch (...) {
	PrintException(std::current_exception());
	return EXIT_FAILURE;
}
