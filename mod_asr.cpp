/* asr mod. Copyright 2017, xuhuaiyi ALL RIGHTS RESERVED!
    Author: cdevelop@qq.com(wwww.ddrj.com)

    之前写了一个商业模块mod_vad，以及二次开发接口SmartIVR(RESTful API)详细看 http://www.dingdingtong.cn/smartivr/
    很多用户只想测试一下ASR的效果,以及希望自己掌握获取FreeSWITCH语音数据的原理和过程，所以特意写了这个开源项目供大家参考。
    本项目的技术交流可以加群：340129771
    如果需要商业服务和支持可以联系QQ：1280791187，或者微信：cdevelop
    
    本项目是通过阿里云实时语音识别接口把通话中的语音流发送到阿里识别服务器，识别结果通过ESL事件通知业务程序。


*/

#include <switch.h>
#include "NlsClient.h"


 SWITCH_MODULE_LOAD_FUNCTION(mod_asr_load);
 SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_asr_shutdown);
 
 extern "C" {
     SWITCH_MODULE_DEFINITION(mod_asr, mod_asr_load, mod_asr_shutdown, NULL);
 };

 NlsClient nlc;
 NlsSpeechCallback  callback;

typedef struct {

    switch_core_session_t   *session;
    switch_media_bug_t      *bug;

    NlsRequest              *request;
    char                    *id;
    char                    *seceret;
    
    char                    *pcm_filename;
    FILE		    *fd_pcm;

    int                     stop;

} switch_da_t;


void OnResultDataRecved(NlsEvent* str, void* para)
{
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, " OnResultDataRecved %s %s\n", str->getId().c_str(), str->getResponse().c_str());
    
    /*
    {"finish":1,"request_id":"13d5daad670947a5a7a12c581f5dc8b7","status_code":200,"version":"2.0"}
    {"finish":0,"request_id":"13d5daad670947a5a7a12c581f5dc8b7","result":{"sentence_id":10,"begin_time":50960,"end_time":52915,"status_code":0,"text":"电话挂掉吧拜拜"},"status_code":200,"version":"2.0"}
    {"finish":0,"request_id":"13d5daad670947a5a7a12c581f5dc8b7","result":{"sentence_id":10,"begin_time":50960,"end_time":-1,"status_code":1,"text":"电话挂"},"status_code":200,"version":"2.0"}
    {"finish":0,"request_id":"76a6e062ec9b4db68f5f6016c7f700aa","result":{"sentence_id":1,"begin_time":1100,"end_time":3505,"status_code":0,"text":"哎，好的一订单大概多少钱啊"},"status_code":200,"version":"2.0"}
    {"finish":0,"request_id":"76a6e062ec9b4db68f5f6016c7f700aa","result":{"sentence_id":1,"begin_time":1100,"end_time":-1,"status_code":1,"text":"哎，好的一订单大概多少钱"},"status_code":200,"version":"2.0"}
    */

    switch_event_t *event = NULL;
    if (switch_event_create(&event, SWITCH_EVENT_CUSTOM) == SWITCH_STATUS_SUCCESS) {

        event->subclass_name = strdup("asr");
        switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Event-Subclass", event->subclass_name);
        switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "ASR-Response", str->getResponse().c_str());
        switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Channel", str->getId().c_str());
        switch_event_fire(&event);
    }

}

void OnOperationFailed(NlsEvent* str, void* para)
{
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, " OnOperationFailed %s %s\n", str->getId().c_str(), str->getErrorMessage().c_str());

    switch_event_t *event = NULL;
    if (switch_event_create(&event, SWITCH_EVENT_CUSTOM) == SWITCH_STATUS_SUCCESS) {

        event->subclass_name = strdup("asr");
        switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Event-Subclass", event->subclass_name);
        switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "ASR-Error", str->getErrorMessage().c_str());
        switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Channel", str->getId().c_str());
        switch_event_fire(&event);
    }


}

void OnChannelCloseed(NlsEvent* str, void* para)
{
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, " OnChannelCloseed %s %s\n", str->getId().c_str(), str->getResponse().c_str());

    /*
    WebSocketSession [80066], appkey [nls-service-realtime-8k], uuid [291fbd2577d2474081e68abadb9708d5], orgCode [1613207423
    */
    switch_event_t *event = NULL;
    if (switch_event_create(&event, SWITCH_EVENT_CUSTOM) == SWITCH_STATUS_SUCCESS) {

        event->subclass_name = strdup("asr");
        switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Event-Subclass", event->subclass_name);
        switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "ASR-Close", str->getResponse().c_str());
        switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Channel", str->getId().c_str());
        switch_event_fire(&event);
    }

}



