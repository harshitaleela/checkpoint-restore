#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include "ckpt.h"

void print_segment_info(struct ckpt_segment *seg) {
    printf("Segment: %s\n", seg->name);
    printf("Start Address: %p\n", seg->start);
    printf("End Address: %p\n", seg->end);
    printf("Permissions: %s%s%s\n", 
           seg->read ? "r" : "-",
           seg->write ? "w" : "-",
           seg->execute ? "x" : "-");
    printf("Data Size: %d bytes\n", seg->data_size);
    printf("Is Context: %s\n", seg->is_context ? "Yes" : "No");
 //   printf("Is Canary: %s\n", seg->is_canary ? "Yes" : "No");
    printf("Offset: %ld\n", seg->offset);

}

void read_checkpoint(const char *filename) {
    int ckpt_fd = open(filename, O_RDONLY);
    if (ckpt_fd == -1) {
        perror("Error opening checkpoint file");
        exit(EXIT_FAILURE);
    }
    int i=0;

    struct ckpt_segment seg;
    while (read(ckpt_fd, &seg, sizeof(seg)) > 0) {
        printf("\n################ %d ##################\n", i++);
	print_segment_info(&seg);
	
        // Reading the data of the segment
        void *data = malloc(seg.data_size);
        if (data == NULL) {
            perror("Error allocating memory for segment data");
            close(ckpt_fd);
            exit(EXIT_FAILURE);
        }

        if (read(ckpt_fd, data, seg.data_size) != seg.data_size) {
            perror("Error reading segment data");
            free(data);
            close(ckpt_fd);
            exit(EXIT_FAILURE);
        }

        // You can add additional logic to process the data if needed
        // For example, you could restore it to memory using mmap.

        if (!seg.is_context) {
            printf("Segment Data (first 16 bytes):\n");
            for (int i = 0; i < seg.data_size && i < 16; i++) {
                printf("%02x ", ((unsigned char *)data)[i]);
            }
            printf("\n");
        } else {
            printf("This segment contains context data.\n");
        }

        free(data);
        printf("-----\n");
    }

    close(ckpt_fd);
    printf("Checkpoint reading completed.\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <checkpoint_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    read_checkpoint(argv[1]);
    return 0;
}

