#define _GNU_SOURCE
#define _POSIX_C_SOURCE

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <ucontext.h>
#include <fcntl.h>
#include <string.h>
#include "ckpt.h"

ucontext_t context;
int dummy = 0;

void print_registers(ucontext_t context, const char *stage) {
    printf("---- %s Register State ----\n", stage);

    // General-purpose registers for x86-64 architecture
    printf("RIP: %llx\n", (unsigned long long)context.uc_mcontext.gregs[REG_RIP]); fflush(stdout);
    printf("RSP: %llx\n", (unsigned long long)context.uc_mcontext.gregs[REG_RSP]); fflush(stdout);
    printf("RBP: %llx\n", (unsigned long long)context.uc_mcontext.gregs[REG_RBP]); fflush(stdout);
    printf("RAX: %llx\n", (unsigned long long)context.uc_mcontext.gregs[REG_RAX]); fflush(stdout);
    printf("RBX: %llx\n", (unsigned long long)context.uc_mcontext.gregs[REG_RBX]); fflush(stdout);
    printf("RCX: %llx\n", (unsigned long long)context.uc_mcontext.gregs[REG_RCX]); fflush(stdout);
    printf("RDX: %llx\n", (unsigned long long)context.uc_mcontext.gregs[REG_RDX]); fflush(stdout);
    printf("RSI: %llx\n", (unsigned long long)context.uc_mcontext.gregs[REG_RSI]); fflush(stdout);
    printf("RDI: %llx\n", (unsigned long long)context.uc_mcontext.gregs[REG_RDI]); fflush(stdout);
    printf("R8:  %llx\n", (unsigned long long)context.uc_mcontext.gregs[REG_R8]);  fflush(stdout);
    printf("R9:  %llx\n", (unsigned long long)context.uc_mcontext.gregs[REG_R9]);  fflush(stdout);
    printf("R10: %llx\n", (unsigned long long)context.uc_mcontext.gregs[REG_R10]); fflush(stdout);
    printf("R11: %llx\n", (unsigned long long)context.uc_mcontext.gregs[REG_R11]); fflush(stdout);
    printf("R12: %llx\n", (unsigned long long)context.uc_mcontext.gregs[REG_R12]); fflush(stdout);
    printf("R13: %llx\n", (unsigned long long)context.uc_mcontext.gregs[REG_R13]); fflush(stdout);
    printf("R14: %llx\n", (unsigned long long)context.uc_mcontext.gregs[REG_R14]); fflush(stdout);
    printf("R15: %llx\n", (unsigned long long)context.uc_mcontext.gregs[REG_R15]); fflush(stdout);
}

void print_segment_info(struct ckpt_segment *seg) {
    printf("------------------------------------------------------");
    printf("Segment: %s\n", seg->name);		fflush(stdout);
    printf("Start Address: %p\n", seg->start);	fflush(stdout);
    printf("End Address: %p\n", seg->end);	fflush(stdout);
    printf("Permissions: %s%s%s\n",
           seg->read ? "r" : "-",
           seg->write ? "w" : "-",
           seg->execute ? "x" : "-");		fflush(stdout);
    printf("Data Size: %d bytes\n", seg->data_size); fflush(stdout);
    printf("Is Context: %s\n", seg->is_context ? "Yes" : "No"); fflush(stdout);
//    printf("Is Canary: %s\n", seg->is_canary ? "Yes" : "No"); fflush(stdout);
}



void print_maps()
{
        struct ckpt_segment proc_maps_line;
        int proc_maps_fd = open("/proc/self/maps", O_RDONLY);
        int rc = -2;

        if (proc_maps_fd == -1)
        {
                perror("File open failed");
                exit(1);
        }

        int i;

        for (i=0; rc!=EOF; i++)
        {
                rc = match_one_line(proc_maps_fd, &proc_maps_line);
                print_segment_info(&proc_maps_line);
        }

}

void save_ckpt()
{
	int ckpt_fd = open("myckpt", O_WRONLY | O_CREAT | O_TRUNC, 0600);
	int proc_maps_fd = open("/proc/self/maps", O_RDONLY);
	int rc = -2;

	if (ckpt_fd == -1 || proc_maps_fd == -1)
	{
		perror("File open failed");
		exit(1);
	}

	struct ckpt_segment header[150];
	int i;

	for (i=0; rc!=EOF; i++)
	{
		rc = match_one_line(proc_maps_fd, &header[i]);

		if (strstr(header[i].name, "[vsyscall]") || strstr(header[i].name, "[vvar]") || (header[i].read==0))
		{	continue;
		}

		write(ckpt_fd, &header[i], sizeof(header[i]));
		write(ckpt_fd, (void *)header[i].start, header[i].data_size);
	}


	
//	printf("Memory segments saved");
	header[i].data_size = sizeof(ucontext_t);
	//header.name = "context";
	strncpy(header[i].name, "context", strlen("context")+1);
	header[i].is_context = 1;
	write(ckpt_fd, &header[i], sizeof(header[i]));
        write(ckpt_fd, &context, header[i].data_size);
	
	close(ckpt_fd);
	close(proc_maps_fd);

}


void signal_handler_work(int sig)
{
//	printf("signal received\n");
	static int is_restart;
	is_restart = 0;
//	int i=10;
	getcontext(&context);
	
	fflush(stdout);
	if (is_restart == 1)
	{	
//		print_registers(context, "post-ckpt");	
//		print_maps();
		//signal(SIGUSR2, &signal_handler);
		is_restart = 0;
//		print_registers(context, "pre-ckpt");
//		fflush(stdout);
//		while(dummy);
//		printf("%d\n", i);
//		fflush(stdout);
		return;
	}	

	else
	{
		print_registers(context, "pre-ckpt");
		//print_maps();
		is_restart = 1;
		//print_registers(context, "pre-ckpt");
		save_ckpt();
	}
}

void signal_handler(int sig)
{
	signal_handler_work(sig);
	printf("Returned from signal_handler_work\n");
	fflush(stdout);
	while(dummy);
}

void __attribute__((constructor))
myconstructor()
{
  struct sigaction act;
  memset(&act, 0, sizeof act);
  act.sa_handler = signal_handler;
  sigfillset(&act.sa_mask);
  act.sa_flags = SA_RESTART;

  sigaction(SIGUSR2, &act, NULL);
}
