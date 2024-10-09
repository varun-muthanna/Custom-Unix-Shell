#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include<string.h>

#define BUFSIZE 1024 //size of buffer in bytes
#define TOKBUFSIZE 64

char *get_input(){
    int pos=0,sz=BUFSIZE;
    char *line=malloc(sz*sizeof(char));

    char ch;

    if(!line){
        printf("Allocation error");
        exit(EXIT_FAILURE);
    }

    while(1){

        ch = getchar();

        if(ch==EOF || ch=='\n'){ //value of EOF typically -1, constant defined in stdio.h
            line[pos]='\0';
            return line;
        }else{
            line[pos]=ch;
        }


        ++pos;
        if(pos>=sz){
            sz+=BUFSIZE;
            line= realloc(line,sz);
            if(!line){
                printf("Allocation error");
                exit(EXIT_FAILURE);
            }
        }

    }

    return line;
}

char **tokenize(char *line){
    int sz=TOKBUFSIZE,pos=0;
    const char *del= " \n\t\a\r";

    char **args= malloc(sz*sizeof(char*)); //size of pointer to char , (system specific - 64 bit or 32 bit system)
    char *token = strtok(line,del);

    if(!args){
        printf("Allocation error");
        exit(EXIT_FAILURE);
    }

    while(token!=NULL){

        args[pos]=token;
        ++pos;

        if(pos>=sz){
            sz+=TOKBUFSIZE;
            args= realloc(args,sz*sizeof(char*));
            if(!args){
                printf("Allocation error");
                exit(EXIT_FAILURE);
            }
        }


        token=strtok(NULL,del); //continue tokenizing from last position , as strtok maintain static pointer and tracks the position in string , replaces last del with '\0'
    }

    args[pos]=NULL;

    return args;
}

int sh_cd(char **args);
int sh_help(char **args);
int sh_exit(char **args);

char *commands[] ={"cd","help","exit"}; //array of pointers to string

int (*builtin_func[]) (char **)={ //function pointer array . int - return type , dec(name) , char ** what it take in argument
    &sh_cd,
    &sh_help,
    &sh_exit
};

int builtin_func_len=sizeof(commands)/sizeof(char*);

int sh_exit(char **args){
    return 0;
}

int sh_help(char **args){

    printf("Welcome to Varun Muthanna's shell");
    printf("The following are the built in commands\n");

    for(int i=0;i<builtin_func_len;i++){
        printf(" %s\n",commands[i]);
    }

    return 1;
}

int sh_cd(char **args){

    if(args[1]==NULL){
        printf("Expected argument to cd \n");
    }else{
        if(chdir(args[1])==-1){ //system call to chang cwd , return 0 for success and -1 for failure
            perror(NULL); //prints the last error during system call
        }
    }

    return 1;
}

int external(char **args){

    int status;

    pid_t pid = fork();

    if(pid==-1){
        printf("fork failed");
        exit(EXIT_FAILURE);
    }else if(pid==0){ //child
        if(execvp(args[0],args)==-1){ //program name or absolute path , vector of strings , with first program
            perror(NULL);
        }
    }else{//parent process
        do{
            waitpid(pid,&status,WUNTRACED); //wait for current child to finish execution , status has info about child process, WUNTRACED return if child is stopped by a signal
        }while(!WIFEXITED(status) && !WIFSIGNALED(status)); //required for check for abnormal termination , return true exited normally and exited via signal
    }

    return 1;
}

int execute(char **args){

    if(args[0]==NULL){
        printf("Empty command");
        return 1;
    }


    for(int i=0;i<builtin_func_len;i++){
        if(strcmp(commands[i],args[0])==0){
            return (*builtin_func[i])(args);
        }
    }

    return external(args);

}

void loop(){

    int status;

    char *line,**args;

    do{

        printf(">");
        line = get_input();
        args = tokenize(line);
        status = execute(args);

        free(line);
        free(args);

    }while(status);

}

int main() {
    //load config files if any

    //read commands
    loop();

    //any termination commands
    return 0;
}
