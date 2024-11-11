#include <assert.h>
#include <stdio.h>
#include <ucontext.h>
#ifndef __CKPT_H__
#define __CKPT_H__
struct ckpt_segment
{
	void *start;
	void *end;
	long offset;
	int read;
	int write;
	int execute;
	int is_context;
	int data_size;
	int dummy;
	char name[256];
};

void read_next_line(int fd, char* line)
{
	assert(line != NULL);
	char ch = 0;
	while (ch != '\n') {
		int ret = read(fd, &ch, 1);
		if (ret == 0) {
			break;
		}
		*line = ch;
		line++;
	}
	*line = '\0';
	return;
}

int match_one_line(int proc_maps_fd, struct ckpt_segment *proc_maps_line) 
{
	unsigned long int start, end;
	int deviceMajor;
	int deviceMinor;
	char device[16] = "";
	long int inode = 0;
  	char rwxp[4];
	char tmp[10];
	unsigned long int offset;
  	
	char line[1024];
	read_next_line(proc_maps_fd, line);

	if (line[0] == '\0')
	{
    		proc_maps_line -> start = NULL;
    		proc_maps_line -> end = NULL;
    		return EOF;
  	}

  	int rc = sscanf(line, "%lx-%lx %4c %lx %x:%x %ld %[^\n]",
                 	&start, &end, rwxp, &offset, &deviceMajor, &deviceMinor, &inode, proc_maps_line->name);

//	printf("filename: <%s>\n", proc_maps_line->name);
//	printf("offset: <%lx>\n", offset);
//	printf("data read\n");
	proc_maps_line -> start = (void *)start;
  	proc_maps_line -> end = (void *)end;
  	proc_maps_line -> offset = offset;
  	proc_maps_line -> read = (rwxp[0] == 'r') ? 1 : 0;
	proc_maps_line -> write = (rwxp[1] == 'w') ? 1 : 0;
	proc_maps_line -> execute = (rwxp[2] == 'x') ? 1 : 0;
	proc_maps_line -> data_size = end - start;
	proc_maps_line -> is_context = 0;
  	return 0;
}
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


#endif
