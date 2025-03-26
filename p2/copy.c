/* 
 * Group number (on canvas): Sindri/Óskar
 * Student 1 name: Óskar
 * Student 2 name: Sindri 
 */

#define _POSIX_C_SOURCE 2
#include "copy.h"
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

// No need to change this. Parses argc into the CopyArgs structure
int parseCopyArgs(int argc, char * const argv[], CopyArgs* args)
{
	if (args == NULL) {
		return -1;
	}

	// Initialize with default values
	args->blocksize  = 4096;

	int opt;
	while ((opt = getopt(argc, argv, "b:")) != -1) {
		switch (opt) {
			case 'b':
				args->blocksize = atoi(optarg);

				if ((errno != 0) || (args->blocksize <= 0)) {
					return -1; // Invalid blocksize argument.
				}

				break;

			default: /* '?' */
				return -1;
		}
	}

	if (optind != argc - 2) {
		return -1; // We expect two parameters after the options.
	}

	args->src = argv[optind];
	args->dst = argv[optind + 1];

	return 0;
}


int doCopy(CopyArgs* args)
{
	if (args == NULL) {
		return -1;
	}

	// ----------------
	// Copy the file.
	// ----------------
    int src_fd, dst_fd;
    ssize_t bytes_read, bytes_written;
    char *buffer;
    struct stat src_stat;
    int all_zeros;
    off_t offset;
    
    src_fd = open(args->src, O_RDONLY);
    if (src_fd == -1) {
        perror("Error opening source file");
        return -1;
    }
    
    
    if (fstat(src_fd, &src_stat) == -1) {
        perror("Error getting source file stats");
        close(src_fd);
        return -1;
    }
    
    dst_fd = open(args->dst, O_WRONLY | O_CREAT | O_EXCL, src_stat.st_mode);
    if (dst_fd == -1) {
        perror("Error opening destination file");
        close(src_fd);
        return -1;
    }
    
    buffer = malloc(args->blocksize);
    if (buffer == NULL) {
        perror("Error allocating buffer");
        close(src_fd);
        close(dst_fd);
        return -1;
    }
    
    while ((bytes_read = read(src_fd, buffer, args->blocksize)) > 0) {
        all_zeros = 1;
        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] != 0) {
                all_zeros = 0;
                break;
            }
        }
        
        if (all_zeros) {
            offset = lseek(dst_fd, bytes_read, SEEK_CUR);
            if (offset == -1) {
                perror("Error seeking in destination file");
                free(buffer);
                close(src_fd);
                close(dst_fd);
                return -1;
            }
        } else {
            bytes_written = write(dst_fd, buffer, bytes_read);
            if (bytes_written != bytes_read) {
                perror("Error writing to destination file");
                free(buffer);
                close(src_fd);
                close(dst_fd);
                return -1;
            }
        }
    }
    
    if (bytes_read == -1) {
        perror("Error reading from source file");
        free(buffer);
        close(src_fd);
        close(dst_fd);
        return -1;
    }
    
    
    free(buffer);
    close(src_fd);
    close(dst_fd);
    
    return 0;
}
