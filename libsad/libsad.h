/*!
* \file dd_vad.h
*
* \author 徐怀移 <cdevelop@qq.com>
* \date 2019/08/10
* \brief vad接口
*
*/


/*!
顶顶通VAD，集成SAD(噪音人声识别)程序 1.0
本程序包的授权文件是10并发1个月的体验授权，仅用于体验和测试使用，商业使用请联系 顶顶通购买正式授权
联系方式 微信 cdevelop 网站 www.ddrj.com


本接口提供了VAD和SAD（人声识别） 2个接口
SAD接口只使用神经网络算法进行处理。
VAD接口如果noise_filter_level设置大于0.8就会启用SAD(神经网络算法)来优化VAD效果，如果低于0.8就是普通的VAD。

SAD接口需要CPU比较多
VAD接口(noise_filter_level设置大于0.8)，只有VAD检测到声音，才启用神经网络算法分析是人身还是噪音，可以节约大量的CPU，也可以达到和单独使用SAD接口类似的效果。
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

    SAD_API int libsad_init(const char *license_file, const char *model_dir);
    SAD_API void libsad_clean();
    SAD_API void libsad_license(char *buffer,size_t len);


    typedef struct DD_SAD_T DD_SAD;
    SAD_API DD_SAD* dd_sad_new(int freq);
    SAD_API void dd_sad_destory(DD_SAD* sad);
    SAD_API int dd_sad_frame_size(DD_SAD* sad);
    SAD_API int dd_sad_frames_right_context(DD_SAD* sad);

    //0：非人声 1：人声
    SAD_API int dd_sad_process(DD_SAD* sad, const short* samples, size_t len,  bool last);
    //返回已经提交处理样本数量
    SAD_API size_t dd_sad_count(DD_SAD* sad);
    //返回当前状态持续样本数量
    SAD_API size_t dd_sad_duration(DD_SAD* sad);
    //返回未处理的样本
    SAD_API size_t dd_sad_pending(DD_SAD* sad);
    //把未处理的数据处理完成并且释放sad
    SAD_API int dd_sad_finish(DD_SAD* sad, size_t* sad_count, size_t* voice_begin, size_t* voice_end);




    typedef enum {
        DD_VAD_MODE_QUALITY = 0,
        DD_VAD_MODE_LOWBITRATE = 1,
        DD_VAD_MODE_AGGRESSIVE = 2,
        DD_VAD_MODE_VERYAGGRESSIVE = 3
    } DD_VAD_MODE;

    typedef struct DD_VAD_T DD_VAD;

    //frame_time 必须 10，20，30中的一个
    //min_activity_time 声音时间大于它，进入活动状态
    //min_silence_time 静音时间大于它，进入静音状态
    //noise_filter_level 0-1 建议 0.5
    SAD_API DD_VAD* dd_vad_create(int freq_hz, int frame_time, size_t cache_ms, DD_VAD_MODE mode, int min_activity_time, int min_silence_time, int voice_threshold, double noise_filter_level);
    SAD_API void dd_vad_destory(DD_VAD* vad);
    SAD_API void dd_vad_reset(DD_VAD* vad, DD_VAD_MODE mode, int min_activity_time, int min_silence_time, int threshold, double filter);
    SAD_API void dd_vad_reset_default(DD_VAD* vad);

    //0:静音  1:声音 -1:错误
    SAD_API int dd_vad_process(DD_VAD* vad, const short* samples, size_t len);
    //把未决的数据处理完成
    SAD_API int dd_vad_last(DD_VAD* vad);


    //返回vad状态 0 静音 1 未决[SAD分析中] 2声音
    SAD_API int dd_vad_status(DD_VAD* vad);
    //返回提交处理采样总数
    SAD_API size_t dd_vad_count(DD_VAD* vad);
    //返回当前状态的采样数
    SAD_API size_t dd_vad_duration(DD_VAD* vad);
    //返回上一个状态的采样数
    SAD_API size_t dd_vad_prev_duration(DD_VAD* vad);
    //返回当前状态的未确定采样数
    SAD_API size_t dd_vad_pend_duration(DD_VAD* vad);
    //boottime声音前面多少毫秒的声音返回
    //first_len,second_len 为样本个数，不是字节数。
    SAD_API size_t dd_vad_cachedata(DD_VAD* vad, size_t boottime, short** first_sample, size_t* first_len, short** second_sample, size_t* second_len);
   


    SAD_API int dd_vad_exec(int freq_hz, int frame_time, DD_VAD_MODE mode, int min_activity_time, int min_silence_time, int voice_threshold, double noise_filter_level, const short* samples, size_t len, size_t** result);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif