#include <stdio.h>
#include <unistd.h>
#include <ucontext.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/mman.h>

struct ckpt_segment
{
    void *start;
    void *end;
    int read, write, execute;
    int is_context;
    int data_size;
    char name[80];
};

void stack_unmap()
{
	 int proc_maps_fd = open("/proc/self/maps", O_RDONLY);
	 unsigned long int start, end;
         char filename[80] = " ";
         char rwxp[4];
         char tmp[10];
	 int rc = -2;
         int tmp_stdin = dup(0);
         dup2(proc_maps_fd, 0);

	 for (int i=0; rc!=EOF; i++)
	 {
         	rc = scanf("%lx-%lx %4c %*s %*s %*[0-9 ]%[^\n]\n",
                 	       &start, &end, rwxp, filename);
         	assert(fseek(stdin, 0, SEEK_CUR) == 0);
		if (strstr(filename, "[stack]"))
		{
			printf("stack %lx-%lx\n", start, end);
			fflush(stdout);
			int err = munmap((void *)start, end-start);
			if (err == -1)
            		{
                		perror("munmap");
                		exit(1);
            		}
			return;
		}
	 }
}	 

void restart()
{
    stack_unmap();
    ucontext_t context;
    int fd = open("myckpt", O_RDONLY);

    if (fd == -1)
    {
        perror("open");
        exit(1);
    }

    struct ckpt_segment header[100];
    int i = 0;
    int rc = read(fd, &header[i], sizeof(header[i]));

    while (rc > 0)  
    {
        while (rc < sizeof(header[i]))
        {
            rc += read(fd, (char *)&header[i] + rc, sizeof(header[i]) - rc);
        }

        if (header[i].is_context)
        {
            rc = read(fd, &context, sizeof(context));
            while (rc < sizeof(context))
            {
                rc += read(fd, (char *)&context + rc, sizeof(context) - rc);
            }
        }
        else
        {
            printf("Mapping segment %d: start=%p, size=%d\n", i, header[i].start, header[i].data_size);

            void *addr = mmap(header[i].start, header[i].data_size,
                              PROT_READ | PROT_WRITE | PROT_EXEC,
                              MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

            if (addr == MAP_FAILED)
            {
                perror("mmap");
                exit(1);
            }

            rc = read(fd, (char *)addr, header[i].data_size);
            while (rc < header[i].data_size)
            {
                rc += read(fd, (char *)addr + rc, header[i].data_size - rc);
            }
        }

        i++;
        rc = read(fd, &header[i], sizeof(header[i]));  // Read the next segment
    }

    close(fd);

    printf("Restoring context...\n");
    setcontext(&context);
    perror("setcontext");
}

void recursive(int level)
{
    if (level > 0)
        recursive(level - 1);
    else
        restart();
}

void map_memory()
{
	size_t size = 0x0010000; 
	void  *start_add = (void *)0x6000000;

	void *memory = mmap(start_add, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);

	 if (memory == MAP_FAILED)
         {
                perror("mmap");
                exit(1);
         }

	void *stack_ptr = start_add + size - 16;
	asm volatile ("mov %0, %%rsp" : : "g" (stack_ptr) : "memory");
        restart();
}

int main()
{
//    recursive(100);
    map_memory();
    return 0;
}