static switch_bool_t asr_callback(switch_media_bug_t *bug, void *user_data, switch_abc_type_t type)
{
    switch_da_t *pvt = (switch_da_t *)user_data;
    switch_channel_t *channel = switch_core_session_get_channel(pvt->session);

    switch (type) {
    case SWITCH_ABC_TYPE_INIT:
        {


#ifdef _WIN32
            pvt->request = nlc.createRealTimeRequest(&callback, "config-realtime.txt");
#else
            pvt->request = nlc.createRealTimeRequest(&callback, "/etc/config-realtime.txt");
#endif

            if (pvt->request) {

                pvt->request->SetParam("Id", switch_channel_get_name(channel));
                pvt->request->Authorize(pvt->id, pvt->seceret);
                if (pvt->request->Start() < 0)
                {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "ASR Start Failed channel:%s\n", switch_channel_get_name(channel));

                    delete pvt->request;
                    pvt->request = NULL;
                    return SWITCH_FALSE;

                }
                else {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "ASR Start Succeed channel:%s\n", switch_channel_get_name(channel));
                }
            }
        }
        break;
    case SWITCH_ABC_TYPE_CLOSE:
        {
        if (pvt->request) {

                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "ASR Stop Succeed channel:%s\n", switch_channel_get_name(channel));

                pvt->request->Stop();
                delete pvt->request;
                pvt->request = NULL;
            }
        }
        break;

    case SWITCH_ABC_TYPE_READ_REPLACE:
        {
  
            switch_frame_t *frame;
            if ((frame = switch_core_media_bug_get_read_replace_frame(bug))) {
                char*frame_data = (char*)frame->data;
                int frame_len = frame->datalen;
                switch_core_media_bug_set_read_replace_frame(bug, frame);
                
                if (frame->channels != 1)
                {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "nonsupport channels:%d!\n",frame->channels);
                    return SWITCH_FALSE;
                }

                if (pvt->request) {
			fwrite(frame_data, 1, frame_len, pvt->fd_pcm);

                    if (pvt->request->SendAudio(frame_data, frame_len) <= 0) {
                        return SWITCH_FALSE;
                    }
                }
            }
 
        }
        break;
    default: break;
    }

    return SWITCH_TRUE;
}


SWITCH_STANDARD_APP(stop_asr_session_function)
{
    switch_da_t *pvt;
    switch_channel_t *channel = switch_core_session_get_channel(session);

    if ((pvt = (switch_da_t*)switch_channel_get_private(channel, "asr"))) {

        switch_channel_set_private(channel, "asr", NULL);
        switch_core_media_bug_remove(session, &pvt->bug);
        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "%s Stop ASR\n", switch_channel_get_name(channel));

    }
}


SWITCH_STANDARD_APP(start_asr_session_function)
{
    switch_channel_t *channel = switch_core_session_get_channel(session);

    switch_status_t status;
    switch_da_t *pvt;
    switch_codec_implementation_t read_impl;
    memset(&read_impl, 0, sizeof(switch_codec_implementation_t));

    char *argv[2] = { 0 };
    int argc;
    char *lbuf = NULL;


    if (!zstr(data) && (lbuf = switch_core_session_strdup(session, data))
        && (argc = switch_separate_string(lbuf, ' ', argv, (sizeof(argv) / sizeof(argv[0])))) >= 2) {

        switch_core_session_get_read_impl(session, &read_impl);

        if (!(pvt = (switch_da_t*)switch_core_session_alloc(session, sizeof(switch_da_t)))) {
            return;
        }

        pvt->stop = 0;
        pvt->session = session;
        pvt->id = argv[0];
        pvt->seceret = argv[1];



        char pcm_filename[] = "/tmp/testpcm/test.pcm"; 
        pvt->pcm_filename = pcm_filename;

/******test add file ******/
        FILE *fd_pcm = fopen(pvt->pcm_filename ,"wb");
        pvt->fd_pcm  = fd_pcm;
        char pcm_test1[6] = "test1"; 
        char pcm_test2[6] = "test2"; 
//	fwrite(pcm_test1, sizeof(char) , 5, pvt->fd_pcm);
//	fwrite(pcm_test2, sizeof(char) , 5, pvt->fd_pcm);
//	fclose(fd_pcm);

        if ((status = switch_core_media_bug_add(session, "asr", NULL,
            asr_callback, pvt, 0, SMBF_READ_REPLACE | SMBF_NO_PAUSE | SMBF_ONE_ONLY, &(pvt->bug))) != SWITCH_STATUS_SUCCESS) {
            return;
        }

        switch_channel_set_private(channel, "asr", pvt);
        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "%s Start ASR\n", switch_channel_get_name(channel));
    }
    else {
        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "%s id or secret can not be empty\n", switch_channel_get_name(channel));
    }

    
}






SWITCH_MODULE_LOAD_FUNCTION(mod_asr_load)
{
    switch_application_interface_t *app_interface;

    *module_interface = switch_loadable_module_create_module_interface(pool, modname);

    SWITCH_ADD_APP(app_interface, "start_asr", "asr", "asr",start_asr_session_function, "", SAF_MEDIA_TAP);
    SWITCH_ADD_APP(app_interface, "stop_asr", "asr", "asr", stop_asr_session_function, "", SAF_NONE);

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, " asr_load\n");


    callback.setOnMessageReceiced(OnResultDataRecved);
    callback.setOnOperationFailed(OnOperationFailed);
    callback.setOnChannelClosed(OnChannelCloseed);


    return SWITCH_STATUS_SUCCESS;
}


 SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_asr_shutdown)
{
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, " asr_shutdown\n");

    return SWITCH_STATUS_SUCCESS;
}
