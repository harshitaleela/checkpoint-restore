#include <assert.h>
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

	printf("filename: <%s>\n", proc_maps_line->name);
	printf("offset: <%lx>\n", offset);
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
#endif
