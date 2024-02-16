#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#define SYSCALL_STRING_REVERT  450
#define SYSCALL_PLUS  451
#define SYSCALL_MINUS 452

int isChar(char* str){
    int i,len;
    len = strlen(str);
    //printf("********Checking Character*******\n");
    for (i = 0; i < len; i++) {
        if (str[i] < '0' || str[i] > '9') {
            if(str[i] == ' ' || str[i] == '+' || str[i] == '-'){
                continue;
            }
            else{
                //printf("Detected [%d] : %c\n",i,str[i]);
                //printf("return 0\n");
                return 1;
            }
        }
        //printf("Passed [%d] : %c\n",i,str[i]);
    }
    //printf("return 1\n");
    return 0;
}

int isSpace(char *str){
    int i, len;
    len = strlen(str);
    for(i = 0; i < len; i++){
        if(str[i] == ' '){
            return 1;
        }
    }
    return 0;
}

int main() {
    char input[100];
    char output[100];
    
    while (1) {
        printf("Input: ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        input[strcspn(input, "\n")] = 0; // \n => NULL

        /* preprocessing start */
        //printf("checking input : %s\n",input);
        /* preprocessing done */

        if (strlen(input) == 0) { //no input stop
            break;
        }

        long ret;
        if (strchr(input, '+')) { //+ or - in input
            if(!isChar(input))
                ret = syscall(SYSCALL_PLUS, input, output);
            else
                ret = -1;
        } else if(strchr(input, '-')){
            if(!isChar(input))
                ret = syscall(SYSCALL_MINUS,input,output);
            else
                ret = -1;
        }
        else {
            if(!isChar(input) && !isSpace(input)){
                //printf("SYS_STRING_REVERT CALL!\n");
                ret = syscall(SYSCALL_STRING_REVERT, input, output);
            }
            else
                ret = -1;
        }

        if (ret == 0) {
            printf("Output: %s\n", output);
        } else {
            printf("Wrong Input!!\n");
        }
    }

    return 0;
}
