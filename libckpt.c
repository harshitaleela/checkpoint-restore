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
#include <asm/prctl.h>

ucontext_t context;
int dummy = 0;


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

		if (strstr(header[i].name, "[vsyscall]") || (header[i].read==0))
		{	continue;
		}

		write(ckpt_fd, &header[i], sizeof(header[i]));
		if(!(strstr(header[i].name, "[vvar]") || strstr(header[i].name, "[vdso]")))
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
	unsigned long int fsBase;
       
        
//	int i=10;
	getcontext(&context);
	
//	fflush(stdout);
	if (is_restart == 1)
	{	
//		print_registers(context, "post-ckpt");	
//		print_maps();
		//signal(SIGUSR2, &signal_handler);
		is_restart = 0;
		arch_prctl(ARCH_SET_FS, fsBase);
//		print_registers(context, "pre-ckpt");
//		fflush(stdout);
//		while(dummy);
//		printf("%d\n", i);
//		fflush(stdout);
		return;
	}	

	else
	{
//		print_registers(context, "pre-ckpt");
		//print_maps();
		arch_prctl(ARCH_GET_FS, &fsBase);
		is_restart = 1;
//		print_registers(context, "pre-ckpt");
		save_ckpt();
	}
}

void signal_handler(int sig)
{
	signal_handler_work(sig);
//	printf("Returned from signal_handler_work\n");
//	fflush(stdout);
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
