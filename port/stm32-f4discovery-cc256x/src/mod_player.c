#include "stm32f4_discovery_audio.h"
#include "hxcmod.h"
#include "mods/mod.h"

#define HMOD_SAMPLES 100
static uint16_t audio_samples1[HMOD_SAMPLES*2];
static uint16_t audio_samples2[HMOD_SAMPLES*2];
static volatile int active_buffer;
static int hxcmod_initialized = 0;
static modcontext mod_context;

void BSP_AUDIO_OUT_TransferComplete_CallBack(void){
	if (active_buffer){
		BSP_AUDIO_OUT_ChangeBuffer(&audio_samples1[0], HMOD_SAMPLES * 2);
		active_buffer = 0;
	} else {
		BSP_AUDIO_OUT_ChangeBuffer(&audio_samples2[0], HMOD_SAMPLES * 2);
		active_buffer = 1;
	}
}

void audio(void){
	BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_BOTH, 70, 44100);

	hxcmod_initialized = hxcmod_init(&mod_context);
    if (hxcmod_initialized){
        hxcmod_setcfg(&mod_context, 44100, 16, 1, 1, 1);
        hxcmod_load(&mod_context, (void *) &mod_data, mod_len);
    }

	active_buffer = 0;
	hxcmod_fillbuffer(&mod_context, (unsigned short *) &audio_samples1[0], HMOD_SAMPLES, NULL);
	BSP_AUDIO_OUT_Play(&audio_samples1[0], HMOD_SAMPLES * 4);

	while (1){
		hxcmod_fillbuffer(&mod_context, (unsigned short *) &audio_samples2[0], HMOD_SAMPLES, NULL);
		while (active_buffer == 0){
			__asm__("wfe");
		}
		hxcmod_fillbuffer(&mod_context, (unsigned short *) &audio_samples1[0], HMOD_SAMPLES, NULL);
		while (active_buffer == 0){
			__asm__("wfe");
		}
	}
}
