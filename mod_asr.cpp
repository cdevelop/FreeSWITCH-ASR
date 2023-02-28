/* asr mod. Copyright 2023, xuhuaiyi ALL RIGHTS RESERVED!
    Author: cdevelop@qq.com(wwww.ddrj.com)


    顶顶通VAD（支持噪音人声识别）集成FreeSWITCH演示程序
    本程序包的授权文件是10并发1个月的体验授权，仅用于体验和测试使用，商业使用请联系 顶顶通购买正式授权
    联系方式 微信 cdevelop 网站 www.ddrj.com

*/

#include <switch.h>
#include <switch_curl.h>
#include "libsad/libsad.h"

 SWITCH_MODULE_LOAD_FUNCTION(mod_asr_load);
 SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_asr_shutdown);
 
 extern "C" {
     SWITCH_MODULE_DEFINITION(mod_asr, mod_asr_load, mod_asr_shutdown, NULL);
 };



 //本例子使用多方asr接口，注册地址 http://ai.hiszy.com/#/user/register?code=RK9RD7W 注册后可以联系ASR服务商微信 aohu6789 获取免费次数
 //

 static char* g_token = NULL;
 time_t g_expir = 0;
 static switch_mutex_t *g_lock = NULL;


 static size_t curlrecv(char* buffer, size_t size, size_t nitems, void* outstream)
 {
     switch_buffer* response = (switch_buffer*)outstream;
     switch_buffer_write(response, buffer, size * nitems);
     return size * nitems;
 }

 static const char* getasrtoken()
 {
     char* ret = NULL;
     switch_mutex_lock(g_lock);

     if (g_expir - switch_epoch_time_now(0) < 500) {

         char* appKey = switch_core_get_variable_dup("appKey");
         char* appSecret = switch_core_get_variable_dup("appSecret");
         char buffer[1024] = "\0";
         int len = snprintf(buffer, sizeof(buffer), "{\"appKey\":\"%s\",\"appSecret\":\"%s\"}", appKey, appSecret);
         free(appKey);
         free(appSecret);
         switch_buffer* response;
         switch_buffer_create_dynamic(&response, 1024, 1024, 1024 * 4);
         switch_CURL* curl_handle = switch_curl_easy_init();
         switch_curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0);
         switch_curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
         switch_curl_easy_setopt(curl_handle, CURLOPT_HTTPPOST, 1);
         switch_curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT_MS, 2000);
         switch_curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, 5000);
         switch_curl_easy_setopt(curl_handle, CURLOPT_URL, "http://openapi.duofangai.com/server/api/auth/get-token");
         switch_curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, response);
         switch_curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curlrecv);
         switch_curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, (void*)&buffer);
         switch_curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, len);
         char tmp[512];
         switch_curl_slist_t* headerlist = NULL;
         snprintf(tmp, sizeof(tmp), "Content-Type: %s", "application/json");
         headerlist = switch_curl_slist_append(headerlist, tmp);
         //snprintf(tmp, sizeof(tmp), "Content-Length: %zd", datalength);
         //headerlist = switch_curl_slist_append(headerlist, tmp);
         switch_curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headerlist);
         long httpRes = 0;
         switch_curl_easy_perform(curl_handle);
         switch_curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &httpRes);
         switch_curl_slist_free_all(headerlist);
         switch_curl_easy_cleanup(curl_handle);
         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "get token:%.*s.\n", (int)switch_buffer_inuse(response), (char*)switch_buffer_get_head_pointer(response));
         if (httpRes == 200) {
             cJSON* json = cJSON_Parse((const char*)switch_buffer_get_head_pointer(response));
             if (json) {
                 cJSON* code = cJSON_GetObjectItem(json, "code");
                 if (code && code->valueint == 200) {
                     cJSON* data = cJSON_GetObjectItem(json, "data");
                     switch_strdup(g_token, data->valuestring);
                     g_expir = switch_epoch_time_now(0) * 24 * 3600;
                 }
                 cJSON_Delete(json);
             }
         }



         switch_buffer_destroy(&response);
     }

     ret = g_token;

     switch_mutex_unlock(g_lock);

     return ret ? ret : "";
 }


 static const char* execasr(const unsigned char*data,size_t len,char *buffer,size_t size)
 {

     switch_buffer* response;
     switch_buffer_create_dynamic(&response, 1024, 1024, 1024 * 4);

     switch_CURL* curl_handle = switch_curl_easy_init();
     switch_curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0);
     switch_curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
     switch_curl_easy_setopt(curl_handle, CURLOPT_HTTPPOST, 1);
     switch_curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT_MS, 2000);
     switch_curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, 5000);

     switch_curl_easy_setopt(curl_handle, CURLOPT_URL, "http://openapi.duofangai.com/open/asr/sentence/v1/recognition");
     switch_curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, response);
     switch_curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curlrecv);
     switch_curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, (void*)data);
     switch_curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, len);


     char tmp[1024];
     switch_curl_slist_t* headerlist = NULL;
     snprintf(tmp, sizeof(tmp), "Content-Length: %zd", len);
     headerlist = switch_curl_slist_append(headerlist, tmp);
     snprintf(tmp, sizeof(tmp), "Content-Type: %s", "application/octet-stream");
     headerlist = switch_curl_slist_append(headerlist, tmp);
     snprintf(tmp, sizeof(tmp), "aitoken: %s", getasrtoken());
     headerlist = switch_curl_slist_append(headerlist, tmp);
     snprintf(tmp, sizeof(tmp), "format: pcm");
     headerlist = switch_curl_slist_append(headerlist, tmp);
     snprintf(tmp, sizeof(tmp), "pct: true");
     headerlist = switch_curl_slist_append(headerlist, tmp);
     snprintf(tmp, sizeof(tmp), "itn: true");
     headerlist = switch_curl_slist_append(headerlist, tmp);
     switch_curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headerlist);


     long httpRes = 0;
     switch_curl_easy_perform(curl_handle);
     switch_curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &httpRes);
     switch_curl_slist_free_all(headerlist);
     switch_curl_easy_cleanup(curl_handle);


     switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "asr result:%.*s.\n", (int)switch_buffer_inuse(response), (char*)switch_buffer_get_head_pointer(response));

     if (httpRes == 200) {
         cJSON* json = cJSON_Parse((const char*)switch_buffer_get_head_pointer(response));
         if (json) {
             cJSON* code = cJSON_GetObjectItem(json, "code");
             if (code && code->valueint != 200000) {
                 cJSON* message = cJSON_GetObjectItem(json, "message");
                 if (message) {
                     snprintf(buffer, size, "%s", message->valuestring);
                 }
             }
             else {
                 cJSON* data = cJSON_GetObjectItem(json, "data");
                 if (data) {
                     snprintf(buffer, size, "%s", data->valuestring);
                 }
             }
             cJSON_Delete(json);
         }
     }

     switch_buffer_destroy(&response);

     return buffer;

 }


