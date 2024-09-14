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

ucontext_t context;


struct ckpt_segment
{
	void *start;
	void *end;
	int read, write, execute;
	int is_context;
	int data_size;
	char name[80];
};


int match_one_line(int proc_maps_fd, struct ckpt_segment *proc_maps_line) 
{
	unsigned long int start, end;
	char filename[80] = " ";
  	char rwxp[4];
	char tmp[10];
  	int tmp_stdin = dup(0); 
  	dup2(proc_maps_fd, 0); 
  	
  	int rc = scanf("%lx-%lx %4c %*s %*s %*[0-9 ]%[^\n]\n",
                 	&start, &end, rwxp, filename);
  	
  	assert(fseek(stdin, 0, SEEK_CUR) == 0);
  	dup2(tmp_stdin, 0); 
  	close(tmp_stdin);
  	
	if (rc == EOF || rc == 0) 
	{
    		proc_maps_line -> start = NULL;
    		proc_maps_line -> end = NULL;
    		return EOF;
  	}

  	if (rc == 3) 
	{ 	// if no filename on /proc/self/maps line:
    		strncpy(proc_maps_line -> name,
            		"ANONYMOUS_SEGMENT", strlen("ANONYMOUS_SEGMENT")+1);
  	} 
	else 
	{
    		assert( rc == 4 );
    		strncpy(proc_maps_line -> name, filename, 80);
    		proc_maps_line->name[80] = '\0';
  	}
  	
//	printf("data read\n");
	proc_maps_line -> start = (void *)start;
  	proc_maps_line -> end = (void *)end;
  	proc_maps_line -> read = (rwxp[0] == 'r') ? 1 : 0;
	proc_maps_line -> write = (rwxp[1] == 'w') ? 1 : 0;
	proc_maps_line -> execute = (rwxp[2] == 'x') ? 1 : 0;
	proc_maps_line -> data_size = end - start;
	proc_maps_line -> is_context = 0;
  	return 0;
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
	header[i].data_size = sizeof(context);
	//header.name = "context";
	strncpy(header[i].name, "context", strlen("context")+1);
	header[i].is_context = 1;
	write(ckpt_fd, &header[i], sizeof(header[i]));
        write(ckpt_fd, &context, header[i].data_size);
	
	close(ckpt_fd);
	close(proc_maps_fd);

}


static int is_restart;
void signal_handler_work(int sig)
{
//	printf("signal received\n");
	is_restart = 0;
	getcontext(&context);

	if (is_restart == 1)
	{		
		//signal(SIGUSR2, &signal_handler);
		//is_restart = 0;
		//while(1);
		return;
	}	

	else
	{
		is_restart = 1;
		save_ckpt();
	}
}

void signal_handler(int sig)
{
	signal_handler_work(sig);
	if (is_restart == 1)
	{		
		is_restart = 0;
		return;
	}	
}

void __attribute__((constructor))
myconstructor()
{
	signal(SIGUSR2, &signal_handler);
	return;
}
