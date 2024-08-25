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

void restart()
{
    ucontext_t context;
    int fd = open("myckpt", O_RDONLY);

    if (fd == -1)
    {
        perror("open");
        exit(1);
    }

    struct ckpt_segment header[1000];
    int i = 0;
    int rc = read(fd, &header[i], sizeof(header[i]));

    while (rc > 0)  
    {
        if (rc == -1)
        {
            perror("read");
            exit(1);
        }

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
            // Print debug information
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

    // Print before setcontext
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

int main()
{
    recursive(100);
    return 0;
}

