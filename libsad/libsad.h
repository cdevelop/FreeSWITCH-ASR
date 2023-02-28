/*!
����ͨVAD������SAD(��������ʶ��)���� 1.0
�����������Ȩ�ļ���10����1���µ�������Ȩ������������Ͳ���ʹ�ã���ҵʹ������ϵ ����ͨ������ʽ��Ȩ
��ϵ��ʽ ΢�� cdevelop ��վ www.ddrj.com


���ӿ��ṩ��VAD��SAD������ʶ�� 2���ӿ�
SAD�ӿ�ֻʹ���������㷨���д���
VAD�ӿ����noise_filter_level���ô���0.8�ͻ�����SAD(�������㷨)���Ż�VADЧ�����������0.8������ͨ��VAD��

SAD�ӿ���ҪCPU�Ƚ϶�
VAD�ӿ�(noise_filter_level���ô���0.8)��ֻ��VAD��⵽�������������������㷨���������������������Խ�Լ������CPU��Ҳ���Դﵽ�͵���ʹ��SAD�ӿ����Ƶ�Ч����


*/


#ifndef _DD_VAD_H_
#define _DD_VAD_H_

#include <stddef.h>
#include "stdbool.h"


#if !defined(_WIN32) || defined(SAD_STATIC)
#define SAD_API
#else
#ifdef LIBSAD_EXPORTS
#define SAD_API __declspec(dllexport)
#else
#define SAD_API __declspec(dllimport)
#endif
#endif



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    SAD_API int libsad_init();
    SAD_API void libsad_clean();
    SAD_API void libsad_license(char *buffer,size_t len);

   
    typedef struct DD_SAD_T DD_SAD;
    SAD_API DD_SAD* dd_sad_new();
    SAD_API void dd_sad_destory(DD_SAD* sad);
    SAD_API int dd_sad_frame_size(DD_SAD* sad, int freq);
    SAD_API int dd_sad_frames_right_context(DD_SAD* sad, int freq);

    //0�������� 1������
    SAD_API int dd_sad_process(DD_SAD* sad, const short* samples, size_t len, int freq, bool last);
    //�����Ѿ��ύ������������
    SAD_API size_t dd_sad_count(DD_SAD* sad);
    //���ص�ǰ״̬������������
    SAD_API size_t dd_sad_duration(DD_SAD* sad);
    //����δ���������
    SAD_API size_t dd_sad_pending(DD_SAD* sad);
    //��δ��������ݴ�����ɲ����ͷ�sad
    SAD_API int dd_sad_finish(DD_SAD* sad, size_t* sad_count, size_t* voice_begin, size_t* voice_end);




    typedef enum {
        DD_VAD_MODE_QUALITY = 0,
        DD_VAD_MODE_LOWBITRATE = 1,
        DD_VAD_MODE_AGGRESSIVE = 2,
        DD_VAD_MODE_VERYAGGRESSIVE = 3
    } DD_VAD_MODE;

    typedef struct DD_VAD_T DD_VAD;

    //frame_time ���� 10��20��30�е�һ��
    //min_activity_time ����ʱ�������������״̬
    //min_silence_time ����ʱ������������뾲��״̬
    //noise_filter_level 0-1 ��0.8���ϻ�������������ʶ���㷨�Ż�VADЧ����
    SAD_API DD_VAD* dd_vad_create(int freq_hz, int frame_time, size_t cache_ms, DD_VAD_MODE mode, int min_activity_time, int min_silence_time, int voice_threshold, double noise_filter_level);
    SAD_API void dd_vad_destory(DD_VAD* vad);
    SAD_API void dd_vad_reset(DD_VAD* vad, DD_VAD_MODE mode, int min_activity_time, int min_silence_time, int threshold, double filter);
    SAD_API void dd_vad_reset_default(DD_VAD* vad);

    //0:����  1:���� -1:����
    SAD_API int dd_vad_process(DD_VAD* vad, const short* samples, size_t len);
    //��δ�������ݴ������
    SAD_API int dd_vad_last(DD_VAD* vad);


    //����vad״̬ 0 ���� 1 δ��[SAD������] 2����
    SAD_API int dd_vad_status(DD_VAD* vad);
    //�����ύ�����������
    SAD_API size_t dd_vad_count(DD_VAD* vad);
    //���ص�ǰ״̬�Ĳ�����
    SAD_API size_t dd_vad_duration(DD_VAD* vad);
    //������һ��״̬�Ĳ�����
    SAD_API size_t dd_vad_prev_duration(DD_VAD* vad);
    //���ص�ǰ״̬��δȷ��������
    SAD_API size_t dd_vad_pend_duration(DD_VAD* vad);
    //boottime����ǰ����ٺ������������
    //first_len,second_len Ϊ���������������ֽ�����
    SAD_API size_t dd_vad_cachedata(DD_VAD* vad, size_t boottime, short** first_sample, size_t* first_len, short** second_sample, size_t* second_len);
   



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif