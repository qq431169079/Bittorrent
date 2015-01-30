#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "tracker.h"


int main(int argc, char *argv[]){
	//check the arg count
	if(argc != 2){
		return -1;
	}
	//parse the .torrent file
	metadata *md = parse_meta(argv[1]);

	connect_to_tracker(md);

	//connect to server
	//connect to peers

	/*
	printf("%s\n", md->name);
	printf("%s\n", md->pieces);
	printf("%s\n", md->announce);	
	printf("%lu\n", md->length);
	printf("%d\n", md->num_files);
	//single_file ** files = md->files;
	//single_file *f = *files;
	//printf("%s\n", f->path);	
	*/

	free_metadata(md);
}