#include "header.h"

int count_cores() {
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) return -1;

    char line[256];
    int cores = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "processor", 9) == 0) {
            cores++;
        }
    }

    fclose(fp);
    return cores;
}

void memoryUsage(int samples, int i, MemoryInfo info, bool graph, bool seq, double *previous, char record[][MAX_STR_LEN]){
    struct rusage usage;
    struct sysinfo sys_info;
    getrusage(RUSAGE_SELF, &usage); 
    sysinfo(&sys_info);
    if (sysinfo(&sys_info) == -1) {
        fprintf(stderr, "Failed to get system information. (%s)\n", strerror(errno));
        fprintf(stderr, "Terminating the process and its parent.\n");
        kill(getpid(), SIGTERM); 
        kill(getppid(), SIGTERM);
        return;
    }
    printf("MemoryUsage: %ld kilobytes\n", usage.ru_maxrss);
    printf("--------------------------------------------\n");
    printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
    float ram = sys_info.totalram / 1000000000.0;
    float phys = ram - sys_info.freeram / 1000000000.0;
    float mem = sys_info.totalswap / 1000000000.0 + ram;
    float virtual = phys + sys_info.totalswap / 1000000000.0 - sys_info.freeswap / 1000000000.0;
    if (! graph) sprintf(record[i],"%.2f GB / %.2f GB -- %.2f GB/ %.2f GB\n", info.used_memory, info.total_memory, info.used_virtual, info.total_virtual);
    else sprintf(record[i],"%.2f GB / %.2f GB -- %.2f GB/ %.2f GB\t|", info.used_memory, info.total_memory, info.used_virtual, info.total_virtual);
    if (graph){
        appendMemoryGraphicToTail(info.used_memory, previous, i, record);
    }

    for (int j = 0; j <= i; j++){
        if (!seq || j == i){
            printf("%s", record[j]);
        }
        else{
            printf("\n");
        }
    }
    for (int j = 0; j < samples - i - 1; j++){
        printf("\n");
    }
}
void memoryStats(int pipe[2]) {
    struct sysinfo sys_memory_info;
    int result = sysinfo(&sys_memory_info);

    if (result != 0) {
        fprintf(stderr, "Error: %d - %s\n", errno, strerror(errno));
        kill(getpid(), SIGTERM);
        kill(getppid(), SIGTERM);
        return;
    }

    MemoryInfo memInfo;
    const double convert = 1000000000.0;
    memInfo.total_memory = (double) sys_memory_info.totalram / convert;
    memInfo.used_memory = (double) (sys_memory_info.totalram - sys_memory_info.freeram) / convert;
    memInfo.total_virtual = (double) (sys_memory_info.totalram + sys_memory_info.totalswap) / convert;
    memInfo.used_virtual = (double) (sys_memory_info.totalram - sys_memory_info.freeram + sys_memory_info.totalswap - sys_memory_info.freeswap) / convert;

    ssize_t bytes_written = write(pipe[1], &memInfo, sizeof(memInfo));

    if (bytes_written == -1) {
        perror("Error writing to pipe");
        kill(getpid(), SIGTERM);
        kill(getppid(), SIGTERM);
    }
}
void appendMemoryGraphicToTail(double memory_current, double *memory_previous, int i, char record[][MAX_STR_LEN]) {
    if (i == 0) {
        *memory_previous = memory_current;
    }

    // Calculate the absolute difference in memory usage
    double diff = memory_current - *memory_previous;
    double abs_diff = fabs(diff);

    // Initialize the visual representation string
    int visual_len = (int)( abs_diff / 0.01 );
    char last_char;
    char sign;

    // Choose the appropriate sign and last character based on the difference in memory usage
    if (diff >= 0) { 
        sign = '#';
        last_char = '*';
        if (visual_len == 0){ 
            last_char = 'o'; 
        }
    }
    else { 
        sign = ':'; 
        last_char = '@'; 
    }

    *memory_previous = memory_current;
    // Fill in the visual representation
    int visual_start = strlen(record[i]);
    for (int j = 0; j < visual_len && visual_start + j + 6 < 1023; ++j) {
        record[i][visual_start + j] = sign;
    }

    if (visual_start + visual_len + 6 < 1023) {
        record[i][visual_start + visual_len] = last_char;
        // Account for the newline and null terminator in the remaining space calculation
        snprintf(record[i] + visual_start + visual_len + 1, 1024 - visual_start - visual_len - 1, " %.2f (%.2f)\n", abs_diff, memory_current);
    }
}

