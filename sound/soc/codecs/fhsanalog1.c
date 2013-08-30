/*
 * fhsanalog1.c  --  ALSA Soc FHS codec support
 *
 * Copyright:	NBEE Embedded Systems SL
 * Author:	Miguel Angel Ajo <miguelangel@nbee.es>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  Revision history
 *    14th Oct 2011   Initial version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/ac97_codec.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include "fhsanalog1.h"



static int fhsanalog1_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	#if 0
	struct snd_soc_codec *codec = codec_dai->codec;
	
	/* our hardware setup is fixed, but keep this as a placeholder for 
	   future implementations of FHSANALOG_ boards */
	   
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
			case SND_SOC_DAIFMT_I2S:
			
				break;
			case SND_SOC_DAIFMT_LSB:
			
				break;
			case SND_SOC_DAIFMT_MSB:
	
	}
 	#endif
	return 0;
}

static int fhsanalog1_pcm_prepare(struct snd_pcm_substream *substream)
{
	
	#if 0
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->codec;
	
		/* placeholder as we do nothing right now about this */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
	
	} else {
	
	}
	#endif

	return 0;
}

static int fhsanalog1_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	#if 0
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->codec;

	int rate = params_rate(params);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		{
		}
	else
		{
		}

	#endif
	
	return 0;
}

static void fhsanalog1_pcm_shutdown(struct snd_pcm_substream *substream)
{
	#if 0
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->codec;
	
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
	{}
	else
	{}

	#endif
}

static int fhsanalog1_mute(struct snd_soc_dai *codec_dai, int mute)
{
	#if 0
	struct snd_soc_codec *codec = codec_dai->codec;
	if (mute)
	{}	
	else
	{}		
	#endif
	return 0;
}




struct snd_soc_dai fhsanalog1_dai = {
	.name = "FHSANALOG1",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_44100,
		.formats = SNDRV_PCM_FMTBIT_S16_LE, },
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_44100,
		.formats = SNDRV_PCM_FMTBIT_S16_LE, },
		
		.ops = {
            .hw_params = fhsanalog1_pcm_hw_params,
            .shutdown = fhsanalog1_pcm_shutdown,
            .prepare = fhsanalog1_pcm_prepare,
        },
    .dai_ops = {
            .digital_mute = fhsanalog1_mute,
            .set_fmt = fhsanalog1_set_dai_fmt,
     },
    
};


EXPORT_SYMBOL_GPL(fhsanalog1_dai);



static int fhsanalog1_soc_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	int ret = 0;

	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;
	mutex_init(&codec->mutex);
	codec->name = "FHSANALOG1";
	codec->owner = THIS_MODULE;
	codec->dai = &fhsanalog1_dai;
	codec->num_dai = 1;
	socdev->codec = codec;
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(KERN_ERR "fhsanalog1: failed to create pcms\n");
		goto pcm_err;
	}

	ret = snd_soc_register_card(socdev);
	if (ret < 0) {
		printk(KERN_ERR "fhsanalog1: failed to register card\n");
		goto register_err;
	}

	return ret;

register_err:
	snd_soc_free_pcms(socdev);
pcm_err:
	kfree(socdev->codec);
	socdev->codec = NULL;
	return ret;
}

static int fhsanalog1_soc_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	if (codec == NULL)
		return 0;
	snd_soc_free_pcms(socdev);
	kfree(codec);
	return 0;
}

struct snd_soc_codec_device soc_codec_dev_fhsanalog1 = {
	.probe = 	fhsanalog1_soc_probe,
	.remove = 	fhsanalog1_soc_remove,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_fhsanalog1);

MODULE_DESCRIPTION("ASoC FHS Analog Board 1 driver");
MODULE_AUTHOR("Miguel Angel Ajo <miguelangel@nbee.es>");
MODULE_LICENSE("GPL");
