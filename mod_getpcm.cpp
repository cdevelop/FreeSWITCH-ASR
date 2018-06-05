#include <switch.h>


 SWITCH_MODULE_LOAD_FUNCTION(mod_getpcm_load);
 SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_getpcm_shutdown);
 
 extern "C" {
     SWITCH_MODULE_DEFINITION(mod_getpcm, mod_getpcm_load, mod_getpcm_shutdown, NULL);
 };


typedef struct {

    switch_core_session_t   *session;
    switch_media_bug_t      *bug;

    
    char                    *pcm_filename;
    FILE		    *fd_pcm;

    int                     stop;

} switch_da_t;


static switch_bool_t getpcm_callback(switch_media_bug_t *bug, void *user_data, switch_abc_type_t type)
{
    switch_da_t *pvt = (switch_da_t *)user_data;
    switch_channel_t *channel = switch_core_session_get_channel(pvt->session);

    switch (type) {
    case SWITCH_ABC_TYPE_INIT:
        {
       		 break;
        }
    case SWITCH_ABC_TYPE_CLOSE:
        {
		fclose(pvt->fd_pcm);
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

			fwrite(frame_data, 1, frame_len, pvt->fd_pcm);

                }
        }
        break;
    default: break;
    }

    return SWITCH_TRUE;
}


SWITCH_STANDARD_APP(stop_getpcm_session_function)
{
    switch_da_t *pvt;
    switch_channel_t *channel = switch_core_session_get_channel(session);

    if ((pvt = (switch_da_t*)switch_channel_get_private(channel, "getpcm"))) {

        switch_channel_set_private(channel, "getpcm", NULL);
        switch_core_media_bug_remove(session, &pvt->bug);
	fclose(pvt->fd_pcm);
        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "%s Stop GETPCM\n", switch_channel_get_name(channel));

    }
}


SWITCH_STANDARD_APP(start_getpcm_session_function)
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


        char pcm_filename[] = "/tmp/testpcm/test.pcm"; 
        pvt->pcm_filename = pcm_filename;

        FILE *fd_pcm = fopen(pvt->pcm_filename ,"wb");
        pvt->fd_pcm  = fd_pcm;
        char pcm_test1[6] = "test1"; 
        char pcm_test2[6] = "test2"; 

        if ((status = switch_core_media_bug_add(session, "getpcm", NULL,
            getpcm_callback, pvt, 0, SMBF_READ_REPLACE | SMBF_NO_PAUSE | SMBF_ONE_ONLY, &(pvt->bug))) != SWITCH_STATUS_SUCCESS) {
            return;
        }

        switch_channel_set_private(channel, "getpcm", pvt);
        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "%s Start GETPCM\n", switch_channel_get_name(channel));
    }
    else {
        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "%s id or secret can not be empty\n", switch_channel_get_name(channel));
    }

    
}


SWITCH_MODULE_LOAD_FUNCTION(mod_getpcm_load)
{
    switch_application_interface_t *app_interface;

    *module_interface = switch_loadable_module_create_module_interface(pool, modname);

    SWITCH_ADD_APP(app_interface, "start_getpcm", "getpcm", "getpcm",start_getpcm_session_function, "", SAF_MEDIA_TAP);
    SWITCH_ADD_APP(app_interface, "stop_getpcm", "getpcm", "getpcm", stop_getpcm_session_function, "", SAF_NONE);

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, " getpcm_load\n");


    return SWITCH_STATUS_SUCCESS;
}


 SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_getpcm_shutdown)
{
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, " getpcm_shutdown\n");

    return SWITCH_STATUS_SUCCESS;
}
