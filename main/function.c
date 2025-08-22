/*
 * function.c
 *
 *  Created on: 4 трав. 2025 р.
 *      Author: voltekel
 */


#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"
#include "global.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "mlist.h"
#include "gpio_all.h"
#include "pins.h"
#include "wifi.h"

#define BYTES_PER_LINE 16


static const char *TAG = "LOG";
static pthread_mutex_t logmlmutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t ntpmutex=PTHREAD_MUTEX_INITIALIZER;
uint8_t lastpowerstate=255;

MList *MLlogs=NULL;
MList *MLulogs=NULL;
uint32_t calc_crccfg(configs *rcfg);


uint64_t gettimems(void);
//-----------------------------------------------------------------------
void affree(void *al,size_t size)
{
	void **tmp = (void **)al;
	if(*tmp)
	{
		memset(*tmp,0,size);
		free(*tmp);
		*tmp=NULL;
	}
}
//-----------------------------------------------------------------------
int aalloc(void *al,size_t size)
{
	//	void *al;
	void **tmp = (void **)al;
	if(*tmp)
	{
		fprintf(stderr,"Allocate pointer not null\r\n");
		return 0;
	}		
	*tmp = calloc(size,1);
	if(*tmp == NULL)
	{
		fprintf(stderr,"Cannot allocate memory\r\n");
		return (0);
	}
//	memset(*tmp, 0, size);
	return 1;
}
//-----------------------------------------------------------------------
void chk_log(void *arg)
{
	while(1)
    {
		pthread_mutex_lock(&logmlmutex);
		while(MLlogs)
		{
			pthread_mutex_unlock(&logmlmutex);
			alogs *ls=MLlogs->data;
			if(ls->str)
			{
//				size_t s=heap_caps_get_free_size(MALLOC_CAP_8BIT);
				ESP_LOG_LEVEL(ls->mtype, ls->tag, "%llu %s %u",ls->time,ls->str,MLlogs->list->count);
				free(ls->str);
			}
			pthread_mutex_lock(&logmlmutex);
// 			size_t s=heap_caps_get_free_size(MALLOC_CAP_8BIT);
// 			a_log(TAG, "Free 1 %u",s);
			MLlogs=m_list_remove(MLlogs,ls);
// 			s=heap_caps_get_free_size(MALLOC_CAP_8BIT);
// 			a_log(TAG, "Free 2 %u",s);
			free(ls);
// 			s=heap_caps_get_free_size(MALLOC_CAP_8BIT);
// 			a_log(TAG, "Free 3 %u",s);
		}
		pthread_mutex_unlock(&logmlmutex);
		vTaskDelay(1);
	}
}
//-----------------------------------------------------------------------
void u_log(alogs *ls)
{
	if(freemem>50000)
	{
		alogs *uls=NULL;
		aalloc(&uls,sizeof(alogs));
		memcpy(uls,ls,sizeof(alogs));
		uls->str=strdup(ls->str);
		pthread_mutex_lock(&logmlmutex);
		MLulogs=m_list_append(MLulogs,uls);
//		ESP_LOGE(TAG, "udps add %u",MLulogs->list->count);
		while(MLulogs->list->count>20)
		{
			alogs *uls=MLulogs->data;
			MLulogs=m_list_remove(MLulogs,uls);
			free(uls->str);
			free(uls);
		}
		pthread_mutex_unlock(&logmlmutex);
	}else
	{
		pthread_mutex_lock(&logmlmutex);
		while(MLulogs)
		{
			alogs *uls=MLulogs->data;
			MLulogs=m_list_remove(MLulogs,uls);
//			ESP_LOGE(TAG, "udps del %u",MLulogs->list->count);
			free(uls->str);
			free(uls);
		}
		pthread_mutex_unlock(&logmlmutex);
	}
}
//-----------------------------------------------------------------------
void a_log(const char *type, char *fmt,...) 
{
    static char strk[1024]={0};
	alogs *ls=NULL;
	aalloc(&ls,sizeof(alogs));
	strcpy(ls->tag,type);
	ls->flag = 1;
	ls->mtype=ESP_LOG_INFO;
	va_list params;
	va_start(params, fmt);
	vsnprintf (strk, sizeof(strk), fmt, params);
	va_end(params);
	ls->mac=mac64;
	ls->time=gettimems();
	ls->str=strdup(strk);

//	u_log(ls);
	if(logrealtime)
	{
		ESP_LOG_LEVEL(ls->mtype,type,"%s",ls->str);
		free(ls->str);
		free(ls);
	}else
	{
		pthread_mutex_lock(&logmlmutex);
		MLlogs=m_list_append(MLlogs,ls);
		pthread_mutex_unlock(&logmlmutex);
	}
}
//-----------------------------------------------------------------------
void a_logl(esp_log_level_t level, const char *type, char *fmt,...) 
{
    static char strk[1024]={0};
	alogs *ls=NULL;
	aalloc(&ls,sizeof(alogs));
	strcpy(ls->tag,type);
	ls->flag = 1;
	ls->mtype=level;
	va_list params;
	va_start(params, fmt);
	vsnprintf (strk, sizeof(strk), fmt, params);
	va_end(params);
	ls->time=gettimems();
	ls->str=strdup(strk);
	//ls->str=strdup("   ");
//	gettimeofday(&ls->tval,(struct timezone *)0);
//	u_log(ls);
	if(logrealtime)
	{
		ESP_LOG_LEVEL(level,type,"%s",ls->str);
		free(ls->str);
		free(ls);
	}else
	{
		pthread_mutex_lock(&logmlmutex);
		MLlogs=m_list_append(MLlogs,ls);
		pthread_mutex_unlock(&logmlmutex);
	}
}

