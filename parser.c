#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "bencode.h"
#include <openssl/sha.h>


int meta_size;	//the size of the metafile in bytes



//extract a string from a buffer and return it as a malloced pointer
char* make_string(const char* buff, int len){
	char* target = malloc(len+1);
	if(target == NULL){
		printf("malloc failed\n");
		return NULL;
	}
	strncpy(target, buff,len);
	target[len] = '\0';	//pad a string terminator
	//printf("%s\n", target);
	return target;
}



/*
 * parsing metadata
 */



char* get_data(char *filename){
	
	//open the file
	FILE *fp = fopen(filename,"rb");
	if(fp == NULL){
		printf("cannot open the file\n");
		return NULL;
	}

	//get the file length
	//int size;
	
	fseek(fp, 0L, SEEK_END);
	meta_size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	if(meta_size == -1){
		printf("cannot get the size\n");
		return NULL;
	}

	//fill the buffer
	char *data = (char*)malloc(meta_size+1);
	long i;
	for(i = 0; i<meta_size; i++){
		data[i] = fgetc(fp);
	}
	data[meta_size] = '\0';
	fclose(fp);
	return data;
}

char *parse_path(bencode_t *list){
	bencode_t value;
	const char *buff;
	int len;
	int total = 0;
	char res[180];
	strcpy(res, "");
	while(bencode_list_has_next(list)){
		bencode_list_get_next(list, &value);
		bencode_string_value(&value, &buff, &len);
		total+=len;
		strncat(res, buff, len);
		if(bencode_list_has_next(list)){
			strcat(res, "/");
			total++;
		}
	}
	char *path = malloc(total+1);
	strcpy(path, res);
	path[total] = '\0';
	//printf("%d and %d\n", strlen(res), total);
	return path;
}


void parse_single_file(bencode_t *dict, metadata *md, int numFiles){
	single_file *sf = (single_file *) malloc(sizeof(single_file));
	bencode_t value;
	const char *buff;
	int len;
	long int val;
	md->files = (single_file **)realloc(md->files, sizeof(single_file*)*(numFiles+1));
	
	while(bencode_dict_has_next(dict)){
		bencode_dict_get_next(dict, &value, &buff, &len);
		if(!strncmp(buff, "length", len)){
			bencode_int_value(&value, &val);
			sf->length = val;
		}
		else if(!strncmp(buff, "path", len)){
			sf->path = parse_path(&value);
			//printf("%s\n", sf->path);
		}

	}
	//memcpy(&(md->files[numFiles]), sf, sizeof(single_file*));
	md->files[numFiles] = sf;
}

void parse_multiple_file_list(bencode_t *list, metadata *md){
	bencode_t value;
	int numFiles = 0;

	while(bencode_list_has_next(list)){
		//printf("b\n");
		bencode_list_get_next(list, &value);
		parse_single_file(&value, md, numFiles);
		numFiles++;
	}
	//printf("%d\n", numFiles);
	md->num_files = numFiles;
}


//compute the info hash
void get_info_hash(bencode_t *info, metadata *md){
	int len;
	const char *ren;
	bencode_dict_get_start_and_len(info, &ren, &len);
	char* digest = malloc(SHA_DIGEST_LENGTH+1);
	SHA_CTX c;
	
	SHA1_Init(&c);
	SHA1_Update(&c, ren, len);
 	SHA1_Final(digest, &c);
 	digest[SHA_DIGEST_LENGTH] = '\0';
 	
 	md->info_hash = digest;
 //	printf("%s\n", digest);
}


//parse the info dict of the bencoded file
void parse_info(bencode_t *key, metadata *md){
	get_info_hash(key, md);
	bencode_t value;
	const char *buff;
	int len;
	long int val;
	while(bencode_dict_has_next(key)){
		bencode_dict_get_next(key, &value, &buff, &len);


		if(!strncmp(buff, "name", len)){
			bencode_string_value(&value, &buff, &len);
			//printf("name: %.*s\n", len, buff);
			md->name = make_string(buff, len);
		}
		else if(!strncmp(buff, "piece length", len)){
			bencode_int_value(&value, &val);
			//printf("size: %ld\n", val);
			md->piece_size = val;
		}
		else if(!strncmp(buff, "pieces", len)){
			bencode_string_value(&value, &buff, &len);
			//printf("pieces\n");
			md->pieces = make_string(buff, len);
		}
		//single_file
		else if(!strncmp(buff, "length", len)){
			bencode_int_value(&value, &val);
			//printf("file length: %ld\n", val);
			md->length = val;
			md->num_files = 1;
		}
		//multiple files
		else if(!strncmp(buff, "files", len)){
			bencode_int_value(&value, &val);
			//printf("filessssss\n");
			parse_multiple_file_list(&value, md);
		}
	}
}

