/*
 * sound/soc/lpc313x/lpc313x-uda1380.c
 *
 * Author: Kevin Wells <kevin.wells@nxp.com>
 *
 * Copyright (C) 2009 NXP Semiconductors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include "../codecs/fhsanalog1.h"
#include "lpc313x-pcm.h"
#include "lpc313x-i2s.h"
#include "lpc313x-i2s-clocking.h"

#define SND_MODNAME "lpc313x_fhsanalog1"

static int fhs3143_fhsanalog1_hw_params(struct snd_pcm_substream *substream,
				    struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	const unsigned int fmt = (SND_SOC_DAIFMT_I2S |
				  SND_SOC_DAIFMT_CBM_CFS);
	int ret;


	/* Set the CPU I2S rate clock (first) */
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, params_rate(params),
					    SND_SOC_CLOCK_OUT);
	if (ret < 0) {
		pr_warning("%s: "
			   "Failed to set I2S clock (%d)\n",
			   SND_MODNAME, ret);
		return ret;
	}

	/* Set CPU and CODEC DAI format */
	ret = snd_soc_dai_set_fmt(cpu_dai, fmt);
	if (ret < 0) {
		pr_warning("%s: "
			   "Failed to set CPU DAI format (%d)\n",
			   SND_MODNAME, ret);
		return ret;
	}
	ret = snd_soc_dai_set_fmt(codec_dai, fmt);
	if (ret < 0) {
		pr_warning("%s: "
			   "Failed to set CODEC DAI format (%d)\n",
			   SND_MODNAME, ret);
		return ret;
	}

	return 0;
}

static struct snd_soc_ops fhs3143_fhsanalog1_ops = {
	.hw_params = fhs3143_fhsanalog1_hw_params,
};

static const struct snd_soc_dapm_widget fhs3143_dapm_widgets[] = {
	SND_SOC_DAPM_LINE("Line Out", NULL),
	SND_SOC_DAPM_LINE("Line In", NULL),
};

static const struct snd_soc_dapm_route intercon[] = {

	/* Line Out connected to VOUTR, VOUTL */
	{"Line Out", NULL, "VOUTR"},
	{"Line Out", NULL, "VOUTL"},

	/* Line In connected to VINR, VINL */
	{"VINL", NULL, "Microphone In Voice"},
	{"VINR", NULL, "Microphone In Guitar"},
};

static int fhs3143_fhsanalog1_init(struct snd_soc_codec *codec)
{
	
#if 0
	/* Add widgets */
	
	snd_soc_dapm_new_controls(codec, fhs3143_dapm_widgets,
				  ARRAY_SIZE(fhs3143_dapm_widgets));

	/* Set up audio path audio_map */
	snd_soc_dapm_add_routes(codec, intercon, ARRAY_SIZE(intercon));

	/* Always connected pins */
	snd_soc_dapm_enable_pin(codec, "Line Out");
	snd_soc_dapm_enable_pin(codec, "Line In");

	snd_soc_dapm_sync(codec);
#endif
	return 0;
}

static struct snd_soc_dai_link fhs3143_fhsanalog1_dai[] = {
	{
		.name = "FHSANALOG1",
		.stream_name = "FHSANALOG1",
		.cpu_dai = &lpc313x_i2s_dai,
		.codec_dai = &fhsanalog1_dai,
		.init = fhs3143_fhsanalog1_init,
		.ops = &fhs3143_fhsanalog1_ops,
	},
};

static struct snd_soc_machine snd_soc_machine_fhs3143 = {
	.name = "LPC313X_I2S_FHSANALOG1",
	.dai_link = &fhs3143_fhsanalog1_dai[0],
	.num_links = ARRAY_SIZE(fhs3143_fhsanalog1_dai),
};



static struct snd_soc_device fhs3143_fhsanalog1_snd_devdata = {
	.machine = &snd_soc_machine_fhs3143,
	.platform = &lpc313x_soc_platform,
	.codec_dev = &soc_codec_dev_fhsanalog1
};

static struct platform_device *fhs3143_snd_device;
static int __init fhs3143_asoc_init(void)
{
	int ret = 0;

	/*
	 * Create and register platform device
	 */
	fhs3143_snd_device = platform_device_alloc("soc-audio", 0);
	if (fhs3143_snd_device == NULL) {
		return -ENOMEM;
	}

	platform_set_drvdata(fhs3143_snd_device, &fhs3143_fhsanalog1_snd_devdata);
	fhs3143_fhsanalog1_snd_devdata.dev = &fhs3143_snd_device->dev;

	/*
	 * Enable CODEC clock first or I2C will fail to the CODEC
	 */
	lpc313x_main_clk_rate(48000);

	ret = platform_device_add(fhs3143_snd_device);
	if (ret) {
		pr_warning("%s: platform_device_add failed (%d)\n",
			   SND_MODNAME, ret);
		goto err_device_add;
	}

	return 0;

err_device_add:
	if (fhs3143_snd_device != NULL) {
		platform_device_put(fhs3143_snd_device);
		lpc313x_main_clk_rate(0);
		fhs3143_snd_device = NULL;
	}

	return ret;
}

static void __exit fhs3143_asoc_exit(void)
{
	platform_device_unregister(fhs3143_snd_device);
	lpc313x_main_clk_rate(0);
	fhs3143_snd_device = NULL;
}

module_init(fhs3143_asoc_init);
module_exit(fhs3143_asoc_exit);

MODULE_AUTHOR("Miguel Angel Ajo <miguelangel@nbee.es>");
MODULE_DESCRIPTION("ASoC machine driver for LPC313X/FHSANALOG1");
MODULE_LICENSE("GPL");

