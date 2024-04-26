#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <math.h>
#include <utmp.h>
#include <errno.h>

#define _POSIX_C_SOURCE 200809L
#define MAX_STR_LEN 1024

/**
 * @brief A node in a linked list for storing memory usage information.
 * 
 * This structure represents a node in a linked list, where each node holds a name
 * and a pointer to the next node in the list. The name field is intended to store
 * descriptive information about the memory usage, possibly including both textual
 * and graphical data.
 *
 * @param name A character array to store descriptive information about memory usage.
 * @param next A pointer to the next node in the linked list.
 */
typedef struct LinkedList {
    char name[1024];
    struct LinkedList *next; 
}Memory;

/**
 * @brief A structure to hold detailed memory statistics.
 * 
 * This structure encapsulates information about the system's memory usage, including
 * total and used physical memory, as well as total and used virtual memory. It is used
 * to aggregate memory statistics for easy access and manipulation.
 *
 * @param total_memory Total physical memory available on the system (in GB).
 * @param used_memory Amount of physical memory currently in use (in GB).
 * @param total_virtual Total virtual memory available on the system (in GB).
 * @param used_virtual Amount of virtual memory currently in use (in GB).
 */
typedef struct {
    double total_memory;
    double used_memory;
    double total_virtual;
    double used_virtual;
} MemoryInfo;

/**
 * @brief A structure to hold CPU usage statistics.
 * 
 * Represents the CPU usage data by storing the total idle time and the total time
 * spent performing all system activities. This structure can be used to calculate
 * CPU utilization percentages and to track CPU activity over time.
 *
 * @param time Total idle time of the CPU.
 * @param tot Total time spent on all CPU activities including system, user, and idle processes.
 */
typedef struct CPUUsage{
    long int time;
    long int tot;
}CPU;

/**
 * @brief Monitors and prints memory usage information.
 * 
 * This function retrieves current process memory usage and system memory information,
 * constructs a memory usage string, and stores it directly in a specified index of a 2D character array.
 * Each 'row' of this array represents a snapshot of memory usage information.
 * If graph mode is enabled, it also appends a graphical representation of memory usage to the same 'row'.
 * The function prints memory usage information for each snapshot up to the current index,
 * and if in sequential mode, it specifically prints information corresponding to the current iteration.
 * Additional empty lines may be printed for alignment in graphical mode.
 *
 * @param samples The total number of samples to process. This should match the first dimension of the 'record' array.
 * @param i The current sample index. This determines which 'row' of the 'record' array to update with the new memory usage information.
 * @param info A struct containing memory information metrics. This includes details such as used and total physical and virtual memory.
 * @param graph Boolean flag indicating if graphical mode is enabled. In graphical mode, a visual representation of memory usage changes is appended.
 * @param seq Boolean flag indicating if sequential mode is enabled. In sequential mode, only the current iteration's memory usage information is printed.
 * @param previous Pointer to a double tracking the previous memory usage. This is used for generating the graphical representation in graph mode.
 * @param record A 2D character array where each 'row' is designated to store a snapshot of memory usage information, including optional graphical representations. The size of each 'row' is defined by MAX_STR_LEN.
 * @return void
 */
void memoryUsage(int samples, int i, MemoryInfo info, bool graph, bool seq, double *previous, char record[][MAX_STR_LEN]);

/**
 * @brief Retrieves memory statistics and writes them to a pipe.
 * 
 * This function gathers memory statistics using sysinfo, populates a MemoryInfo struct
 * with total and used memory (physical and virtual), and writes this information to the
 * specified pipe. If an error occurs during retrieval or writing, the process and its
 * parent are terminated.
 *
 * @param pipe An array of two integers representing the read and write ends of a pipe.
 * @return void
 */
void memoryStats(int pipe[2]);

/**
 * @brief Appends a graphical representation of memory usage change to a specific index of a 2D character array.
 * 
 * This function calculates the difference in memory usage since the last call and creates a visual representation of this change.
 * It then appends this graphical representation to the memory usage string stored at the specified index of the 2D array.
 * The visual representation includes a line of characters indicating the magnitude of the change, followed by a special
 * character representing the direction of the change (increase, decrease, or no change). Additionally, the function
 * formats and appends textual information about the absolute difference in memory usage and the current memory usage.
 *
 * @param memory_current The current memory usage measurement.
 * @param memory_previous A pointer to a double value representing the previous memory usage measurement. This value is updated
 *                        with `memory_current` after the graphical representation is generated.
 * @param i The index of the 2D array `record` where the memory usage information and graphical representation will be appended.
 *          This index corresponds to a specific snapshot or time point in the memory monitoring process.
 * @param record A 2D character array where each 'row' is designated to store a snapshot of memory usage information, including
 *               the graphical representation. The array should have a sufficient number of rows to accommodate all desired
 *               snapshots, and each row must have a length defined by MAX_STR_LEN to ensure there is enough space for the information.
 * 
 * @note The function assumes that `record[i]` has been properly initialized and contains any initial memory usage information
 *       before the graphical representation is appended. The function does not clear or overwrite existing content in `record[i]`
 *       except to append the new graphical representation.
 */
