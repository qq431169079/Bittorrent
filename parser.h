#pragma once

typedef struct{
	long int length;
	char* path;
}single_file;	//data about a single file in a multiple file torrent

typedef struct{
	char* name;
	char* announce;
	char* pieces;
	long int piece_size;
	long int length; //the total size of the file
	int num_files; //number of files in the torrent
	single_file** files; //dynamic array of files in multifile torrent
	char* info_hash; 	//the SHA-1 hash of the info value
}metadata;

typedef struct{
	char* peer_id;
	char* peer_ip;
	int port;
}peer_t;

typedef struct{
	char *failure_reason;
	long int interval;
	char *tracker_id;
	int complete;
	int incomplete;
	int num_peers;
	peer_t** peers;
}tracker_message;	//a summery of a message sent by the tracker

metadata *parse_meta(char *filename);
tracker_message *get_tracker_message(char *message, size_t size);

void free_metadata(metadata *md);
void free_tracker_message(tracker_message *tm);