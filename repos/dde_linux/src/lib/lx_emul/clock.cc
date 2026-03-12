/*
 * \brief  Lx_emul backend for peripheral clocks
 * \author Stefan Kalkowski
 * \date   2021-03-22
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2 or later.
 */

#include <base/log.h>
#include <linux/errno.h>
#include <lx_emul/clock.h>
#include <lx_kit/env.h>

/*
 * CONFIG_OF is normally disabled on PC thus of_device_is_compatible() should
 * be "static inline" not "extern" (see include/linux/of.h), thus enable this
 * function only if not building for x86.
 */
extern "C" int of_device_is_compatible(const struct device_node * node, const char * compat);
static int _of_device_is_compatible(const struct device_node * node, const char * compat)
{
#if defined(__x86_64__) || defined(__i386__)
	(void)node; (void)compat;
	return 0;
#else
	return of_device_is_compatible(node, compat);
#endif
}

struct clk * lx_emul_clock_get(const struct device_node * node,
                               const char * name)
{
	using namespace Lx_kit;

	struct clk * ret = nullptr;

	env().devices.for_each([&] (Device &d) {
		if (!_of_device_is_compatible(node, d.compatible()))
			return;
		ret = name ? d.clock(name) : d.clock(0U);
		if (!ret) warning("No clock ", name, " found for device ", d.name());
	});

	return ret;
}


unsigned long lx_emul_clock_get_rate(struct clk * clk)
{
	if (!clk)
		return 0;

	return clk->rate;
}

int lx_emul_clock_set_rate(struct clk * clk, unsigned long rate)
{
	using namespace Lx_kit;

	if (!clk) {
		Genode::error("driver called ", __FUNCTION__, " with a null clk ptr!");
		return -EINVAL;
	}

	int ret = -ENODEV;

	env().devices.for_each([&] (Device &d) {
		if (d.name() != clk->device_name) return;
		ret = d.clock_set_rate(clk->idx, rate);
	});

	if (ret < 0) {
		Genode::error("could not set clock '", clk->name, "' rate to '", rate, "' on device '", clk->device_name, "' (error: ", ret, ")!");
	}

	return ret;
}