void appendMemoryGraphicToTail(double memory_current, double *memory_previous, int i, char record[][MAX_STR_LEN]);

/**
 * @brief Reads login records from the utmp file and writes them to a pipe.
 * 
 * This function sets the path to the utmp file to read login records and iterates
 * through each record. For each user process found (indicating an active user session),
 * it formats a string containing the username, session ID, and host, and writes this
 * string to the specified pipe. If any step fails, it outputs an error message and
 * terminates the current process as well as its parent process.
 *
 * @param pipe An array of two integers representing the read and write ends of a pipe.
 * @return void
 */
void userOutput(int pipe[2]);

/**
 * @brief Counts the number of processor cores available on the system.
 * 
 * This function reads the "/proc/cpuinfo" file to count the number of occurrences
 * of the string "processor", which corresponds to an individual core. It returns
 * the total count of processor cores found. If the file cannot be opened, it returns -1,
 * indicating an error.
 * 
 * @return The number of processor cores on the system. Returns -1 if the file cannot be opened.
 */
int count_cores();

/**
 * @brief Retrieves CPU usage statistics and writes them to a pipe.
 * 
 * This function opens and reads from the "/proc/stat" file to gather CPU statistics,
 * including time spent in different modes (user, nice, system, idle, iowait, irq,
 * softirq). It calculates total CPU time and idle time, encapsulates these values
 * in a CPU structure, and writes the structure to the specified pipe. If an error occurs
 * during any step of the process, it outputs an error message and terminates both the
 * current and parent processes to prevent further execution.
 *
 * @param pipe An array of two integers representing the read and write ends of a pipe.
 * @return void
 */
void cpuStats(int pipefd[2]);

/**
 * @brief Calculates current CPU usage, optionally appends a graphical representation to a record array, and prints CPU usage information.
 * 
 * This function calculates the current CPU usage based on the difference in total and idle CPU times since the last measurement.
 * It then prints general CPU information, including the number of cores and the calculated CPU usage percentage. If graphical
 * output is enabled, it also calls `appendAndPrintCpuGraphics` to append a graphical representation of the current CPU usage to
 * the record array and print all records up to the current index.
 *
 * @param graphics Boolean flag indicating if graphical representation is enabled. When true, graphical CPU usage data is appended
 *                 to the record array and printed alongside textual CPU information.
 * @param i The current index for storing the graphical representation in the record array. This parameter is relevant only when
 *          graphical output is enabled.
 * @param cpu_previous Pointer to a long integer tracking the previous total CPU time. This value is updated with the current total
 *                     CPU time after calculating the current CPU usage.
 * @param time_previous Pointer to a long integer tracking the previous idle CPU time. This value is updated with the current idle
 *                      CPU time after calculating the current CPU usage.
 * @param info A structure containing the current total and idle CPU times.
 * @param record A 2D character array for storing graphical representations of CPU usage, used when graphical output is enabled. Each
 *               'row' in the array corresponds to a snapshot of CPU usage information, with the array size defined to accommodate
 *               all snapshots, and each 'row' having a length determined by MAX_STR_LEN.
 */
void cpu_output(bool graphics, int i, long int* cpu_previous, long int* time_previous, CPU info, char record[][MAX_STR_LEN]);

/**
 * @brief Constructs a graphical representation of CPU usage and appends it to the record array, then prints all records up to the current index.
 * 
 * This function creates a visual representation of the CPU usage, consisting of a series of bars corresponding to the usage percentage.
 * It then stores this graphical representation in the specified index of a 2D character array. After appending the graphical data,
 * the function prints each entry of the array up to and including the current one, effectively displaying the CPU usage history.
 *
 * @param usage The current CPU usage percentage to be graphically represented.
 * @param i The index in the 2D character array where the graphical representation of the current CPU usage is to be stored.
 * @param record A 2D character array where each 'row' stores a graphical representation of CPU usage. The array must be
 *               large enough to accommodate all entries, with each 'row' having a length defined by MAX_STR_LEN.
 */
void appendAndPrintCpuGraphics(double usage, int i, char record[][MAX_STR_LEN]);

