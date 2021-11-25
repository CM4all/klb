klb
===

*klb* means "Kernel Load Balancer".  It is a (user-space) daemon which
acts as a bridge between `Linux IPVS
<http://www.linuxvirtualserver.org/software/ipvs.html>`__ and
`Zeroconf <http://www.zeroconf.org/>`__.  IPVS services are populated
with destinations discovered using Zeroconf / `DNS-SD
<http://www.dns-sd.org/>`__.  This allows implementing
high-performance load balancers with a dynamic set of servers.


Building klb
------------

You need:

- a C++20 compliant compiler (e.g. `GCC <https://gcc.gnu.org/>`__ or
  `clang <https://clang.llvm.org/>`__)
- `systemd <https://www.freedesktop.org/wiki/Software/systemd/>`__
- `avahi <https://www.avahi.org/>`__
- `Meson 0.56 <http://mesonbuild.com/>`__ and `Ninja
  <https://ninja-build.org/>`__
- a Linux kernel with ``CONFIG_IP_VS=y``

First make sure that the ``libcommon`` submodule is available::

  git submodule update --init

Run ``meson``::

  meson . output

Compile and install::

  ninja -C output
  ninja -C output install


Configuring klb
---------------

The file ``/etc/cm4all/klb/klb.conf`` contains the configuration.
Example::

  service {
    bind "1.2.3.4:80"
    zeroconf_service "foo"
    zeroconf_interface "service"
  }

This load-balances all incoming TCP connections on ``1.2.3.4:80`` to
all workers which announce a service called ``_foo._tcp`` via
Zeroconf.

There can be any number of ``service`` sections, each of which can
contain the following options:

- ``bind``: an IP address with port where TCP connections arrive (IPv4
  only currently)
- ``scheduler``: the name of the IPVS scheduler; defaults to ``rr``
  (round-robin)
- ``zeroconf_service``: the name of the Zeroconf service where
  connections will be routed to
- ``zeroconf_domain``: an optional Zeroconf domain
- ``zeroconf_interface``: an optional network interface name where
  Zeroconf services will be discovered

To publish Zeroconf services to be discovered by this software, you
can either use static service files in
``/etc/avahi/services/*.service``, the command-line tool
``/usr/bin/avahi-publish-service`` (launched as daemon along with the
daemon which actually listens on the port) or a daemon which has a
built-in Zeroconf publisher (e.g. `beng-proxy
<https://github.com/CM4all/beng-proxy/>`__).

On workers (where connections will be routed to), routing needs to be
set up so that reply IP packets are routed through the machine running
this software.  If you run it on a dedicated network interface (and
the default gateway is on a different interface), you can configure a
routing policy by adding something like that to the
``systemd.network`` file of that dedicated interface on each worker::

  # Assuming the dedicated interface has network 192.168.99.0/24 and
  # 192.168.99.1 is the server running "klb"; this creates the routing
  # table "99", and a route to the interface's network:
  [Route]
  Destination=192.168.99.0/24
  Table=99

  # Now configure the "klb" server as gateway on that routing table:
  [Route]
  Gateway=192.168.99.1
  GatewayOnLink=yes
  Scope=global
  Table=99

  # This policy tells the server to use our newly created routing
  # table for all reply packets to connections which were routed
  # through "klb", instead of using the default gateway in the
  # standard routing table:
  [RoutingPolicyRule]
  From=192.168.99.0/24
  Table=99


Building the Debian package
---------------------------

After installing the build dependencies, run::

 dpkg-buildpackage -rfakeroot -b -uc -us
