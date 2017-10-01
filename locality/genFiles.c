#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

int getCountFiles(char * com){
	FILE *fp1;
	fp1 = popen(com, "r");
	if(fp1 == 0)
		perror(com);
		
	char *line = (char*)malloc(25 * sizeof(char));
		
	int n = 0;
	
	if ( fgets( line, 25, fp1))
		n = atoi(line);
		
	free(line); 
	
	pclose(fp1); 
	
	return n;
}

void copyFile(char * old_file, char * new_file){
	FILE *fp1;	
		
	char *command = (char*)malloc(200 * sizeof(char));
	
	sprintf(command, "cp db/stage1_input/%s db/stage1_input/%s", old_file, new_file);
	
	//printf("Command: %s\n", command);
	
	fp1 = popen(command, "r");
	if(fp1 == 0)
		perror(command);
		
	free(command); 
	
	pclose(fp1); 
}

void generate(){
	int total_files = getCountFiles("ls db/stage1_input/ -1 | wc -l");
	srand(time(NULL));
	int r1 = rand() % total_files; 
	int r2 = rand() % total_files; 
	
	printf("Two Numberd: %d and %d\n", r1, r2);	
	
	FILE *fp1;
	fp1 = popen("ls db/stage1_input -1", "r");  
	if(fp1 == 0)
		perror("ls db/stage1_input -1");
		
	char *line = (char*)malloc(25 * sizeof(char));
	
	char *new_name = (char*)malloc(35 * sizeof(char));
	int i = 0;	
	while ( fgets( line, 25, fp1))
	{
		line[strlen(line)-1]='\0';
		
		sprintf(new_name, "%s_0", line);		
		
		//printf("[%d]. L: %s, new: %s\n", i, line, new_name);		
		
		copyFile(line,new_name );
		
		if (i++ > 20000) break;
	}
	
	pclose(fp1); 
}

int main(argc,argv)
int argc;
char **argv;
{ 	
	
	printf("Number of files: %d\n", getCountFiles("ls db/stage1_input/ -1 | wc -l"));
	
	generate();
	
	return 1;   
}