SWITCH_STANDARD_APP(play_and_asr_session_function)
{
    switch_channel_t* channel = switch_core_session_get_channel(session);
    switch_frame_t* read_frame;
    switch_status_t status = SWITCH_STATUS_FALSE;
    switch_codec_t raw_codec = { 0 };
    int16_t* abuf = NULL;
    switch_frame_t write_frame = { 0 };
    switch_file_handle_t play_fh = { 0 };
    switch_codec_implementation_t read_impl;
    int play = 0;
    unsigned int maxwaittime = 5000;
    DD_VAD_T* vad = NULL;
    switch_buffer* speakbuffer = NULL;
    unsigned int maxspeaktime = 60000;
    switch_bool_t allowbreak = SWITCH_FALSE;
    switch_time_t speakstarttime = 0;
    switch_time_t waitstarttime = 0;
    int laststate = 0;

    memset(&read_impl, 0, sizeof(read_impl));
    switch_core_session_get_read_impl(session, &read_impl);


    char* argv[20] = {0};
    char* lbuf = switch_core_session_strdup(session, data);
    switch_separate_string(lbuf, ' ', argv, (sizeof(argv) / sizeof(argv[0])));



    if (!zstr(argv[0])) {
        if (switch_core_file_open(&play_fh,
            argv[0],
            read_impl.number_of_channels,
            read_impl.actual_samples_per_second, SWITCH_FILE_FLAG_READ | SWITCH_FILE_DATA_SHORT, NULL) != SWITCH_STATUS_SUCCESS) {
            switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "Failure opening playback file %s.\n", argv[0]);
            waitstarttime = switch_micro_time_now();
        }
        else {
            play = 1;
        }
    }

    if (!zstr(argv[1])) {
        maxwaittime = switch_atoui(argv[1]);
    }

    if (!zstr(argv[2])) {
        maxspeaktime = switch_atoui(argv[2]);
    }

    maxwaittime *= 1000;
    maxspeaktime *= 1000;

    if (!zstr(argv[3])) {
        allowbreak = switch_true(argv[3]);
    }

    abuf = (int16_t*)malloc(SWITCH_RECOMMENDED_BUFFER_SIZE);
    write_frame.data = abuf;
    write_frame.buflen = SWITCH_RECOMMENDED_BUFFER_SIZE;


    if (switch_core_codec_init(&raw_codec,
        "L16",
        NULL,
        NULL,
        read_impl.actual_samples_per_second,
        read_impl.microseconds_per_packet / 1000,
        1, SWITCH_CODEC_FLAG_ENCODE | SWITCH_CODEC_FLAG_DECODE,
        NULL, switch_core_session_get_pool(session)) != SWITCH_STATUS_SUCCESS) {

        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "Failed to initialize L16 codec.\n");
        status = SWITCH_STATUS_FALSE;
        goto end;
    }

    write_frame.codec = &raw_codec;


    switch_core_session_set_read_codec(session, &raw_codec);

    vad = dd_vad_create(read_impl.actual_samples_per_second, 20, 1000, DD_VAD_MODE_AGGRESSIVE, 100, 800, 100, 0.8);
    if (!vad) {
        goto end;
    }


    while (switch_channel_ready(channel)) {

        status = switch_core_session_read_frame(session, &read_frame, SWITCH_IO_FLAG_NONE, 0);

        if (!SWITCH_READ_ACCEPTABLE(status)) {
            switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "Failed to read frame.\n");
            break;
        }

        switch_size_t olen = raw_codec.implementation->samples_per_packet;

        if (play == 1) {
            write_frame.datalen = (uint32_t)(olen * sizeof(int16_t) * play_fh.channels);
            if (switch_core_file_read(&play_fh, abuf, &olen) != SWITCH_STATUS_SUCCESS) {
                switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "play done file:%s.\n", argv[0]);
                play = 2;
                waitstarttime= switch_micro_time_now();
            }
        }
        else {
            write_frame.datalen = (uint32_t)(olen * sizeof(int16_t));
            memset(write_frame.data, 0, write_frame.buflen);
        }

        write_frame.samples = (uint32_t)olen;
        if ((status = switch_core_session_write_frame(session, &write_frame, SWITCH_IO_FLAG_NONE, 0)) != SWITCH_STATUS_SUCCESS) {
            switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "Failed to write frame.\n");
            break;
        }

        if (read_frame->datalen > 2) {

            int state = dd_vad_process(vad, (const short*)read_frame->data, read_frame->datalen / 2);
            if (state != laststate) {
                laststate = state;

                if (state) {
                    switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "start speak.\n");
                    speakstarttime = switch_micro_time_now();

                    if (allowbreak && play == 1) {
                        play = 2;
                        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "play break file:%s.\n", argv[0]);
                    }

                    short* first_sample;
                    size_t first_len = 0;
                    short* second_sample;
                    size_t second_len = 0;
                    dd_vad_cachedata(vad, 200, &first_sample, &first_len, &second_sample, &second_len);
                    switch_buffer_create_dynamic(&speakbuffer, 10 * read_impl.actual_samples_per_second * 2, 10 * read_impl.actual_samples_per_second * 2, maxspeaktime / 1000 * read_impl.actual_samples_per_second * 2);
                    switch_buffer_write(speakbuffer, first_sample, first_len * 2);
                    switch_buffer_write(speakbuffer, second_sample, second_len * 2);
                }
                else {
                    switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "stop speak.\n");
                    break;
                }
            }

            if (speakbuffer) {
                switch_buffer_write(speakbuffer, read_frame->data, read_frame->datalen);
            }


        }

        if (laststate == 0)
        {
            if (play != 1 && dd_vad_status(vad) == 0 && switch_micro_time_now() - waitstarttime > maxwaittime) {
                switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "wait speak timeout.\n");
                break;
            }
        }
        else if (switch_micro_time_now()- speakstarttime > maxspeaktime) {
            switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "speak time too long.\n");
            break;
        }

    }

    switch_core_session_reset(session, SWITCH_FALSE, SWITCH_TRUE);
    switch_core_codec_destroy(&raw_codec);