/**
 * @brief Prints operating system information.
 * 
 * Retrieves information about the operating system using the uname function and prints
 * the system name, machine name, version, release, and architecture to the standard output.
 *
 * @return void
 */
void getOSInfo();

/**
 * @brief Prints the system's uptime.
 * 
 * Reads the system's uptime from "/proc/uptime", calculates the total time in days, hours,
 * minutes, and seconds format, and prints this information along with the total hours to the
 * standard output.
 *
 * @return void
 */
void getUptime();

/**
 * @brief Prints system information and uptime.
 * 
 * A convenience function that prints a divider followed by calling `getOSInfo` to print
 * operating system details, and `getUptime` to print the system uptime.
 *
 * @return void
 */
void systemInfo();

/**
 * @brief Signal handler for SIGINT and SIGTSTP signals.
 * 
 * This function provides custom handling for SIGINT (Ctrl+C) and SIGTSTP (Ctrl+Z) signals.
 * For SIGINT, it prompts the user to confirm if they want to quit the application. If the
 * user responds with 'y' or 'Y', the application exits successfully. For SIGTSTP, the
 * function simply returns, effectively ignoring the signal. If `scanf` fails to read the
 * input due to a signal interruption (EINTR), it prints a message and resumes. Other
 * `scanf` errors trigger a fatal error message and the application exits with a failure
 * status.
 *
 * @param sig The signal number of the received signal.
 * @return void
 */
void handle_sigint(int sig);

/**
 * @brief Checks if a string represents an integer.
 * 
 * Validates whether a given string consists solely of digit characters, potentially
 * preceded by a sign (+/-), thus representing a valid integer.
 *
 * @param str Pointer to the string to be checked.
 * @return Returns true if the string represents an integer, false otherwise.
 */
bool isInteger(const char *str);

/**
 * @brief Parses command-line arguments to configure application parameters.
 * 
 * This function interprets the command-line arguments provided to the application,
 * setting the appropriate parameters for sample size, delay, and various boolean flags
 * indicating the operational modes (system, user, sequential, and graphics). If no mode
 * is explicitly selected, it defaults to both user and system modes enabled. It supports
 * arguments in both '--key=value' and positional formats. Positional numeric arguments
 * are interpreted as 'samples' and 'delay' if not preceded by a flag, in that order.
 *
 * @param argc The number of command-line arguments.
 * @param argv Array of pointers to the argument strings.
 * @param samples Pointer to an int to store the sample size.
 * @param delay Pointer to an int to store the delay.
 * @param seq Pointer to a bool to indicate if sequential mode is enabled.
 * @param sys Pointer to a bool to indicate if system mode is enabled.
 * @param user Pointer to a bool to indicate if user mode is enabled.
 * @param graph Pointer to a bool to indicate if graphics mode is enabled.
 * @return Returns true if arguments are successfully parsed; otherwise, false.
 */
bool parseargument(int argc, char **argv, int* samples, int* delay,bool* seq, bool* sys, bool* user, bool* graph);

/**
 * @brief Collects and prints system information based on the provided parameters.
 * 
 * This function sets up signal handling for SIGINT and SIGTSTP, creates pipes for interprocess communication,
 * and forks separate processes to collect memory, CPU, and user session information. Each child process
 * executes a specific task and writes its output to a pipe. The parent process reads from these pipes
 * and prints the gathered information. It supports printing information in a sequential manner or all at once,
 * based on the 'seq' flag. It also handles graphical representation of the memory and CPU usage if the 'graph'
 * flag is enabled. After collecting and printing the specified number of samples with the given delay, it
 * cleans up by closing pipes and freeing allocated memory.
 *
 * @param samples Number of information samples to collect and print.
 * @param delay Time delay between each sample collection in seconds.
 * @param seq Boolean flag indicating if the output should be sequential.
 * @param sys Boolean flag indicating if system (CPU and memory) information should be collected.
 * @param user Boolean flag indicating if user session information should be collected.
 * @param graph Boolean flag indicating if graphical representation is enabled for memory and CPU usage.
 * @return void
 */
void printinfo(int samples, int delay, bool seq, bool sys, bool user, bool graph);

/**
 * @brief The entry point of the program.
 * 
 * Parses command-line arguments to determine the number of samples, delay between samples, and
 * flags for sequential output, system information collection, user session information collection,
 * and graphical representation. It then calls `printinfo` to collect and print the specified
 * information based on these parameters. Finally, it displays general system information before exiting.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return Returns 0 on successful execution and 1 on failure to parse arguments.
 */
int main(int argc, char **argv);

#endif