void userOutput(int pipe[2]) {
    // Set the path to the utmp file to read login records
    if (utmpname(_PATH_UTMP) != 0) {
        perror("Error setting utmp file path");
        kill(getpid(), SIGTERM);
        kill(getppid(), SIGTERM);
        return; // Early return as the parent process should decide what to do next
    }

    // Rewind to the start of the utmp file to begin reading
    setutent();

    struct utmp *userSession;
    while ((userSession = getutent()) != NULL) { // Read each record
        // Filter for user processes
        if (userSession->ut_type == USER_PROCESS) {
            // Prepare a string with user, session ID, and host
            char buffer[MAX_STR_LEN];
            snprintf(buffer, sizeof(buffer), "%s\t%s (%s)\n", userSession->ut_user, userSession->ut_line, userSession->ut_host);

            // Write the formatted string to the pipe
            ssize_t bytesWritten = write(pipe[1], buffer, strlen(buffer));
            if (bytesWritten == -1) {
                perror("Error writing to pipe");
                kill(getpid(), SIGTERM); 
                kill(getppid(), SIGTERM);
                return; 
            }
        }
    }
    // Cleanup: Close the utmp file and the write-end of the pipe
    endutent();
    close(pipe[1]);
}
void cpuStats(int pipe[2]) {
    CPU cpu_stats;
    // Attempt to retrieve system info
    if (sysinfo(&(struct sysinfo){}) != 0) {
        fprintf(stderr, "Error: (%s)\n", strerror(errno));
        kill(getpid(), SIGTERM);
        kill(getppid(), SIGTERM);
        return; // Ensures that we don't execute further code
    }

    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) {
        fprintf(stderr, "Error: (%s)\n", strerror(errno));
        kill(getpid(), SIGTERM);
        kill(getppid(), SIGTERM);
        return;
    }

    long int user, nice, system, idle, iowait, irq, softirq;
    int read_items = fscanf(fp, "cpu %ld %ld %ld %ld %ld %ld %ld", 
                            &user, &nice, &system, &idle, 
                            &iowait, &irq, &softirq);
    cpu_stats.tot = user + nice + system + iowait + irq + softirq;
    cpu_stats.time = idle;
    fclose(fp); 
    if (read_items != 7) {
        fprintf(stderr, "Error\n");
        kill(getpid(), SIGTERM);
        kill(getppid(), SIGTERM);
        return;
    }
    ssize_t bytes_written = write(pipe[1], &cpu_stats, sizeof(cpu_stats));
    if (bytes_written == -1) {
        perror("Error writing to pipe");
        kill(getpid(), SIGTERM);
        kill(getppid(),SIGTERM);
    }  
}

void getOSInfo() {
    struct utsname unameData;
    uname(&unameData);
    printf("System Name = %s\n", unameData.sysname);
    printf("Machine Name = %s\n", unameData.nodename);
    printf("Version = %s\n", unameData.version);
    printf("Release = %s\n", unameData.release);
    printf("Architecture = %s\n", unameData.machine);
}

bool isInteger(const char *str) {
    // Check if the string is empty after a sign or if it was empty to start with
    if (!*str) {
        return false;
    }

    // Check each character to ensure it's a digit
    while (*str) {
        if (!isdigit((unsigned char)*str)) {
            return false; // Found a non-digit character
        }
        str++; // Move to the next character
    }
    return true; // Only digit characters were found
}
void getUptime() {
    FILE* uptimefile;
    int days, hours, minutes, seconds, total_hours;
    float uptime_seconds;

    uptimefile = fopen("/proc/uptime", "r");
    if (uptimefile == NULL) {
        perror("Error opening uptime file");
        exit(1);
    }
   
    fscanf(uptimefile, "%f", &uptime_seconds);
    fclose(uptimefile);
    days = (int)uptime_seconds / (24 * 3600);
    uptime_seconds -= days * (24 * 3600);
    hours = (int)uptime_seconds / 3600;
    uptime_seconds -= hours * 3600;
    minutes = (int)uptime_seconds / 60;
    uptime_seconds -= minutes * 60;
    seconds = (int)uptime_seconds;
    total_hours = days * 24 + hours;
    printf("System running since last reboot: %d days %02d:%02d:%02d (%d:%02d:%02d)\n", days, hours, minutes, seconds, total_hours, minutes, seconds);
}

void systemInfo(){
    printf("--------------------------------------\n");
    getOSInfo();
    getUptime();
}

void appendAndPrintCpuGraphics(double usage, int i, char record[][MAX_STR_LEN]) {
   // Construct the graphical representation
    int usageBars = (int)usage;
    int visualLength = usageBars + 3; // Starting '|||' plus usage bars
    if (visualLength > MAX_STR_LEN - 10) {
        visualLength = MAX_STR_LEN - 10; // Reserve space for usage text
    }

    // Initialize the string with spaces
    memset(record[i], ' ', MAX_STR_LEN - 1);
    record[i][MAX_STR_LEN - 1] = '\0'; // Ensure string is null-terminated

    // Build the graphical representation
    strncpy(record[i], "|||", 3);
    for (int j = 3; j < visualLength; ++j) {
        record[i][j] = '|';
    }
    snprintf(record[i] + visualLength, MAX_STR_LEN - visualLength, " %.2f%%", usage);

    for (int j = 0; j <= i; j++){
        printf("%s\n", record[j]);
    }
}
void cpu_output(bool graphics, int i, long int* cpu_previous, long int* time_previous, CPU info, char record[][MAX_STR_LEN]){
    long int total_prev = *cpu_previous + *time_previous;
    long int total_cur = info.time + info.tot;
    double totald = (double) total_cur - (double) total_prev;
    double idled = (double) info.time - (double) *time_previous;
    double cpu_use = fabs((1000 * (totald - idled) / (totald + 1e-6) + 1) / 10);
    if (cpu_use > 100){ cpu_use = 100; }
    *cpu_previous = info.tot;
    *time_previous = info.time;
    printf("--------------------------------------------\n");
    printf("Number of Cores: %d\n", count_cores());
    printf("CPU Usage: %.2f%%\n", cpu_use);
    if(graphics){
        appendAndPrintCpuGraphics(cpu_use, i, record);
    }
}