//parse a bencoded dict and fill the metadata object
int ben_parse_data(char* data, metadata* md){

	bencode_t ben1, ben2;
	int len;
	const char* buff;
	bencode_init(&ben1, data, meta_size);
	
	//check formatting
	if(bencode_is_dict(&ben1) == 0){
		printf("Torrent file invalid\n");
		return -1;
	}

	while(bencode_dict_has_next(&ben1)){
		bencode_dict_get_next(&ben1, &ben2, &buff, &len);

		//printf("name: %.*s\n", len, buff);
		
		if(strncmp(buff, "announce", len) == 0){
			//set announcer
			bencode_string_value(&ben2, &buff, &len);
			md->announce = make_string(buff, len);
			//printf("announce: %.*s\n", len, buff);
		}
		else if(!strncmp(buff, "info", len)){
			parse_info(&ben2, md);
		}

	}

	return 0;
}

//the main parsing method
metadata *parse_meta(char *filename){
	char* data = get_data(filename);
	//printf("%s\n", data);
	metadata *md = (metadata *) malloc(sizeof(metadata));
	if(md == NULL){
		printf("malloc failed\n");
		return NULL;
	}
	ben_parse_data(data, md);
	free(data);
	return md;
} 


/*
 * parsing tracker messages
 */

void parse_single_peer(bencode_t *peer, tracker_message *tm, int numPeers){
	peer_t *p = (peer_t *) malloc(sizeof(peer_t));
	bencode_t value;
	const char *buff;
	int len;
	long int val;
	tm->peers = (peer_t **)realloc(tm->peers, sizeof(peer_t*)*(numPeers+1));
	
	while(bencode_dict_has_next(peer)){
		bencode_dict_get_next(peer, &value, &buff, &len);
		if(!strncmp(buff, "peer id", len)){
			bencode_string_value(&value, &buff, &len);
			p->peer_id = make_string(buff, len);
		}
		else if(!strncmp(buff, "", len)){
			//printf("%s\n", sf->path);
		}

	}

	tm->peers[numPeers] = p;
}


//determine how the peers are encoded and parse them accordingly
void parse_peers(bencode_t *peers, tracker_message *tm){
	int len, numPeers;
	const char *buff;
	long int val;
	bencode_t value;

	//peer list case
	if(bencode_is_list(peers)){
		while(bencode_list_has_next(peers)){
			bencode_list_get_next(peers, &value);
			parse_single_peer(&value, tm, numPeers);
			numPeers++;
		}
	}
	//peer string case
	else{

	}

	tm->num_peers = numPeers;

}


void parse_tracker_message(char *message, tracker_message *tm, size_t size){
	bencode_t ben1, ben2;
	int len;
	const char* buff;
	long int val;
	bencode_init(&ben1, message, size);
	
	while(bencode_dict_has_next(&ben1)){
		bencode_dict_get_next(&ben1, &ben2, &buff, &len);
		if(!strncmp(buff, "failure reason", len)){
			bencode_string_value(&ben2, &buff, &len);
			tm->failure_reason = make_string(buff, len);
		}
		else if(!strncmp(buff, "interval", len)){
			bencode_int_value(&ben2, &val);
			tm->interval = val;
		}
		else if(!strncmp(buff, "tracker id", len)){
			bencode_string_value(&ben2, &buff, &len);
			tm->tracker_id = make_string(buff, len);
		}
		else if(!strncmp(buff, "complete", len)){
			bencode_int_value(&ben2, &val);
			tm->complete = val;
		}
		else if(!strncmp(buff, "incomplete", len)){
			bencode_int_value(&ben2, &val);
			tm->incomplete = val;
		}else if(!strncmp(buff, "peers", len)){
			parse_peers(&ben2, tm);
		}
	}
}


tracker_message *get_tracker_message(char *message, size_t size){
	tracker_message *tm = (tracker_message*) malloc(sizeof(tracker_message));
	parse_tracker_message(message, tm, size);
};



//free the metadata object
void free_metadata(metadata *md){
	int i;
	if(md == NULL) return;
	if(md->name != NULL) free(md->name);
	if(md->announce != NULL) free(md->announce);
	if(md->pieces != NULL) free(md->pieces);
	if(md->info_hash != NULL) free(md->info_hash);
	if(md->num_files>1){
		for(i = 0; i<md->num_files; i++){
			free((md->files[i])->path);
			free(md->files[i]);
		}
		free(md->files);
	}
	free(md);
}