//-----------------------------------------------------------------------
void a_dump(const char *tag, const void *buffer, uint16_t buff_len, esp_log_level_t log_level)
{

    if (buff_len == 0) {
        return;
    }
    char temp_buffer[BYTES_PER_LINE + 3]; //for not-byte-accessible memory
    const char *ptr_line;
    //format: field[length]
    // ADDR[10]+"   "+DATA_HEX[8*3]+" "+DATA_HEX[8*3]+"  |"+DATA_CHAR[8]+"|"
    char hd_buffer[2 + sizeof(void *) * 2 + 3 + BYTES_PER_LINE * 3 + 1 + 3 + BYTES_PER_LINE + 1 + 1];
    char *ptr_hd;
    int bytes_cur_line;

    do {
        if (buff_len > BYTES_PER_LINE) {
            bytes_cur_line = BYTES_PER_LINE;
        } else {
            bytes_cur_line = buff_len;
        }
        if (!esp_ptr_byte_accessible(buffer)) {
            //use memcpy to get around alignment issue
            memcpy(temp_buffer, buffer, (bytes_cur_line + 3) / 4 * 4);
            ptr_line = temp_buffer;
        } else {
            ptr_line = buffer;
        }
        ptr_hd = hd_buffer;

        ptr_hd += sprintf(ptr_hd, "%p ", buffer);
        for (int i = 0; i < BYTES_PER_LINE; i ++) {
            if ((i & 7) == 0) {
                ptr_hd += sprintf(ptr_hd, " ");
            }
            if (i < bytes_cur_line) {
                ptr_hd += sprintf(ptr_hd, " %02x", (unsigned char) ptr_line[i]);
            } else {
                ptr_hd += sprintf(ptr_hd, "   ");
            }
        }
        ptr_hd += sprintf(ptr_hd, "  |");
        for (int i = 0; i < bytes_cur_line; i ++) {
            if (isprint((int)ptr_line[i])) {
                ptr_hd += sprintf(ptr_hd, "%c", ptr_line[i]);
            } else {
                ptr_hd += sprintf(ptr_hd, ".");
            }
        }
        ptr_hd += sprintf(ptr_hd, "|");
        a_logl(ESP_LOG_ERROR,tag, "%s", hd_buffer);
        buffer += bytes_cur_line;
        buff_len -= bytes_cur_line;
    } while (buff_len);
}

//-----------------------------------------------------------------------
char *trimrn(char *txt)
{
	register int l;
	register char *p1, *p2;

	if (*txt==' ')
	{
		for (p1=p2=txt;(*p1==' ') || (*p1=='\t') || (*p1=='\n') || (*p1=='\r'); p1++);
		while (*p1) *p2++=*p1++;
		*p2='\0';
	}
	if ((l=strlen(txt))>0){
		for (p1=txt+l-1;(*p1==' ') || (*p1=='\t') || (*p1=='\n') || (*p1=='\r');*p1--='\0');
	}
	
	return(txt);
}
//-----------------------------------------------------------------------
char *strtolower(char *txt)
{
	char *p;
	for (p=txt; *p; p++){
		*p=tolower(*p);
	}
	
	return(txt);
}
//-----------------------------------------------------------------------
uint64_t gettime(void)
{
	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);
	return (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
}
//-----------------------------------------------------------------------
uint64_t gettimems(void)
{
	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);
	return (int64_t)tv_now.tv_sec * 1000L + ((int64_t)tv_now.tv_usec/1000);
}
//-----------------------------------------------------------------------
uint64_t gettimesec(void)
{
	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);
	return (uint64_t)tv_now.tv_sec;
}
//-----------------------------------------------------------------------
void create_thread(char *name,void *funtr,void *data)
{
	pthread_t	nthread;
	int ret;
	ret=pthread_create(&nthread, NULL, funtr, data);
	if (ret)
	{
		ESP_LOGE(TAG,"Thread %s not started - errno %d\r\n",name,errno);
		while(1){};
	}
	pthread_detach(nthread);
}
//-----------------------------------------------------------------------
void time_sync_notification_cb(struct timeval *tv)
{
    a_log(TAG, "Notification of a time synchronization event");
}
//-----------------------------------------------------------------------
void obtain_ntp_time(void)
{
//    ESP_ERROR_CHECK(example_connect());
	if(pthread_mutex_trylock(&ntpmutex)==0)
	{
		a_log(TAG, "Initializing and starting SNTP");
		esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(cfg->ntp_server);
		config.sync_cb = time_sync_notification_cb;     // Note: This is only needed if we want

		esp_netif_sntp_init(&config);
		time_t now = 0;
		struct tm timeinfo = { 0 };
		int retry = 0;
		const int retry_count = 15;
		while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
			a_log(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
		}
		time(&now);
		localtime_r(&now, &timeinfo);
		esp_netif_sntp_deinit();
		pthread_mutex_unlock(&ntpmutex);
	}
	pthread_exit(0);
}

