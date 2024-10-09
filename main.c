#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include<string.h>

#define BUFSIZE 1024 //size of buffer in bytes
#define TOKBUFSIZE 64


int cmd_len;

typedef struct Command{
    char **args;
}C;

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

        if(strcmp(token,"|")==0){
            args[pos++]=strdup("|"); //dynamically allocate , so  we can free
        }else {
            args[pos++]=token;
        }

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



C *parse_cmd(char **line){
    int sz=TOKBUFSIZE,pos=0,i=0;

    char **args= malloc(sz*sizeof(char*)); //size of pointer to char , (system specific - 64 bit or 32 bit system)

    C *commands = malloc(sz*sizeof(C)); //should create array of commands (tokens)

    if(!args){
        printf("Allocation error");
        exit(EXIT_FAILURE);
    }

    for(int j=0;line[j]!=NULL;j++){

        if(strcmp(line[j],"|")==0){
            args[pos]=NULL;
            commands[i++].args=args;

            pos=0;
            args=malloc(sz*sizeof(char*));
            if(!args){
                printf("Allocation error");
                exit(EXIT_FAILURE);
            }

            continue;
        }

        args[pos]=line[j];
        ++pos;

        if(pos>=sz){
            sz+=TOKBUFSIZE;
            args= realloc(args,sz*sizeof(char*));
            if(!args){
                printf("Allocation error");
                exit(EXIT_FAILURE);
            }
        }
    }

    args[pos]=NULL;
    commands[i++].args=args;

    cmd_len=i;

    return commands;
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

    pid_t pid = fork(); //shell itself would be replaced

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

    return -1;

}

int execute_commands(C *cmd){

    int in_fd=0;
    pid_t pid;
    int status;

    for(int i=0;i<cmd_len;i++){

        int fd[2];

        if(i<cmd_len-1){
            if(pipe(fd)==-1){
                printf("Piping error");
                exit(EXIT_FAILURE);
            }
        }

        pid = fork();

        if(pid==-1) exit(EXIT_FAILURE);

        if(pid==0) {
            if (in_fd != 0) { //not the first process then read
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            if (i < cmd_len - 1) { //set ouput for propagation
                dup2(fd[1], STDOUT_FILENO); //so if we write in fd[1] , we can read from there fd[0] later
                close(fd[1]);
            }

            if (fd[0]) { // read fd[0] , write fd[1]
                close(fd[0]);
            }

            if (external(cmd[i].args) == 0) {
                exit(0);
            }
            exit(1);
        }else{
            waitpid(pid,&status,0);

            if(in_fd!=0){ //close prev file
                close(in_fd);
            }

            if (i < cmd_len - 1) { //close write
                close(fd[1]);
            }

            //save the read
            if(i<cmd_len-1){
                in_fd=fd[0]; //pipe it down to next command
            }

        }

    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return 0;
    }

    return 1;

}

void loop(){

    int status;

    char *line,**args;
    C *cmd;

    do{

        printf(">");
        line = get_input();

        args = tokenize(line);
        cmd= parse_cmd(args);

        status=execute(args);

        if(status==-1) {
            status = execute_commands(cmd);

            for(int i=0;i<cmd_len;i++) free(cmd[i].args);
            free(cmd);
        }

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
