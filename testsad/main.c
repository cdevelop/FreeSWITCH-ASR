// testsad.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
#include <stdlib.h>
#include "libsad.h"



#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"


void testvad(const char* filename)
{
    drwav wav;
    if (!drwav_init_file(&wav, filename, NULL)) {
        printf("open file failed\n");
        return;
    }

    if (wav.channels != 1) {
        printf("%d channels nonsupport\n", wav.channels);
        drwav_uninit(&wav);
        return;
    }



    drwav_int16* pDecodedInterleavedPCMFrames = malloc(wav.totalPCMFrameCount * wav.channels * sizeof(drwav_int16));
    size_t numberOfSamplesActuallyDecoded = drwav_read_pcm_frames_s16(&wav, wav.totalPCMFrameCount, pDecodedInterleavedPCMFrames);



    DD_VAD* vad = dd_vad_create(wav.sampleRate, 20, 1000, DD_VAD_MODE_AGGRESSIVE, 100, 200, 20, 0.8);

    if (vad) {
        int laststate = 0;

        drwav* fragment = NULL;
        drwav_data_format format;
        format.container = drwav_container_riff;     // <-- drwav_container_riff = normal WAV files, drwav_container_w64 = Sony Wave64.
        format.format = DR_WAVE_FORMAT_PCM;          // <-- Any of the DR_WAVE_FORMAT_* codes.
        format.channels = 1;
        format.sampleRate = wav.sampleRate;
        format.bitsPerSample = 16;

        size_t frameSize = 8 * 20;
        size_t i = 0;
        bool last = false;
        int voicecount = 0;

        for (i = 0; !last; i += frameSize) {

            short* data = pDecodedInterleavedPCMFrames + i;
            size_t len = numberOfSamplesActuallyDecoded - i;
            if (len > frameSize) {
                len = frameSize;
            }
            else {
                last = true;
            }

            int state = dd_vad_process(vad, data, len);
            if (last) {
                state = dd_vad_last(vad);
            }

            if (fragment) {
                drwav_write_pcm_frames(fragment, frameSize, data);
            }

            if (state != laststate) {
                laststate = state;

                if (state) {

                    printf("voice   : %0.2fs\n", (double)(dd_vad_count(vad) - dd_vad_duration(vad)) / wav.sampleRate);

                    char tmp[256] = "\0";
                    snprintf(tmp, sizeof(tmp), "vad_%d.wav", voicecount++);
                    fragment = malloc(sizeof(drwav));
                    if (!drwav_init_file_write(fragment, tmp, &format, NULL)) {
                        free(fragment);
                        fragment = NULL;
                    }

                    if (fragment) {
                        short* first_sample;
                        size_t first_len = 0;
                        short* second_sample;
                        size_t second_len = 0;
                        dd_vad_cachedata(vad, 100, &first_sample, &first_len, &second_sample, &second_len);
                        drwav_write_pcm_frames(fragment, first_len, first_sample);
                        drwav_write_pcm_frames(fragment, second_len, second_sample);
                    }
                }
                else {
                    printf("silence : %0.2fs\n", (double)(dd_vad_count(vad) - dd_vad_duration(vad)) / wav.sampleRate);
                    drwav_uninit(fragment);
                    free(fragment);
                    fragment = NULL;
                }
            }
        }

        if (fragment) {
            drwav_uninit(fragment);
            free(fragment);
        }

        dd_vad_destory(vad);

    }
    else {
        printf("dd_vad_create failed\n");
    }

    free(pDecodedInterleavedPCMFrames);
    drwav_uninit(&wav);
}

void testsad(const char* filename)
{
    drwav wav;
    if (!drwav_init_file(&wav, filename, NULL)) {
        printf("open file failed\n");
        return;
    }

    if (wav.channels != 1) {
        printf("%d channels nonsupport\n", wav.channels);
        drwav_uninit(&wav);
        return;
    }

    drwav_int16* pDecodedInterleavedPCMFrames = malloc(wav.totalPCMFrameCount * wav.channels * sizeof(drwav_int16));
    size_t numberOfSamplesActuallyDecoded = drwav_read_pcm_frames_s16(&wav, wav.totalPCMFrameCount, pDecodedInterleavedPCMFrames);

    DD_SAD* sad = dd_sad_new();

    if (sad) {
        int laststate = 0;

        drwav* fragment = NULL;
        drwav_data_format format;
        format.container = drwav_container_riff;     // <-- drwav_container_riff = normal WAV files, drwav_container_w64 = Sony Wave64.
        format.format = DR_WAVE_FORMAT_PCM;          // <-- Any of the DR_WAVE_FORMAT_* codes.
        format.channels = 1;
        format.sampleRate = wav.sampleRate;
        format.bitsPerSample = 16;

        size_t frameSize = 8 * 20;
        size_t i = 0;

        int voicecount = 0;
        size_t voicepoint = 0;
        bool last = false;

        for (i = 0; !last; i += frameSize) {

            short* data = pDecodedInterleavedPCMFrames + i;
            size_t len = numberOfSamplesActuallyDecoded - i;
            if (len > frameSize) {
                len = frameSize;
            }
            else {
                last = true;
            }


            int state = dd_sad_process(sad, data, len, wav.sampleRate, last);


            if (state != laststate) {
                laststate = state;

                if (state) {

                    voicepoint = dd_sad_count(sad) - dd_sad_duration(sad);
                    printf("voice   : %0.2fs\n", (double)(dd_sad_count(sad) - dd_sad_duration(sad)) / wav.sampleRate);

                    char tmp[256] = "\0";
                    snprintf(tmp, sizeof(tmp), "sad_%d.wav", voicecount++);
                    fragment = malloc(sizeof(drwav));
                    if (!drwav_init_file_write(fragment, tmp, &format, NULL)) {
                        free(fragment);
                        fragment = NULL;
                    }
                }
                else {
                    printf("silence : %0.2fs\n", (double)(dd_sad_count(sad) - dd_sad_duration(sad)) / wav.sampleRate);
                    drwav_uninit(fragment);
                    free(fragment);
                    fragment = NULL;
                }

            }

            if (fragment) {
                drwav_write_pcm_frames(fragment, frameSize, pDecodedInterleavedPCMFrames + voicepoint);
                voicepoint += frameSize;
            }

        }

        if (fragment) {
            drwav_uninit(fragment);
            free(fragment);
        }

        dd_sad_destory(sad);
    }
    else {
        printf("dd_vad_create failed\n");
    }


    free(pDecodedInterleavedPCMFrames);


    drwav_uninit(&wav);
}

int main(int argc, char *argv[])
{
    if (!libsad_init("../libsad/license.json", "../libsad/model")) {
        printf("libsad init failed\n");
        return -0;
    }

    char license[2048] = "\0";
    libsad_license(license, sizeof(license));

    printf("%s\n", license);

    const char *filename="test0.wav";
    if(argc>1){
        filename=argv[1];
    }

    printf("vadtest:%s\n",filename);
    testvad(filename);

    printf("sadtest:%s\n",filename);
    testsad(filename);

    //printf("\nenter exit");
    //getchar();

    libsad_clean();
    return 0;
}

