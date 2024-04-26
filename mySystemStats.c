#define _POSIX_C_SOURCE 200809L
#include "header.h"

void handle_sigint(int sig) {
    if (sig == SIGTSTP){
        return;
    }
    if (sig == SIGINT){
        char ans;
        printf("\nDo you want to quit? [y/n]: ");
        if (scanf(" %c", &ans) != 1) {
        // Handle scanf failure, including interruption by a signal.
            if (errno == EINTR) {
                printf("\nSignal detected during input, resuming...\n");
            } else {
            // Other errors are considered fatal.
                perror("Error reading input");
                exit(EXIT_FAILURE);
            }
        } else if (ans == 'y' || ans == 'Y') {
            // Exit if the user confirms.
            exit(EXIT_SUCCESS);
        }
    }
}


bool parseargument(int argc, char **argv, int* samples, int* delay,bool* seq, bool* sys, bool* user, bool* graph){
    bool smple = false;
    bool dely = false;
    for(int i = 1;i<argc;i++){
        char *token = strtok(argv[i], "=");
        if (strcmp(token, "--samples") == 0) {
            *samples = atoi(strtok(NULL, ""));
            smple = true;
        }
        else if (strcmp(token, "--tdelay") == 0) {
            *delay = atoi(strtok(NULL, ""));
            dely = true;
        }
        else if (strcmp(argv[i], "--system") == 0) { 
            *sys = true;
        }
        else if (strcmp(argv[i], "--user") == 0) { 
            *user = true;
        }
        else if (strcmp(argv[i], "--sequential") == 0) { 
            *seq = true;
        }
        else if (strcmp(argv[i], "--graphics") == 0){
            *graph = true;
        }
        else if (isInteger(argv[i]) && (i+1 < argc) && isInteger(argv[i+1]) && (!smple) && (!dely)){
            *delay = atoi(argv[i+1]);
            *samples = atoi(argv[i]);
            smple = true;
            dely = true;
            i += 1;

        }
        else if(isInteger(argv[i])&&(!smple)){
            *samples = atoi(argv[i]);
            smple = true;
        }
        else{
            return false;
        }
    }
    if (! *sys && ! *user){
        *user = true;
        *sys = true;
    }
    return true;
}
void printinfo(int samples, int delay, bool seq, bool sys, bool user, bool graph){
    struct sigaction act;
    act.sa_handler = handle_sigint;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    volatile sig_atomic_t sigint_received = 0;

    if (sigaction(SIGINT, &act, NULL) == -1) {
        perror("sigaction error for SIGINT");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTSTP, &act, NULL) == -1) {
        perror("sigaction error for SIGTSTP");
        exit(EXIT_FAILURE);
    }
    int pipe_memory[2], pipe_cpu[2], pipe_user[2];
    pid_t pid_memory, pid_cpu, pid_user;

    char memory_record[samples][MAX_STR_LEN];
    char cpu_record[samples][MAX_STR_LEN];
    memset(memory_record, 0, sizeof(memory_record));
    memset(cpu_record, 0, sizeof(cpu_record));
    CPU cpu;
    long int cpu_previous = 0, cpu_idle = 0;
    double memory_previous;  
    for(int i = 0; i<samples; i++){
        if (pipe(pipe_memory) == -1 || pipe(pipe_cpu) == -1 || pipe(pipe_user) == -1){
            perror("Pipe creation failed");
            exit(EXIT_FAILURE);
        }
        pid_memory = fork();
        if (pid_memory == -1){
            perror("Fork creation failed");
            exit(EXIT_FAILURE);
        }
        else if (pid_memory == 0){
            close(pipe_memory[0]);
            close(pipe_cpu[0]); close(pipe_cpu[1]); close(pipe_user[0]); close(pipe_user[1]);
            dup2(pipe_memory[1], STDOUT_FILENO);
            memoryStats(pipe_memory);
            exit(EXIT_SUCCESS);
        }
        else{
            pid_user = fork();
            if (pid_user == -1){
                perror("Fork creation failed");
                exit(EXIT_FAILURE);
            }
            else if (pid_user == 0){
                close(pipe_user[0]); close(pipe_cpu[0]); close(pipe_cpu[1]); close(pipe_memory[0]); close(pipe_memory[1]);
                userOutput(pipe_user);
                close(pipe_user[1]);
                exit(EXIT_SUCCESS);
            }
            else{
                pid_cpu = fork();
                if (pid_cpu == -1){
                    perror("Fork creation failed");
                    exit(EXIT_FAILURE);
                }
                else if (pid_cpu == 0){
                    close(pipe_cpu[0]); 
                    close(pipe_memory[0]); close(pipe_memory[1]); close(pipe_user[0]); close(pipe_user[1]);
                    dup2(pipe_cpu[1], STDOUT_FILENO);
                    cpuStats(pipe_cpu);
                    close(pipe_cpu[1]);
                    exit(EXIT_SUCCESS);
                }
                else{
                    close(pipe_memory[1]); close(pipe_cpu[1]); close(pipe_user[1]);
                    // wait for child process to finish
                    waitpid(pid_memory, NULL, 0);
                    waitpid(pid_user, NULL, 0);
                    waitpid(pid_cpu, NULL, 0);
                    if(!seq){
                        printf("\x1b%d", 7);
                    }
                    else{
                        printf(">>> iteration %d\n",i+1);
                    }
                    printf("Number of samples: %d -- every %d secs\n",samples,delay); 
                    if (sys){
                        MemoryInfo info;
                        ssize_t bytes = read(pipe_memory[0], &info, sizeof(info));
                        if (bytes == -1){
                            perror("Error reading from pipe");
                        }
                        memoryUsage(samples, i, info, graph, seq, &memory_previous, memory_record);
                        close(pipe_memory[0]);
                    }
                    
                    if (user){ 
                        char buffer[1024];
                        int bytesRead;

                        printf("--------------------------------------------\n");
                        printf("### Sessions/users ###\n");

                        while ((bytesRead = read(pipe_user[0], buffer, sizeof(buffer) - 1)) > 0) {
                            buffer[bytesRead] = '\0';
                            printf("%s", buffer);
                        }
                        //close it!
                        close(pipe_user[0]);
                    }
                    
                    if (sys){
                        CPU cpu_stats;
                        ssize_t bytes = read(pipe_cpu[0], &cpu_stats, sizeof(cpu_stats));
                        if (bytes== -1){ 
                            perror("Error reading from pipe"); 
                        }
                        cpu_output(graph, i, &cpu_previous, &cpu_idle, cpu_stats, cpu_record);
                        close(pipe_cpu[0]);
                    }
                    if (i+1 < samples) { 
                        sleep(delay); 
                    }
                    if(!seq && i + 1 < samples){
                        printf("\x1b%d", 8);
                    }
                }
            }
        }
    }
}
int main(int argc, char **argv){
   int samples = 10;
   int delay = 1;
   bool seq = false;
   bool system = false;
   bool user = false;
   bool graph = false;
   if(!parseargument(argc, argv, &samples, &delay, &seq, &system, &user, &graph)){
    printf("Incorrect argument\n");
    return 1;
   }
   printinfo(samples,delay,seq,system,user, graph);
   systemInfo();
   return 0;
}