#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "bencode.h"
#include <openssl/sha.h>


const static int BYTES_PER_PEER_INFO = 6;

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
 *
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
 *
 * parsing tracker messages
 *
 */


//parse bencode_t containing a single dict
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
		else if(!strncmp(buff, "port", len)){
			bencode_int_value(&value, &val);
			p->port = val;
		}
		else if(!strncmp(buff, "peer ip", len)){
			bencode_string_value(&value, &buff, &len);
			p->peer_id = make_string(buff, len);	//for now just save the IP as a generic string
		}
	}
}

//invoked when peers received are encoded in a single string. !!UNTESTED!!
void parse_peers_string(bencode_t *peers, tracker_message *tm){
	const char *str;
	int len, i;
	bencode_string_value(peers, &str, &len);
	
	//calculate the number of peers
	int numPeers = len/BYTES_PER_PEER_INFO;
	tm->num_peers = numPeers;

	if(!numPeers)	//if the number of peers is equal to zero, just return
		return;

	tm->peers = (peer_t **)malloc(sizeof(peer_t*)*numPeers);
	char* cur = (char *)str;	//cursor
	char buffIP[5];	//create two buffers
	char buffPort[3];
	buffIP[4] = '\0';	//pad their endings with terminators (yo)
	buffPort[2] = '\0';
	for(i = 0; i<numPeers; i++){
		peer_t *p = (peer_t*)malloc(sizeof(peer_t));
		memcpy(buffIP, cur, 4);
		p->peer_ip = strdup(buffIP);
		cur+=4;	//move the cursor
		memcpy(buffPort, cur, 2);
		p->port = atoi(buffPort);
		tm->peers[i] = p;
	}

	char *pr;

}


//determine how the peers are encoded and parse them accordingly
void parse_peers(bencode_t *peers, tracker_message *tm){
	int len, numPeers;
	const char *buff;
	long int val;
	bencode_t value;

	//peer list case
	if(bencode_is_list(peers)){
		//parse each item in the list
		while(bencode_list_has_next(peers)){
			bencode_list_get_next(peers, &value);
			parse_single_peer(&value, tm, numPeers);
			numPeers++;
		}
		tm->num_peers = numPeers;
	}
	//peer string case
	else{
		parse_peers_string(peers, tm);
	}

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
			//printf("a\n");
			bencode_string_value(&ben2, &buff, &len);
			tm->failure_reason = make_string(buff, len);
		}
		else if(!strncmp(buff, "interval", len)){
			//printf("b\n");
			bencode_int_value(&ben2, &val);
			tm->interval = val;
			//printf("%li\n", val);
		}
		else if(!strncmp(buff, "tracker id", len)){
			//printf("c\n");
			bencode_string_value(&ben2, &buff, &len);
			tm->tracker_id = make_string(buff, len);
		}
		else if(!strncmp(buff, "complete", len)){
			//printf("d\n");
			bencode_int_value(&ben2, &val);
			tm->complete = val;
		}
		else if(!strncmp(buff, "incomplete", len)){
			//printf("e\n");
			bencode_int_value(&ben2, &val);
			tm->incomplete = val;
		}else if(!strncmp(buff, "peers", len)){
			//printf("f\n");
			parse_peers(&ben2, tm);
		}
	}
}


tracker_message *get_tracker_message(char *message, size_t size){
	tracker_message *tm = (tracker_message*) malloc(sizeof(tracker_message));
	parse_tracker_message(message, tm, size);
	return tm;
};


/*
 * cleanup
 */


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

//free the tracker message object
void free_tracker_message(tracker_message *tm){
	if(tm == NULL) return;
	if(tm->failure_reason != NULL) free(tm->failure_reason);
	if(tm->tracker_id != NULL) free(tm->tracker_id);
	if(tm->num_peers>0){
		int i;
		for(i = 0; i<tm->num_peers; i++){
			peer_t *p = tm->peers[i];
			free(p->peer_id);
			free(p->peer_ip);
			free(p);
		}
		free(tm->peers);
	}
	free(tm);
}