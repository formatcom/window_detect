#ifndef _APP_OTA_H_
#define _APP_OTA_H_

#define BUFFSIZE 1024
#define HASH_LEN 32 /* SHA-256 digest length */

#define OTA_URL_SIZE 256

int ota_upgrade();

void init_ota_config(void);

#endif