//-----------------------------------------------------------------------
void check_power(void)
{
	for (int i=0;i<NUM_CH_ADC;i++)
	{
		if(adc_val_avgcnt[i]) adc_val_sr[i]=adc_val_avg[i]/adc_val_avgcnt[i];
		adc_val_avgcnt[i]=0;
		adc_val_avg[i]=0;
	}

	vin= raw_to_volt(adc_val_sr[0])*0.01201974184856815161390158204228;
#if defined(TS_V7)
	vbat=raw_to_volt(adc_val_sr[1])*0.01201974184856815161390158204228;
#endif
	if(vin>11) powerstate|=ST_POWER_EN_VIN; else powerstate&=~ST_POWER_EN_VIN;

#if defined(TS_V7)
	if((powerstate&ST_POWER_BAT_PRESENT)==0)
	{
		if(vbat>6)
		{
			powerstate|=ST_POWER_BAT_PRESENT;
#ifdef CHRG_CE
			CHARGE_EN;
#endif			
		}
	}
	if((powerstate&ST_POWER_BAT_PRESENT)&&((powerstate&ST_POWER_BAT_ERROR)==0))
	{
		if((vbat>(cfg->voff+0.2))&&((powerstate&ST_POWER_EN_BAT)==0)) powerstate|=ST_POWER_EN_BAT;
	}
#endif
	if(powerstate!=lastpowerstate)
	{
		cfg->last_state|=ST_SAVE_CONFIG;
		a_log(TAG, "POWER CHANGE 0x%02x",powerstate);
	/*	
	    if((powerstate&ST_POWER_EN_VIN)&&((lastpowerstate&ST_POWER_EN_VIN)==0))
		{
			wconn(cfg->wifi_mode);
		}
		if((lastpowerstate&ST_POWER_EN_VIN)&&((powerstate&ST_POWER_EN_VIN)==0))
		{
			esp_wifi_stop();
		}
	*/
		lastpowerstate=powerstate;
	}
}
//-----------------------------------------------------------------------
void valve_close(uint8_t num)
{
 	//VALVE_CLOSE;
	{ON(VL_PWR);time_power_valve=TIME_ON_VALVE;OFF(VL_NCNO);closev=1;set_kontaktor(0);}
	a_log(TAG, "VALVE_CLOSE %u",num);
}
//-----------------------------------------------------------------------
void valve_open(uint8_t num)
{
 //VALVE_OPEN;
	{ON(VL_PWR);time_power_valve=TIME_ON_VALVE;ON(VL_NCNO);closev=0;set_kontaktor(1);}
	a_log(TAG, "VALVE_OPEN %u",num);
}
//-------------------------------------------------------------------------------------------------------------------
uint32_t hostname_to_ip(char *hostname)
{
	struct addrinfo *servinfo, *p;
	struct sockaddr_in *h;
	int rv;

	//	memset(&hints, 0, sizeof hints);
	const struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};

	if ( (rv = getaddrinfo( hostname , "80" , &hints , &servinfo)) != 0)
	{
		ESP_LOGE(TAG, "getaddrinfo: %s %s\n",hostname, strerror(rv));
		return 0;
	}
	uint32_t ip=0;
	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		
		h = (struct sockaddr_in *) p->ai_addr;
		ip=(uint32_t) h->sin_addr.s_addr;
		break;
		//		printf("%u %08x\n",ip,ip);
		//		strcpy(ip , inet_ntoa( h->sin_addr ) );
	}
	
	freeaddrinfo(servinfo); // all done with this structure
	return ip;
}

//========================== GLOBAL BASIC FUNCTIONS ====================================
unsigned char set_Bit(char num_val, char num_bit) {return num_val = num_val | (1 << num_bit);}
//------------------------------------------------------------------------------
unsigned char clr_Bit(char num_val, char num_bit) {return  num_val = num_val & ~(1 << num_bit);}
//------------------------------------------------------------------------------
unsigned char tgl_Bit(char num_val, char num_bit) {return num_val = num_val ^ (1 << num_bit);}
//------------------------------------------------------------------------------
bool get_Bit(char num_val, char num_bit) {return ((num_val >> num_bit) & 1)>0; }

//--------------------------------------------------------------------------------------------------