end:
    if (play) {
        switch_core_file_close(&play_fh);
    }
  

    if (speakbuffer) {

        char asr_result[1024] = "\0";
        execasr((const unsigned char*)switch_buffer_get_head_pointer(speakbuffer), switch_buffer_inuse(speakbuffer), asr_result, sizeof(asr_result) - 1);

        switch_channel_set_variable(channel, "asr_result", asr_result);

        if (!zstr(argv[4])) {

            switch_file_handle_t record_fh = { 0 };
            if (switch_core_file_open(&record_fh,
                argv[4],
                read_impl.number_of_channels,
                read_impl.actual_samples_per_second, SWITCH_FILE_FLAG_WRITE | SWITCH_FILE_WRITE_OVER | SWITCH_FILE_DATA_SHORT, NULL) != SWITCH_STATUS_SUCCESS) {
                switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "Failure createing record file %s.\n", argv[4]);
            }
            else {
                switch_size_t writelen = switch_buffer_inuse(speakbuffer);
                switch_core_file_write(&record_fh, switch_buffer_get_head_pointer(speakbuffer), &writelen);
                switch_core_file_close(&record_fh);
            }
        }

        switch_buffer_destroy(&speakbuffer);
    }
    else {
        switch_channel_set_variable(channel, "asr_result", "silence");
    }

    if (abuf) {
        free(abuf);
    }

    if (vad) {
        dd_vad_destory(vad);
    }
   
}






SWITCH_MODULE_LOAD_FUNCTION(mod_asr_load)
{
    switch_status_t status = SWITCH_STATUS_FALSE;
    switch_application_interface_t *app_interface;

    if (libsad_init()) {
        *module_interface = switch_loadable_module_create_module_interface(pool, modname);
        SWITCH_ADD_APP(app_interface, "play_and_asr", "asr", "asr", play_and_asr_session_function, "playfilename waittime maxspeaktime allowbreak recordfilename", SAF_MEDIA_TAP);

        status = SWITCH_STATUS_SUCCESS;
        char license[2048] = "\0";
        libsad_license(license, sizeof(license));

        switch_mutex_init(&g_lock, SWITCH_MUTEX_NESTED, pool);

        getasrtoken();

        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, " mod_asr load license:\n%s\n", license);
    }
    else {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "libsad_init failed\n");
    }


    return status;
}


 SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_asr_shutdown)
{
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_asr shutdown\n");

    libsad_clean();

    switch_mutex_destroy(g_lock);

    return SWITCH_STATUS_SUCCESS;
}
