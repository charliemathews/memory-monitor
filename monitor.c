/*
Authors:		Charlie Mathews 	charles@parkes.io
				Seth Loew			loewsd1@gcc.edu
				
Course: 		COMP 340, Operating Systems
Date: 			5/3/2015
Description: 	This file implements the functionality Project 3

Compile with:  	gcc monitor.c -o monitor
Run with:      ./monitor

	We decided that the best solution would be to insert a system("pause") so that we could print the memory
	information, run a process in another terminal, then continue the program to see how memory changed.
*/

/*
	#include <sys/sysinfo.h>
	...
	struct sysinfo vm ;
	sysinfo(&vm) ;
	printf("total: %lu, used: %lu\n", vm.totalram/1024, vm.freeram/1024);
*/

/*
	Estimating mem.available
	https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/?id=34e431b0ae398fc54ea69ff85ec700722c9da773
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

//Output live memory monitor to terminal?
//!!!THIS MUST BE ACTIVE TO TEST TASKS 3 and 4!!!
#define ACTIVE_MONITOR true

//Alert user when TOGGLE_LIMIT is passed?
#define LIMIT_ALERT true

//Limit in percentage at which the program alerts the user once. (Task 3)
#define TOGGLE_LIMIT 15

//Take action if limit is passed? (Task 4)
#define ACTIVE_MANAGEMENT false

//Update speed in seconds.
#define UPDATE_SPEED 1

//If running on school vm, change to true
#define OLD_KERNEL false

//Colors for printf
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define RESET "\033[0m"


struct Memory
{
	int total ;
	int available ;		// = free + buffers + cached // based on output from shell "free"
						// won't be available on the gcc vm because it has an outdated kernel
	int free ;
	int buffers ;
	int cached ;
};


struct Memory getMem();
double getMemPercent();
int getMemPID(int pid);
void monitor();
void spause();
void processdir(const struct dirent *dir);
int filter(const struct dirent *dir);

int main(int argc, char* argv[])
{
	// *  *  *  *  *  *  *  *  *  *  
	// LIST PROCESSES
	// *  *  *  *  *  *  *  *  *  * 
	/*
	struct dirent **namelist;
	int n;

	n = scandir("/proc", &namelist, filter, 0);
	
	if (n < 0) perror("Not enough memory to scan /proc.");
	else
	{
		while(n--)
		{
			processdir(namelist[n]);
			free(namelist[n]);
		}
		free(namelist);
	}
	*/
	
	
	//run an existing program
	//system("./tests/dp&");
	
	
	// *  *  *  *  *  *  *  *  *  *  
	// SINGLE PROCESS MEMORY USAGE
	// *  *  *  *  *  *  *  *  *  *  
	int samplePID = 2111 ;
	printf("PID %d is using %d kB\n", samplePID, getMemPID(samplePID));
	
	
	// *  *  *  *  *  *  *  *  *  *  
	// PROCESS FORK
	// *  *  *  *  *  *  *  *  *  *  
	/*
	int processes = 5;
	int i;
	for (i = 0; i < processes; ++i) {
	if (fork() == 0) {
	// do the job specific to the child process
	...
	// don't forget to exit from the child
	exit(0);
	}
	}
	// wait all child processes
	int status;
	for (i = 0; i < processes; ++i)
	wait(&status);
	*/
	
	
	//IDEA: spawn processes until memory limit is passed?
	
	
	// *  *  *  *  *  *  *  *  *  *  
	// MEMORY MONITOR
	// *  *  *  *  *  *  *  *  *  *  
	struct Memory prior, current ;
	current = getMem() ;
	double toggle_limit = (double)TOGGLE_LIMIT ;
	int toggle = 0 ;
	
	if(ACTIVE_MONITOR == true) while(1)
	{
		// Get current memory usage and calculate change since last update.
		prior = current ;
		current = getMem() ;
		//double change = (double)(prior.available-current.available)/(double)1000 ;
		int change = (prior.available-current.available) ;
		double memPercent = getMemPercent() ;
		
		//printf("free: %d, buffers %d, cached %d\n", current.free, current.buffers, current.cached);
		
		// Output memory usage.
		printf("\rtotal: %d kB, available: %d kB, usage: %.3f%%, change ", current.total, current.available, memPercent) ;
		if(change > 0) printf(KGRN "(%d kB)", change); // green means memory was reclaimed (freed)
		if(change == 0) printf(KWHT "(--)");
		if(change < 0) printf(KRED "(%d kB)", change); // red means memory was consumed (used)
		printf("        " RESET);
		fflush(stdout);
		
		//If alerts are enabled.
		if(LIMIT_ALERT == true && memPercent > toggle_limit)
		{
			if(toggle == 0)
			{
				toggle = 1 ;
				printf(KRED "\nWarning! Memory consumption has passed %.2f%%.\n" RESET, (double)TOGGLE_LIMIT);
				fflush(stdout);
			}
			
			//if memory reduction is enabled, search for user processes to kill and kill them
			if(ACTIVE_MANAGEMENT == true)
				while(getMemPercent() > TOGGLE_LIMIT)
				{
					//eliminate a process if possible, otherwise break loop
					
				}
		}
		
		//If alert was toggled before and usage drops below TOGGLE_LIMIT
		if(toggle == 1 && memPercent < toggle_limit) toggle = 0 ;
		
		//Wait before next update.
		sleep(UPDATE_SPEED) ;
	}

	return 1 ;
}


