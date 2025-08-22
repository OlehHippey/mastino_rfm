/*
 * otaflash.c
 *
 *  Created on: 4 трав. 2025 р.
 *      Author: voltekel
 */

 
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_encrypted_img.h"
//#include "esp_encrypted_img.h"
#include "esp_mac.h"
#include "esp_tls.h"


#include "global.h"
#include "mainfunc.h"
#include "function.h"

#define CONFIG_OTA_ATTEMPTS 5
#define CONFIG_EXAMPLE_OTA_RECV_TIMEOUT 2000

static const char *TAG = "OTA";

extern const char rsa_private_pem_start[] asm("_binary_key_tsall_pem_start");
extern const char rsa_private_pem_end[]   asm("_binary_key_tsall_pem_end");


void pre_encrypted_ota_task(void *pvParameter);
//--------------------------------------------------------------------------------------------------------------
void accept_ota(void)
{
	esp_ota_mark_app_valid_cancel_rollback();
}
//--------------------------------------------------------------------------------------------------------------

void not_accept_ota(void)
{
	esp_ota_mark_app_invalid_rollback_and_reboot();
}
//--------------------------------------------------------------------------------------------------------------

void update_flash_ota(void)
{
	xTaskCreate(pre_encrypted_ota_task, "pre_encrypted_ota_task", 1024*8, NULL, 6, NULL);
	return;
	char url[256];
	sprintf(url,"%s%s",URL_MASTINO_SITE,URL_FIRMWARE);
	esp_http_client_config_t cfgHTTPS;
	memset(&cfgHTTPS, 0, sizeof(cfgHTTPS));
	cfgHTTPS.url = url;
	cfgHTTPS.skip_cert_common_name_check = false;
	cfgHTTPS.use_global_ca_store = false;
    cfgHTTPS.crt_bundle_attach = esp_crt_bundle_attach;
//	cfgHTTPS.cert_pem = ota_pem_start;
	#if ESP_IDF_VERSION_MAJOR >= 5
	esp_https_ota_config_t cfgOTA;
	memset(&cfgOTA, 0, sizeof(cfgOTA));
	cfgOTA.http_config = &cfgHTTPS;
	#endif // ESP_IDF_VERSION_MAJOR
	uint8_t tryUpdate = 0;
	esp_err_t err = ESP_OK;
	do {
		tryUpdate++;
		a_log(TAG, "Start of firmware upgrade from \"%s\", attempt %d", url, tryUpdate);
		#if ESP_IDF_VERSION_MAJOR < 5
		err = esp_https_ota(&cfgHTTPS);
		#else
		err = esp_https_ota(&cfgOTA);
		#endif // ESP_IDF_VERSION_MAJOR
		if (err == ESP_OK) {
			a_log(TAG, "Firmware upgrade completed!");
			esp_restart();
			} else {
			ESP_LOGE(TAG, "Firmware upgrade failed: %d!", err);
			vTaskDelay(pdMS_TO_TICKS(1000));
		};
	} while ((err != ESP_OK) && (tryUpdate < CONFIG_OTA_ATTEMPTS));	
}



#define OTA_URL_SIZE 256
static esp_err_t validate_image_header(esp_app_desc_t *new_app_info)
{
	if (new_app_info == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	const esp_partition_t *running = esp_ota_get_running_partition();
	esp_app_desc_t running_app_info;
	if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
		a_log(TAG, "Running firmware version: %s", running_app_info.version);
	}
	return ESP_OK;
	#ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
	if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
		ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
		return ESP_FAIL;
	}
	#endif
	return ESP_OK;
}


static esp_err_t _decrypt_cb(decrypt_cb_arg_t *args, void *user_ctx)
{
	if (args == NULL || user_ctx == NULL) {
		ESP_LOGE(TAG, "_decrypt_cb: Invalid argument");
		return ESP_ERR_INVALID_ARG;
	}
	esp_err_t err;
	pre_enc_decrypt_arg_t pargs = {};
	pargs.data_in = args->data_in;
	pargs.data_in_len = args->data_in_len;
	err = esp_encrypted_img_decrypt_data((esp_decrypt_handle_t *)user_ctx, &pargs);
	if (err != ESP_OK && err != ESP_ERR_NOT_FINISHED) {
		return err;
	}

    static bool is_image_verified = false;
    if (pargs.data_out_len > 0) {
	    args->data_out = pargs.data_out;
	    args->data_out_len = pargs.data_out_len;
	    if (!is_image_verified) {
		    is_image_verified = true;
		    const int app_desc_offset = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t);
		    // It is unlikely to not have App Descriptor available in first iteration of decrypt callback.
		    assert(args->data_out_len >= app_desc_offset + sizeof(esp_app_desc_t));
		    esp_app_desc_t *app_info = (esp_app_desc_t *) &args->data_out[app_desc_offset];
		    err = validate_image_header(app_info);
		    if (err != ESP_OK) {
			    free(pargs.data_out);
		    }
		    return err;
	    }
	    } else {
	    args->data_out_len = 0;
    }


	return ESP_OK;
}



