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
#include <limits.h>
#include <utility>
#include <iostream>
using namespace std;

struct dateHash
{
    unsigned long long id, size, modified;
    int fisiere_verif, nr_foldere, nr_fisiere;
    int stare; // -1- terminat, 0, pauza, 1, in progres
    const char* start_folder;
};

std::unordered_map <string, dateHash> cache;

//vector <string>

unsigned long long job_ID;

void initialize_cache(const char* folderPath,  struct stat statBuf)
{
    cout << folderPath << '\n';
    
    job_ID++;
    cache[folderPath].id = job_ID;
    //cache[folderPath].size = 0;
    cache[folderPath].modified =statBuf.st_mtime;
    cache[folderPath].stare =  1;
    
}

dateHash calculateFolderSize(const char* folderPath)
{
    DIR *dir;
    struct dirent *entry;
    struct stat statBuf;
    int fisiere_verif = 0, totalFisiere_verif=0;
    //int nr_foldere = 0, totalFoldere = 0;
    //int nr_fisiere =0, totalFisiere = 0;
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
        initialize_cache(folderPath, statBuf);

        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char filePath[PATH_MAX];
            snprintf(filePath, sizeof(filePath), "%s/%s", folderPath, entry->d_name);

            // Get information about the file
            if (stat(filePath, &statBuf) == -1)
            {
                perror("stat");
                continue;  // Skip to the next entry if stat fails
            }

            if (S_ISREG(statBuf.st_mode) || S_ISLNK(statBuf.st_mode))
            {
                size += statBuf.st_size;
                fisiere_verif++;
                //nr_fisiere++;
                cache[cache[folderPath].start_folder].fisiere_verif++;
            }
            else if (S_ISDIR(statBuf.st_mode))
            {
                char dirPath[PATH_MAX];
                snprintf(dirPath, sizeof(dirPath), "%s/%s", folderPath, entry->d_name);
                cache[dirPath].start_folder = cache[folderPath].start_folder;
                dateHash aux = calculateFolderSize(dirPath);
                size += aux.size;
                fisiere_verif += aux.fisiere_verif;
                //fisiere_verif += aux.fisiere_verif;
                //nr_fisiere += aux.nr_fisiere;
                //nr_foldere += aux.nr_foldere;
                //cache[start_folder].fisiere_verif += aux.fisiere_verif;
            }
        }
        cache[folderPath].size = size;
        cache[folderPath].fisiere_verif = fisiere_verif;
        //cache[folderPath].nr_fisiere = nr_fisiere;
        //cache[folderPath].nr_foldere = nr_foldere;
    }
    else
    {
        std::cout << " precalculat" << folderPath  << "\n";
        //cache[start_folder].fisiere_verif += cache[folderPath].fisiere_verif;
    }

    closedir(dir);

    return cache[folderPath];
}


dateHash countItemsFolder(const char *folderPath)
{
    DIR *dir;
    int nr_foldere=0, nr_fisiere=0, nr_verif = 0;
    struct stat statBuf;
    struct dirent *entry;

    if ((dir = opendir(folderPath)) == NULL)
    {
        perror("opendir");
        exit(EXIT_FAILURE);
    }
    
    if (stat(folderPath, &statBuf) == -1)
        perror("stat");

    if (cache[folderPath].modified != statBuf.st_mtime)
    {
        //aici nu punem ca fi modificat, doar in CalculateSize
        
        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            
            char filePath[PATH_MAX];
            snprintf(filePath, sizeof(filePath), "%s/%s", folderPath, entry->d_name);
            if(stat(filePath, &statBuf) == -1)
                perror("stat");
            
            if (S_ISREG(statBuf.st_mode) || S_ISLNK(statBuf.st_mode))
            {
                //cout << entry->d_name << ", ";
                nr_fisiere++;
            }
            else
            {
                char dirPath[PATH_MAX];
                nr_foldere++;
                
                //cout << "\n" << entry->d_name << " : ";
                snprintf(dirPath, sizeof(dirPath), "%s/%s", folderPath, entry->d_name);
                
                cache[dirPath].start_folder = cache[folderPath].start_folder;
                dateHash aux = countItemsFolder(dirPath);
                
                nr_foldere += aux.nr_foldere;
                nr_fisiere += aux.nr_fisiere;
                nr_verif += aux.fisiere_verif;
            }
        }
    }
    else
    {
        std::cout << " precalculat" << folderPath  << "\n";
        nr_fisiere = cache[folderPath].nr_fisiere;
        nr_foldere = cache[folderPath].nr_foldere;
        nr_verif = cache[folderPath].fisiere_verif;
        
    }
    cache[folderPath].nr_foldere = nr_foldere;
    cache[folderPath].nr_fisiere = nr_fisiere;
    cache[folderPath].fisiere_verif = nr_verif;

    // Close the directory
    closedir(dir);
    cout << '\n';

    return cache[folderPath];
}


void Progress(const char *folderPath)
{
    cout << "verif: " << cache[folderPath].fisiere_verif << " total fisiere : " << cache[folderPath].nr_fisiere  << " total foldere : " << cache[folderPath].nr_foldere << '\n';
    
    //printf (" deja verif: %d , total: %d \n", cache[folderPath].fisiere_verif, cache[folderPath].nr_fisiere);
    
}

void StartTask(const char *folderPath)
{
    cache[folderPath].start_folder = folderPath;
    //aici pe threaduri separate
    countItemsFolder(folderPath);
    calculateFolderSize(folderPath);
}


int main()
{
    const char *s1 = "/Users/anastasiastefanescu/Documents/SO";
    const char *s2= "/Users/anastasiastefanescu/Documents";
   
    //printf("%lld foldere, %lld fisiere \n", getFolderItemCount(s).first, getFolderItemCount(s).second);
    //printf("\n %lf GB \n", calculateFolderSize(s)/1000000);
    
    StartTask(s2);
    
    Progress(s2);
    
    StartTask(s1);
    
    Progress(s1);
    return 0;
}

//atentie - obligatoriu cand CalculateSize ii dam CountItems, dar pe un thread separat(ca sa nu pierdem timpul) - pt ca vrem sa stim cat mai repede nr de fisiere, nr_foldere si cate fisiere au mai fost verificate in taskuri anterioare.

//In CalculateSize, nu adaugam la nr_verificate daca am gasit un folder deja calculat - adaugam numai fisierele neparcurse pana acum