//Simulates a system("pause")
void spause()
{
	printf("Pause... (return to continue)"); 
	while ( getchar() != '\n') ;
}


//Output the current memory usage.
void monitor()
{
	struct Memory current = getMem() ;
	double memPercent = getMemPercent() ;

	// Output memory usage.
	printf("\rtotal: %d kB, available: %d kB, usage: %.3f%%\n", current.total, current.available, memPercent) ;
}


//Returns a struct Memory with info from /proc/meminfo in kB
struct Memory getMem() 
{
	struct Memory mem ;
	
	FILE *ifp = fopen("/proc/meminfo", "r");
	if(ifp == NULL)
	{
		printf("/rfailed to get meminfo!                                       \n") ;
		exit(1) ;
	}
	
	fscanf(ifp, "MemTotal:%d kB\n", &mem.total);
	fscanf(ifp, "MemFree:%d kB\n", &mem.free);
	if(OLD_KERNEL == false) fscanf(ifp, "MemAvailable:%d kB\n", &mem.available);
	fscanf(ifp, "Buffers:%d kB\n", &mem.buffers);
	fscanf(ifp, "Cached:%d kB\n", &mem.cached);
	
	// this is an estimation based on the information garnered from the linux kernel git and "free"
	if(OLD_KERNEL == true) mem.available = (mem.free + mem.buffers + mem.cached) ;
	
	return mem ;
}


//Use getMem() to calculate percentage of memory used.
double getMemPercent()
{
	struct Memory mem = getMem() ;
	return ((double)(mem.total-mem.available)/(double)mem.total)*100 ;
}


//Get memory usage in kB of a specific pid using /proc/{pid}/status -> VmSize
int getMemPID(int pid)
{
	FILE* fp;
	int myint;
	int BUFFERSIZE = 80;
	char* buf = malloc(85);
	
	char filename[30] = "" ;
	sprintf(filename,"/proc/%d/status",pid);

	if((fp = fopen(filename,"r")))
	{
		while(fgets(buf, BUFFERSIZE, fp) != NULL)
			if(strstr(buf, "VmSize") != NULL) 
				if (sscanf(buf, "%*s %d", &myint) == 1) return myint ; //printf("PID %d VmSize is %d\n", pid, myint);
	}
	
	return 0 ;
}


//Source: http://goo.gl/0VYHqg
void processdir(const struct dirent *dir)
{
     puts(dir->d_name);
}

//Trying to figure out how to get a list of processes IDs so that we can eliminate user processes
//Source: http://goo.gl/0VYHqg
int filter(const struct dirent *dir)
{
     uid_t user;
     struct stat dirinfo;
     int len = strlen(dir->d_name) + 7; 
     char path[len];

     strcpy(path, "/proc/");
     strcat(path, dir->d_name);
     user = getuid();
     if (stat(path, &dirinfo) < 0) {
	  perror("processdir() ==> stat()");
	  exit(EXIT_FAILURE);
     }
     return !fnmatch("[1-9]*", dir->d_name, 0) && user == dirinfo.st_uid;
}