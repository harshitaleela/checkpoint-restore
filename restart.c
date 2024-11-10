#define _GNU_SOURCE
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
#include "ckpt.h"

void restore_vdso(size_t size, void *orig_vdso_addr)
{
	void *new_vdso_addr;
	void *tmp_vdso_addr;
	int proc_maps_fd = open("/proc/self/maps", O_RDONLY);
	int i, rc;
        struct ckpt_segment line;
        for (i=0; rc!=EOF; i++)
        {
                rc = match_one_line(proc_maps_fd, &line);
                if(strstr(line.name, "vdso"))
                {
			new_vdso_addr = line.start;
		}
	}
	close(proc_maps_fd);
	tmp_vdso_addr = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (tmp_vdso_addr == MAP_FAILED) 
	{
            perror("mmap");
            return;
        }

	if (mremap(new_vdso_addr, size, size, MREMAP_MAYMOVE|MREMAP_FIXED, tmp_vdso_addr) == MAP_FAILED) 
	{
            perror("first mremap failed");
            munmap(tmp_vdso_addr, size);  // Clean up on failure
            return;
        }

	if (mremap(tmp_vdso_addr, size, size, MREMAP_MAYMOVE|MREMAP_FIXED, orig_vdso_addr) == MAP_FAILED)
        {
            perror("second mremap failed");
            munmap(tmp_vdso_addr, size);  // Clean up on failure
            return;
        }
	munmap(tmp_vdso_addr, size);

}

void stack_unmap()
{
	int proc_maps_fd = open("/proc/self/maps", O_RDONLY);
	int i, rc;
	struct ckpt_segment line;
	for (i=0; rc!=EOF; i++)
        {
                rc = match_one_line(proc_maps_fd, &line);
		if(strstr(line.name, "stack"))
		{
			int err = munmap(line.start, line.end-line.start);
                        if (err == -1)
                        {
                                perror("munmap");
                                exit(1);
                        }
		}
	}
	close(proc_maps_fd);
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
	    
	    int mapfd = -1;
	    int offset = 0;
	    int prot = PROT_WRITE;
	    int flags = MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS;
	    if (header[i].name[0] == '/') 
	    {
		    printf("Opening file for reading: %s\n", header[i].name);
		    mapfd = open(header[i].name, O_RDONLY);
		    assert(mapfd != -1);
		    offset = header[i].offset;
		    flags ^= MAP_ANONYMOUS;
	    } 
	    else if(strstr(header[i].name, "vdso"))
	    {
		    restore_vdso(header[i].data_size, header[i].start);
	    }
	    else {
	    }


	    if(header[i].read) prot|=PROT_READ;
	    if(header[i].write) prot|=PROT_WRITE;
	    if(header[i].execute) prot|=PROT_EXEC;

            void *addr = mmap(header[i].start, header[i].data_size,
			      prot, flags, mapfd, offset);
	    if (mapfd != -1) {
		    close(mapfd);
	    }

            if (addr == MAP_FAILED)
            {
                perror("mmap");
                exit(1);
            }

            rc = read(fd, (char *)addr, header[i].data_size);
            while (rc < header[i].data_size)
            {
                int ret = read(fd, (char *)addr + rc, header[i].data_size - rc);
		assert(ret != -1);
                rc += ret;
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

