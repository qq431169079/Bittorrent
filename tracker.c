#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include "parser.h"
#include "tracker.h"

void connect_to_tracker(metadata *md){
	CURL *curl = curl_easy_init();
	CURLcode res;

	char *header = "Authorization:  Bearer myAccessCode";
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, header);

	curl_easy_setopt(curl, CURLOPT_URL, md->announce);
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);


	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
}