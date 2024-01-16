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
#include <unordered_map>
#include <iomanip>
using namespace std;

struct dateHash
{
    unsigned long long id, size, modified, min_folder_size;
    int fisiere_verif, nr_foldere, nr_fisiere;
    int stare; // -1- terminat, 0 - nu exista, 1  -in progres, 2- pauza
    int nr_units;
    const char* start_folder;
};

unordered_map <string, dateHash> cache;

vector <string> paths;

unsigned long long folder_id;

void initialize_cache(const char* folderPath,  struct stat statBuf)
{
    cout << folderPath << '\n';
    
    folder_id++;
    cache[folderPath].id = folder_id;
    paths[folder_id] = folderPath;
    cache[folderPath].modified =statBuf.st_mtime;
    cache[folderPath].stare =  1;
    cache[folderPath].min_folder_size = ULLONG_MAX;
}

dateHash calculateFolderSize(const char* folderPath)
{
    DIR *dir;
    struct dirent *entry;
    struct stat statBuf;
    int fisiere_verif = 0, totalFisiere_verif=0;
    long long totalSize = 0, size = 0;
    unsigned long long min_folder_size = ULLONG_MAX;

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
                if(aux.size > 0)
                    min_folder_size = min(min_folder_size, aux.size);
            }
        }
        cache[folderPath].size = size;
        cache[folderPath].fisiere_verif = fisiere_verif;
        cache[folderPath].min_folder_size = min(min_folder_size,  cache[folderPath].size);
    }
    else
    {
        std::cout << " precalculat - " << folderPath  << "\n";
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

    return cache[folderPath];
}
int isInTask(const char *folderPath)
{
        const char *lastSeparator = strrchr(folderPath, '/');
        
        if (lastSeparator != nullptr)
        {
            // Calculate the length of the parent folder path
            size_t parentPathLength = lastSeparator - folderPath;
            
            char parentFolderPath[PATH_MAX];
            
            // Copy the parent folder path
            strncpy(parentFolderPath, folderPath, parentPathLength);
            
            // Null-terminate the string
            parentFolderPath[parentPathLength] = '\0';
            
            auto it = cache.find(parentFolderPath);
            if(it != cache.end())
                return cache[parentFolderPath].id;
            
            return isInTask(parentFolderPath);
            
        }
        else {
            return 0;
        }
}

void Progress(const char *folderPath)
{
    cout << "dimensiune : " << cache[folderPath].size << "\n verif: " << cache[folderPath].fisiere_verif << " total fisiere : " << cache[folderPath].nr_fisiere  << " total foldere : " << cache[folderPath].nr_foldere << '\n';
    
    //printf (" deja verif: %d , total: %d \n", cache[folderPath].fisiere_verif, cache[folderPath].nr_fisiere);
    
}

void resultAnalysis(const char* folderPath)
{
    DIR *dir;
    struct dirent *entry;
    struct stat statBuf;

    if ((dir = opendir(folderPath)) == NULL)
    {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    if (stat(folderPath, &statBuf) == -1)
        perror("stat");

    char aux[PATH_MAX];
    
    strcpy(aux, folderPath + strlen(cache[folderPath].start_folder)+1);
    
    if(aux[0] != '\0')
    {
        if(folderPath != cache[folderPath].start_folder)
        {
            cout << "|-/" << aux << " ";
            double proc = ((double) cache[folderPath].size) /  cache[cache[folderPath].start_folder].size;
            cout << setprecision(2) << proc * 100  << "% ";
            double nr_mb = ((double) cache[folderPath].size) / 1000000;
            cout << setprecision(2) << nr_mb << " MB ";
            
            int nr_units = (int) (cache[folderPath].size / cache[cache[folderPath].start_folder].min_folder_size);
            for (int i = 1; i<= nr_units; i++)
                cout << "#";
            cout << '\n';
        }
        
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
            
            if (S_ISDIR(statBuf.st_mode))
            {
                char dirPath[PATH_MAX];
                snprintf(dirPath, sizeof(dirPath), "%s/%s", folderPath, entry->d_name);
                
                resultAnalysis(dirPath);
            }
        }
    }
    closedir(dir);
}

void startAnalysis(const char* folderPath)
{
    cout << "\n Path                 Usage       Size         Amount \n";
    cout << folderPath << " 100% ";
    double nr_mb = ((double) cache[folderPath].size) / 1000000;
    cout << setprecision(2) << nr_mb << " MB ";
    
    int nr_units = (int) (cache[folderPath].size / cache[folderPath].min_folder_size);
    cache[folderPath].nr_units = nr_units;
    
    for (int i = 1; i<= nr_units; i++)
        cout<< "#";
    cout << "\n| \n";
    
    
    resultAnalysis(folderPath);
}

void StartTask(const char *folderPath)
{
    int rez = isInTask(folderPath);
    if(rez != 0)
    {
        cout << "\n Este deja in taskul " << rez <<'\n';
        return;
    }
    
    cache[folderPath].start_folder = folderPath;
    
    //aici pe threaduri separate
    countItemsFolder(folderPath);
    calculateFolderSize(folderPath);
    
    cache[folderPath].stare = -1; //terminat
    
    startAnalysis(folderPath);
}

int main()
{
    paths.resize(1000);
    const char *s1 = "/Users/anastasiastefanescu/Documents/uf";
    const char *s2= "/Users/anastasiastefanescu/Documents/SO";
   
    //printf("%lld foldere, %lld fisiere \n", getFolderItemCount(s).first, getFolderItemCount(s).second);
    //printf("\n %lf GB \n", calculateFolderSize(s)/1000000);
    
    //StartTask(s2);
    
    //Progress(s2);
    
   // StartTask(s2);
    
    //Progress(s2);
    
    StartTask(s2);
    
    return 0;
}

//atentie - obligatoriu cand CalculateSize ii dam CountItems, dar pe un thread separat(ca sa nu pierdem timpul) - pt ca vrem sa stim cat mai repede nr de fisiere, nr_foldere si cate fisiere au mai fost verificate in taskuri anterioare.

//In CalculateSize, nu adaugam la nr_verificate daca am gasit un folder deja calculat - adaugam numai fisierele neparcurse pana acum
