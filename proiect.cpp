#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>


int afiseaza_copii(const char *directory_path)
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir(directory_path);

    if (dir == NULL) 
    {
        perror("Error opening directory");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) 
        printf("%s\n", entry->d_name);

    closedir(dir);
    return 0;
}

//long long countFiles()

long long calculateFolderSize(const char *folderPath)
{
    DIR *dir;
    struct dirent *entry;
    struct stat statBuf;
    long long totalSize = 0;

    // Open the directory
    if ((dir = opendir(folderPath)) == NULL)
    {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

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

        // Check if the entry is a regular file
        if (S_ISREG(statBuf.st_mode))
        {
            // Accumulate the size of the file
            totalSize += statBuf.st_size;
        }
        else
        {
            char dirPath[PATH_MAX];
            snprintf(dirPath, sizeof(dirPath), "%s/%s", folderPath, entry->d_name);
            totalSize += calculateFolderSize(dirPath);
        }
    }

    // Close the directory
    closedir(dir);

    return totalSize;
}


pair<int, int> getFolderItemCount(const char *folderPath)
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
                
                   cout << "\n" << entry->d_name << " : ";
                pair<int, int> aux = getFolderItemCount(dirPath);
                
                nr_foldere += aux.first;
                nr_fisiere += aux.second;
            }
            else{
                if(S_ISREG(statBuf.st_mode))
                {
                    cout << entry->d_name << ", ";
                    nr_fisiere++;
                }
            }
    }
    

    // Close the directory
    closedir(dir);
    cout << '\n';

    return make_pair(nr_foldere, nr_fisiere);
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

/*
int parcurgere_dir(const char *directory_path)
{
     DIR *dir;
    struct dirent *entry;

    dir = opendir(directory_path);

    if (dir == NULL) {
        perror("Error opening directory");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        parcurgere_dir(const char *directory_path);
    }
}
*/
/*
int main(int argc, char *argv[])
{
    pid_t pid;

    //Ne despartim de procesul de baza
    pid = fork();

    if (pid < 0)
        return errno;
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Step 4: Set a new session
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    // Step 5: Change the current working directory to the root
    chdir("/");

    // Step 6: Close Standard File Descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Daemon specific action
    while (1) {


        char buffer[100];
        fgets(buffer, sizeof(buffer), stdin);

        if (buffer != NULL)
        {

        }

        time_t now = time(NULL);
        sleep(5); // The daemon works every 5 seconds
    }

    return EXIT_SUCCESS;
}

*/

int main()
{
    const char *s = "/mnt/c/Users/Lida Rani/dir/";
    printf("%lld\n", calculateFolderSize(s));
}
