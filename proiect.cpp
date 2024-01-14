#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <signal.h>
#include <dirent.h>
#include <cstring>
#include <errno.h>
#include <unordered_map>
#include <string>
#include <iostream>

struct dateHash
{
    unsigned long long id, size, modified;
};

std::unordered_map <const char*, dateHash> cache;

unsigned long long job_ID;

long long calculateFolderSize(const char* folderPath)
{
    DIR *dir;
    struct dirent *entry;
    struct stat statBuf;
    long long totalSize = 0, size = 0;

    if ((dir = opendir(folderPath)) == NULL)
    {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    if (stat(folderPath, &statBuf) == -1)
        perror("stat");

    if (cache[folderPath].modified != statBuf.st_mtime)
    {
        std::cout << folderPath << '\n';
        std::cout << cache[folderPath].id << ' ';
        cache[folderPath].id = ++job_ID;
        std::cout << cache[folderPath].id << '\n' << cache[folderPath].modified << ' ' << statBuf.st_mtime << '\n';

        cache[folderPath].modified = statBuf.st_mtime;
        std::cout << cache[folderPath].modified << '\n';
        cache[folderPath].size = 0;

        // Iterate through each entry in the directory
        while ((entry = readdir(dir)) != NULL)
        {
            // Skip "." and ".." entries
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            // Construct the full path of the file
            char filePath[PATH_MAX];
            snprintf(filePath, sizeof(filePath), "%s/%s", folderPath, entry->d_name);

            // Get information about the file
            if (stat(filePath, &statBuf) == -1)
            {
                perror("stat");
                continue;  // Skip to the next entry if stat fails
            }

            // Regular file or shortcut file
            if (S_ISREG(statBuf.st_mode) || S_ISLNK(statBuf.st_mode))
            {
                size += statBuf.st_size;
            }

            else if (S_ISDIR(statBuf.st_mode))
            {
                char dirPath[PATH_MAX];
                snprintf(dirPath, sizeof(dirPath), "%s/%s", folderPath, entry->d_name);
                size += calculateFolderSize(dirPath);
            }
        }

        cache[folderPath].size = size;
    }
    else
    {
        std::cout << " precalculat" << folderPath  << "\n";
        size = cache[folderPath].size;
    }

    totalSize += size;

    closedir(dir);

    return totalSize;
}

std::pair<int, int> getFolderItemCount(const char *folderPath)
{
    DIR *dir;
    int nr_foldere=0, nr_fisiere=0;
    struct stat statBuf;
    struct dirent *entry;

    if ((dir = opendir(folderPath)) == NULL)
    {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL)
    {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Construct the full path of the file
        char filePath[PATH_MAX];
        snprintf(filePath, sizeof(filePath), "%s/%s", folderPath, entry->d_name);
        if(stat(filePath, &statBuf) == -1)
            perror("stat");

            if(S_ISDIR(statBuf.st_mode))
               {
                   
                char dirPath[PATH_MAX];
                nr_foldere++;
                  
                snprintf(dirPath, sizeof(dirPath), "%s/%s", folderPath, entry->d_name);
                
                std::cout << "\n" << entry->d_name << " : ";
                std::pair<int, int> aux = getFolderItemCount(dirPath);
                
                nr_foldere += aux.first;
                nr_fisiere += aux.second;
            }
            else{
                if(S_ISREG(statBuf.st_mode))
                {
                    std::cout << entry->d_name << ", ";
                    nr_fisiere++;
                }
            }
    }
    

    // Close the directory
    closedir(dir);
    std::cout << '\n';

    return std::make_pair(nr_foldere, nr_fisiere);
}



void outputCurrentFolderPath()
{
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) != NULL)
        printf("Current working directory: %s\n", cwd);
    else
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
}

int main()
{
    const char* s = "/mnt/c/Users/Lida Rani/dir/proiect/", *t = "/mnt/c/Users/Lida Rani/dir/", *p = "/mnt/c/Users/Lida Rani/dir/test";
    calculateFolderSize(s), calculateFolderSize(p);
    std::cout << calculateFolderSize(t) << '\n';
}