void pre_encrypted_ota_task(void *pvParameter)
{
	a_log(TAG, "Starting Pre Encrypted OTA example");

	char url[256];
	char user_agent[256];
	sprintf(user_agent,"ESP32 HTTP Client/1.0 Mac:%s",mac_str);
	sprintf(url,"%s/firmware/ts%u/%s",URL_MASTINO_SITE,TS_TYPE,URL_FIRMWARE);

	esp_err_t ota_finish_err = ESP_OK;
	esp_http_client_config_t config = {
		.url = url,
		.skip_cert_common_name_check = false,
		.use_global_ca_store = false,
		.crt_bundle_attach = esp_crt_bundle_attach,
		//		.cert_pem = server_cert_pem_start,
		.timeout_ms = CONFIG_EXAMPLE_OTA_RECV_TIMEOUT,
		.keep_alive_enable = true,
		.user_agent = user_agent,
	};
	esp_decrypt_cfg_t cfg = {};
	cfg.rsa_priv_key = rsa_private_pem_start;
	cfg.rsa_priv_key_len = rsa_private_pem_end-rsa_private_pem_start;
	esp_decrypt_handle_t decrypt_handle = esp_encrypted_img_decrypt_start(&cfg);
	if (!decrypt_handle) {
		ESP_LOGE(TAG, "OTA upgrade failed");
		vTaskDelete(NULL);
	}
	a_log(TAG, "esp_encrypted_img_decrypt_start");
//	config.skip_cert_common_name_check = true;

	esp_https_ota_config_t ota_config = {
		.http_config = &config,
		.partial_http_download = false,
		.max_http_request_size = 2048,//CONFIG_EXAMPLE_HTTP_REQUEST_SIZE,
		.decrypt_cb = _decrypt_cb,
		.decrypt_user_ctx = (void *)decrypt_handle,
		.enc_img_header_size = esp_encrypted_img_get_header_size(),
	};
	
	esp_https_ota_handle_t https_ota_handle = NULL;
	esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
		vTaskDelete(NULL);
	}

	while (1) {
		err = esp_https_ota_perform(https_ota_handle);
		if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
			break;
		}
		// esp_https_ota_perform returns after every read operation which gives user the ability to
		// monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
		// data read so far.
		ESP_LOGD(TAG, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
	}

	if (!esp_https_ota_is_complete_data_received(https_ota_handle)) {
		// the OTA image was not completely received and user can customise the response to this situation.
		ESP_LOGE(TAG, "Complete data was not received.");
		} else {
		err = esp_encrypted_img_decrypt_end(decrypt_handle);
		if (err != ESP_OK) {
			goto ota_end;
		}
		ota_finish_err = esp_https_ota_finish(https_ota_handle);
		if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
			a_log(TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			esp_restart();
			} else {
			if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
				ESP_LOGE(TAG, "Image validation failed, image is corrupted");
			}
			ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
			vTaskDelete(NULL);
		}
	}

	ota_end:
	esp_https_ota_abort(https_ota_handle);
	ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed");
	vTaskDelete(NULL);
}
 
#define MAX_HTTP_OUTPUT_BUFFER 256

typedef struct user_datas
{
	char *databuf;
	int  datalen;
	int  maxlen;
}user_datas;


esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // Clean the buffer in case of a new request
            if (output_len == 0 && evt->user_data) {
                // we are just starting to copy the output data into the use
                user_datas *ud=evt->user_data;
                memset(ud->databuf, 0, ud->maxlen);
            }
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) 
            {
                ESP_LOGD(TAG, "data copy");
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    ESP_LOGD(TAG, "user data copy");
                    user_datas *ud=evt->user_data;
                    // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
                    copy_len = MIN(evt->data_len, (ud->maxlen - ud->datalen));
                    if (copy_len) {
                        memcpy(ud->databuf + output_len, evt->data, copy_len);
                    }
                    ud->datalen+=copy_len;
                } else {
                    int content_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                        output_buffer = (char *) calloc(content_len + 1, sizeof(char));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (content_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}

char urls[256];
void ota_chk_http_ver(void *pvParameter)
{
	if((cfg->setting&SET_FWUPDATE)==0) vTaskDelete(NULL);
    char buf[MAX_HTTP_OUTPUT_BUFFER];
    user_datas ud={
	    .databuf = buf,
	    .datalen = 0,
	    .maxlen = MAX_HTTP_OUTPUT_BUFFER,
    };
	sprintf(urls,"%s/%s/ts%u/last",URL_MASTINO_SITE,DIR_FIRMWARE,TS_TYPE);
    esp_http_client_config_t config = {
	    .url = urls,
	    .event_handler = _http_event_handler,
	    .crt_bundle_attach = esp_crt_bundle_attach,
	    .user_data = &ud,
	    //        .event_handler =  http_evcb
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
		if(ud.datalen>4){
			char *chvmj=buf;
			char *chvmn=strchr(buf,'.');
			if(chvmn){
				*chvmn++='\0';
				char *chvadd=strchr(chvmn,'.');
				if(chvadd){
					*chvadd++='\0';
					int cvmj=atoi(chvmj);
					int cvmn=atoi(chvmn);
					int cvadd=atoi(chvadd);
					int upd=0;
					ESP_LOGI(TAG, "Firmware on server %d.%d.%d",cvmj,cvmn,cvadd);
					if(cvmj>cfg->vmj) upd=1;
					if(cvmj==cfg->vmj){
						if(cvmn>cfg->vmn) upd=1;
						if(cvmn==cfg->vmn){
							if(cvadd>cfg->vadd) upd=1;
						}
					}
					if(upd) {
						xTaskCreate(pre_encrypted_ota_task, "pre_encrypted_ota_task", 1024*8, NULL, 6, NULL);
					}
				}
			}
		}
//	    ESP_LOG_BUFFER_HEXDUMP(TAG, buf,ud.datalen,0);
//	    ESP_LOGI(TAG, "%d HTTPS Status = %d, content_length = %"PRId64,ud.datalen,esp_http_client_get_status_code(client),esp_http_client_get_content_length(client));
	    } else {
	    ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
	vTaskDelete(NULL);
}