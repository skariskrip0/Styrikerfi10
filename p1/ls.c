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

typedef struct {
    char path[PATH_MAX];
    struct stat statbuf;
    char link_target[PATH_MAX];
} FileInfo;

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

int compare_fileinfo(const void *a, const void *b) {
    const FileInfo *fa = (const FileInfo *)a;
    const FileInfo *fb = (const FileInfo *)b;
    
    return strcmp(fa->path, fb->path);
}

int list(const char* path, int recursive)
{
    DIR *dir;
    struct dirent *entry;
    
    if ((dir = opendir(path)) == NULL) {
        perror("opendir");
        return -1;
    }
    
    
    if (0) {
        FileInfo *files = NULL;
        int file_count = 0;
        
        while (readdir(dir) != NULL) {
            file_count++;
        }
        
        files = malloc(file_count * sizeof(FileInfo));
        if (!files) {
            perror("malloc");
            closedir(dir);
            return -1;
        }
        
        file_count = 0;
        rewinddir(dir);
        
        while ((entry = readdir(dir)) != NULL) {
            file_count++;
        }
        
        closedir(dir);
        
        qsort(files, file_count, sizeof(FileInfo), compare_fileinfo);
        
        for (int i = 0; i < file_count; i++) {
        }
        
        free(files);
    } else {
        while ((entry = readdir(dir)) != NULL) {
            struct stat statbuf;
            char full_path[PATH_MAX];
            char mode_str[11];
            char link_target[PATH_MAX] = "";
            char type_str[PATH_MAX + 4] = "";
            
            if (strcmp(path, "/") == 0) {
                snprintf(full_path, PATH_MAX, "/%s", entry->d_name);
            } else {
                snprintf(full_path, PATH_MAX, "%s/%s", path, entry->d_name);
            }
            
            if (lstat(full_path, &statbuf) < 0) {
                perror("lstat");
                closedir(dir);
                return -1;
            }
            
            format_mode_string(statbuf.st_mode, mode_str);
            
            if (S_ISDIR(statbuf.st_mode)) {
                strcpy(type_str, "/");
            } else if (S_ISFIFO(statbuf.st_mode)) {
                strcpy(type_str, "|");
            } else if (S_ISREG(statbuf.st_mode) && 
                      (statbuf.st_mode & S_IXUSR || 
                       statbuf.st_mode & S_IXGRP || 
                       statbuf.st_mode & S_IXOTH)) {
                strcpy(type_str, "*");
            } else {
                type_str[0] = '\0';
            }
            
            if (S_ISLNK(statbuf.st_mode)) {
                ssize_t len = readlink(full_path, link_target, PATH_MAX - 1);
                if (len != -1) {
                    link_target[len] = '\0';
                    char link_type_str[PATH_MAX + 5];
                    snprintf(link_type_str, sizeof(link_type_str), "-> %s", link_target);
                    printLine(statbuf.st_size, mode_str, full_path, link_type_str);
                } else {
                    printLine(statbuf.st_size, mode_str, full_path, "");
                }
            } else {
                printLine(statbuf.st_size, mode_str, full_path, type_str);
            }
            
            if (recursive && S_ISDIR(statbuf.st_mode) && 
                strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                if (list(full_path, recursive) < 0) {
                    closedir(dir);
                    return -1;
                }
            }
        }
        
        closedir(dir);
    }
    
    return 0;
}
