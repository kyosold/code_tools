//
//  ctapi_curl.h
//  
//
//  Created by SongJian on 16/3/16.
//
//

#ifndef ctapi_curl_h
#define ctapi_curl_h

#include <stdio.h>


struct pstr_t
{
	char *str;
	size_t len;
	size_t size;
};

struct curl_result_string 
{
	struct pstr_t headers;
	struct pstr_t data;
};

int ctapi_curl_post(char *url, char *method, char *post_data, char *save_cookie_fs, char *send_cookie_fs, unsigned int connect_timeout, unsigned int timeout, struct curl_result_string *curl_result_t, char *error, size_t error_size);

void ctapi_curl_free(struct curl_result_string *curl_result_t);

#endif /* ctapi_curl_h */
