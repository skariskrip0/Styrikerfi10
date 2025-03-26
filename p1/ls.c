/* 
 * Group number (on canvas): Sindri/Óskar
 * Student 1 name: Óskar
 * Student 2 name: Sindri 
 */

#include "ls.h"

// You may not need all these headers ...
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>  // For PATH_MAX

// Define a structure to hold file information
typedef struct {
    char path[PATH_MAX];
    struct stat statbuf;
    char link_target[PATH_MAX];
} FileInfo;

// Helper function to get file type character
char get_file_type(mode_t mode) {
    if (S_ISREG(mode)) return '-';
    if (S_ISDIR(mode)) return 'd';
    if (S_ISLNK(mode)) return 'l';
    if (S_ISFIFO(mode)) return 'p';
    if (S_ISCHR(mode)) return 'c';
    if (S_ISBLK(mode)) return 'b';
    if (S_ISSOCK(mode)) return 's';
    return '?';
}

// Helper function to format mode string
void format_mode_string(mode_t mode, char *str) {
    str[0] = get_file_type(mode);
    
    // Owner permissions
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    
    // Group permissions
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    
    // Others permissions
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    
    str[10] = '\0';
}

// Comparison function for qsort - sort by path name
int compare_fileinfo(const void *a, const void *b) {
    const FileInfo *fa = (const FileInfo *)a;
    const FileInfo *fb = (const FileInfo *)b;
    
    // Simple byte-wise comparison (no locale consideration)
    return strcmp(fa->path, fb->path);
}

// Process a single file and print its information
void process_file_info(const FileInfo *info) {
    char mode_str[11];
    char type_str[PATH_MAX + 4] = ""; // Space for " -> " + target
    
    // Format the mode string
    format_mode_string(info->statbuf.st_mode, mode_str);
    
    // Determine the type string
    if (S_ISDIR(info->statbuf.st_mode)) {
        strcpy(type_str, "/");
    } else if (S_ISFIFO(info->statbuf.st_mode)) {
        strcpy(type_str, "|");
    } else if (S_ISREG(info->statbuf.st_mode) && 
              (info->statbuf.st_mode & S_IXUSR || 
               info->statbuf.st_mode & S_IXGRP || 
               info->statbuf.st_mode & S_IXOTH)) {
        strcpy(type_str, "*");
    }
    
    // Handle symbolic links
    if (S_ISLNK(info->statbuf.st_mode) && info->link_target[0] != '\0') {
        snprintf(type_str, sizeof(type_str), " -> %s", info->link_target);
    }
    
    // Print the file information
    printLine(info->statbuf.st_size, mode_str, info->path, type_str);
}

int list(const char* path, int recursive)
{
    DIR *dir;
    struct dirent *entry;
    FileInfo *files = NULL;
    int file_count = 0;
    int capacity = 0;
    
    // Open the directory
    if ((dir = opendir(path)) == NULL) {
        perror("opendir");
        return -1;
    }
    
    // First, count the files to allocate memory
    while (readdir(dir) != NULL) {
        file_count++;
    }
    
    // Allocate memory for file info
    files = malloc(file_count * sizeof(FileInfo));
    if (!files) {
        perror("malloc");
        closedir(dir);
        return -1;
    }
    
    // Reset file count and directory stream
    file_count = 0;
    rewinddir(dir);
    
    // Read directory entries
    while ((entry = readdir(dir)) != NULL) {
        // Build the full path
        if (strcmp(path, "/") == 0) {
            // Handle root directory specially to avoid double slash
            snprintf(files[file_count].path, PATH_MAX, "/%s", entry->d_name);
        } else {
            snprintf(files[file_count].path, PATH_MAX, "%s/%s", path, entry->d_name);
        }
        
        // Get file info
        if (lstat(files[file_count].path, &files[file_count].statbuf) < 0) {
            perror("lstat");
            closedir(dir);
            free(files);
            return -1;
        }
        
        // Handle symbolic links
        if (S_ISLNK(files[file_count].statbuf.st_mode)) {
            ssize_t len = readlink(files[file_count].path, files[file_count].link_target, PATH_MAX - 1);
            if (len != -1) {
                files[file_count].link_target[len] = '\0';
            } else {
                files[file_count].link_target[0] = '\0';
            }
        } else {
            files[file_count].link_target[0] = '\0';
        }
        
        file_count++;
    }
    
    closedir(dir);
    
    // Sort the files by name
    qsort(files, file_count, sizeof(FileInfo), compare_fileinfo);
    
    // Process and print the sorted files
    for (int i = 0; i < file_count; i++) {
        process_file_info(&files[i]);
        
        // Recursively process directories if requested
        if (recursive && S_ISDIR(files[i].statbuf.st_mode)) {
            const char *basename = strrchr(files[i].path, '/');
            if (basename) {
                basename++; // Skip the slash
            } else {
                basename = files[i].path;
            }
            
            // Skip . and .. to avoid infinite recursion
            if (strcmp(basename, ".") != 0 && strcmp(basename, "..") != 0) {
                if (list(files[i].path, recursive) < 0) {
                    free(files);
                    return -1;
                }
            }
        }
    }
    
    // Free allocated memory
    free(files);
    
    return 0;
}
