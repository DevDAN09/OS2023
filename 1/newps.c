#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

void print_file_contents(FILE* fp){
	if(!fp){
		printf("ERROR: Filepoint is null!\n");
		return ;
	}

	fseek(fp, 0,SEEK_SET);

	char ch;
	while((ch = fgetc(fp)) != EOF){
		putchar(ch);
	}

	fseek(fp,0,SEEK_SET);
}

void get_self_tty(char* tty_name){
	ssize_t len = readlink("/proc/self/fd/0", tty_name, 255);
	if(len == -1){
		perror("readlink error in get_self_tty");
	}
    	tty_name[len] = '\0';

    char *dev_ptr = strstr(tty_name, "/dev/");
    if (dev_ptr) {
        memmove(tty_name, dev_ptr + 5, len - (dev_ptr - tty_name) + 5);
        tty_name[len - 5] = '\0';
    }
}

int main() {
   
       	DIR* dir; // /proc
	struct dirent* entry;
	FILE* fp;

	char path[512];
	char current_tty[256];
    	int pid;
    	char tty[256];
   	long utime, stime;
   
       	get_self_tty(current_tty);  // Get the current TTY
    	dir = opendir("/proc");
	if (!dir) { // Check dir
		perror("opendir");
		return 1;
	}
	printf("    PID TTY          TIME CMD\n");
       	while ((entry = readdir(dir)) != NULL) {
	       	if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
		       	snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name);
		       	//printf("%s", path);
		       	fp = fopen(path, "r");
		       	if (fp) {
				char comm[256];
			       	if (fscanf(fp, "%d (%[^)]) %*c %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %ld %ld", &pid, comm, &utime, &stime) >= 3) {                    	                 
			    		fclose(fp);                                  
					snprintf(path, sizeof(path), "/proc/%s/fd/0", entry->d_name);			
					ssize_t len = readlink(path, tty, sizeof(tty) - 1);                    
					if (len != -1) { // remove /dev/                       
						tty[len] = '\0';                     
						char *dev_ptr = strstr(tty, "/dev/");			
						if (dev_ptr) {
                            					memmove(tty, dev_ptr + 5, len - (dev_ptr - tty) + 5);    
								tty[len - 5] = '\0';                        
						}                    
					} else { // tty is X                       
						strcpy(tty, "?");                    
					}                                        
					if (strcmp(tty, current_tty) == 0) {  // Compare TTYs			
						long hertz = sysconf(_SC_CLK_TCK);                        
						long total_time = utime + stime;
						int seconds = total_time / hertz;                        
						int minutes = seconds / 60;
						int hours = minutes / 60;
						minutes %= 60;
						seconds %= 60;                        
						printf("%7d %5s    %02d:%02d:%02d %s\n", pid, tty, hours, minutes, seconds, comm);                    
					}                
				} else {                    
					fclose(fp);                
				}
			}
		}
    
	}
	closedir(dir);
	return 0;
}
