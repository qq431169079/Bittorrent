#include <stdio.h>
#include <stdlib.h>
#include "parser.h"


int main(int argc, char *argv[]){
	//check the arg count
	if(argc != 2){
		return -1;
	}
	//parse the .torrent file
	parse_meta(argv[1]);

}