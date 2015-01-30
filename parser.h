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



metadata *parse_meta(char *filename);
void free_metadata(metadata *md);