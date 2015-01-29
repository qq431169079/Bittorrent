#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "bencode.h"


char* get_data(char *filename){
	
	//open the file
	FILE *fp = fopen(filename,"rb");
	if(fp == NULL){
		printf("cannot open the file\n");
		return NULL;
	}

	//get the file length
	int size;
	
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	if(size == -1){
		printf("cannot get the size\n");
		return NULL;
	}

	//fill the buffer
	char *data = (char*)malloc(size+1);
	long i;
	for(i = 0; i<size; i++){
		data[i] = fgetc(fp);
	}
	data[size] = '\n';
	fclose(fp);
	return data;
}

void parse_info(bencode_t *key){
	bencode_t value;
	const char *buff;
	int len;
	long int val;
	while(bencode_dict_has_next(key)){
		bencode_dict_get_next(key, &value, &buff, &len);

		if(!strncmp(buff, "name", len)){
			bencode_string_value(&value, &buff, &len);
			printf("name: %.*s\n", len, buff);
		}
		if(!strncmp(buff, "piece size", len)){
			bencode_int_value(&value, &val);
			printf("size: %ld\n", val);
		}
		if(!strncmp(buff, "pieces", len)){
			bencode_string_value(&value, &buff, &len);
			printf("pieces: %.*s\n", len, buff);
		}
		if(!strncmp(buff, "length", len)){
			bencode_int_value(&value, &val);
			printf("file length: %ld\n", val);
		}
	}
}

int ben_parse_data(char* data){

	bencode_t ben1, ben2;
	int len;
	const char* buff;
	bencode_init(&ben1, data, strlen(data));
	
	//check formatting
	if(bencode_is_dict(&ben1) == 0){
		printf("Torrent file invalid");
		return -1;
	}

	while(bencode_dict_has_next(&ben1)){
		bencode_dict_get_next(&ben1, &ben2, &buff, &len);
		
		if(strncmp(buff, "announce", len) == 0){
			//set announcer
			bencode_string_value(&ben2, &buff, &len);
			printf("announce: %.*s\n", len, buff);
		}
		else if(!strncmp(buff, "info", len)){
			parse_info(&ben2);
		}

	}

	return 0;
}

int parse_meta(char *filename){
	char* data = get_data(filename);
	ben_parse_data(data);
	free(data);
	return 0;
} 