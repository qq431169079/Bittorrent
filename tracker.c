#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include "parser.h"
#include "tracker.h"
#include "uri.h"


const static char *PEER_ID = "-AZ2060-123456789123";
const static char *PORT = "6881";

typedef struct{
	char * buffer;
	size_t size;
} BufferStruct;


static size_t httpsCallback(void *ptr, size_t size, size_t nmemb, void *data){
	size_t realsize = size * nmemb;
	BufferStruct * mem = (BufferStruct *) data;
	mem->buffer = realloc(mem->buffer, mem->size + realsize + 1);
	
	if(mem->buffer){
		memcpy(&(mem->buffer[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->buffer[ mem->size ] = 0;
	}
	return realsize;
}


char *makePair(char *key, const char *val, int last){
	char *value = uri_encode(val);
	//char *value = val;
	int length = strlen(key) + strlen(value) + 3;
	char *buff = malloc(length+1);
	strcpy(buff, key);
	strcat(buff, "=");
	strcat(buff, value);
	if(!last)
		strcat(buff, "&");
	buff[length] = '\0';
	return buff;
}


//append the information desired to the pregenerated uri prefix
char *format_uri(char *prefix, metadata *md, char *downl, char *upl, char *ev){
	char *uploaded_string, *downloaded_string, *event_string;

	uploaded_string = makePair("uploaded", downl, 0);
	downloaded_string = makePair("downloaded", upl, 0);
	event_string = makePair("event", ev, 1);

	char buff[250];
	strcpy(buff, prefix);
	strcat(buff, uploaded_string);
	strcat(buff, downloaded_string);
	//strcat(buff, makePair("compact", uri_encode("0"), 0);
	strcat(buff, event_string);

	free(uploaded_string);
	free(downloaded_string);
	free(event_string);
	int length = strlen(buff);
	char *uri = malloc(length+1);
	strcpy(uri, buff);
	uri[length] = '\0';
	return uri;
}

//generate the prefix that is announce+info_hash+peer_id+port
char *generate_uri_prefix(metadata *md){
	char *info_hash_string, *peer_id_string, *port_string;
	info_hash_string = makePair("info_hash", md->info_hash, 0);
	peer_id_string = makePair("peer_id", PEER_ID, 0);
	port_string = makePair("port", PORT, 0);

	char buff[250];
	strcpy(buff, md->announce);
	strcat(buff, "?");
	strcat(buff, info_hash_string);
	strcat(buff, peer_id_string);
	strcat(buff, port_string);

	free(info_hash_string);
	free(peer_id_string);
	free(port_string);

	int length = strlen(buff);
	char *uri = malloc(length+1);
	strcpy(uri, buff);
	uri[length] = '\0';
	return uri;
}




void connect_to_tracker(metadata *md){

	char *prefix = generate_uri_prefix(md);
	char *uri = format_uri(prefix, md, "0", "0", "start");
	printf("%s\n", uri);

	CURL *curl = curl_easy_init();
	CURLcode res;

	BufferStruct output;
	output.buffer = NULL;
	output.size = 0;


	curl_easy_setopt(curl, CURLOPT_URL, uri);
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httpsCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&output);

	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	free(uri);
	free(prefix);
	printf("%s\n", output.buffer);
}