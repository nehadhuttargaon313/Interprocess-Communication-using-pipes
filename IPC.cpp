#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <vector>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h> 
#include <iostream>
using namespace std;

typedef enum RESPONSE{
    NUM_FILES_RCVD = 1,
    NUM_FILES_ERR = 2,
    FILENAME_RCVD = 3,
    FILENAME_ERR = 4,
    FILESIZE_RCVD = 5,
    FILESIZE_ERR = 6,
    FILE_RCVD = 7,
    FILE_ERR = 8,
    CREATE_ERR = 9,
    FILE_CREATED = 10,
    WRITE_ERR = 11,
    FILE_WRITTEN = 12
} RESPONSE;

void send_files(char* path, int read_d, int write_d, int log_d){
    DIR *d;
    struct dirent *dir;
    d = opendir(path);
    vector<string> files;
    if(d){
        while ((dir = readdir(d)) != NULL){
            string filename = string(dir->d_name);
            
            if(dir->d_type == DT_REG && filename != "." && filename != ".." && filename[0] != '.'){
                //cout << filename << endl;
                files.push_back(filename);
            }
        }
        closedir(d);
    }

    while(true){
        size_t sz = files.size();
        
        write(write_d, &sz, sizeof(sz));
        RESPONSE res;
        read(read_d, &res, sizeof(res));
        if(res == NUM_FILES_RCVD){
            string log =  "Number of files in " + string(path) + " sent\n"; 
            write(log_d, log.c_str(), log.size());
            break;
        }
        else if(res == NUM_FILES_ERR){
            string log =  "ERROR: NUM_FILES_ERR (" + to_string(res) + ")\n";
            write(log_d, log.c_str(), log.size());
        }
    }

    for(auto file : files){
        string filepath = string(path) + "/" + file;
        int fd = open(filepath.c_str(), O_RDONLY);
        
        while(true){
            char* filename = new char[file.size()+1];
            for(int i = 0; i < file.size(); i++){
                filename[i] = file[i];
            }
            filename[file.size()] = '\0';
            write(write_d, file.c_str(), file.size()+1);
            RESPONSE res;
            read(read_d, &res, sizeof(res));
            if(res == FILENAME_RCVD){
                string log = file + " : filename sent\n";
                write(log_d, log.c_str(), log.size()); 
                break;
            }
            else if(res == FILENAME_ERR){
                string log =  "ERROR: FILENAME_ERR (" + to_string(res) + ")\n";
                write(log_d, log.c_str(), log.size());
            }
            delete[] filename;
        }
        
        RESPONSE res;
        read(read_d, &res, sizeof(res));
        if(res == CREATE_ERR){
            string log = "ERROR : CREATE_ERR (" + to_string(res) + "). Exiting\n";
            write(log_d, log.c_str(), log.size());
            exit(EXIT_FAILURE);
        }
        else{
            string log =  file + " : file created on the receiver end(" + to_string(res) + ")\n";
            write(log_d, log.c_str(), log.size());
        }

        struct stat s;
        fstat(fd, &s);
        while(true){
            //cout << "filesize = " << s.st_size << endl;
            write(write_d, &(s.st_size), sizeof(s.st_size));
            RESPONSE res;
            read(read_d, &res, sizeof(res));
            if(res == FILESIZE_RCVD){
                string log = file + " : filesize sent\n";
                write(log_d, log.c_str(), log.size()); 
                break;
            }
            else if(res == FILESIZE_ERR){
                string log =  "ERROR: FILESIZE_ERR (" + to_string(res) + ")\n";
                write(log_d, log.c_str(), log.size());
            }
        }

        char buf;
        string content;

        while(true){
            lseek(fd, 0, SEEK_SET);
            while(read(fd, &buf, 1)){
                content.push_back(buf);
            }
            write(write_d, content.c_str(), content.size());
            RESPONSE res;
            read(read_d, &res, sizeof(res));
            if(res == FILE_RCVD){
                string log = file + " : file sent\n";
                write(log_d, log.c_str(), log.size()); 
                break;
            }
            else if(res == FILE_ERR){
                string log =  "ERROR: FILE_ERR (" + to_string(res) + ")\n";
                write(log_d, log.c_str(), log.size());
            }
        }
       
        read(read_d, &res, sizeof(res));
        if(res == WRITE_ERR){
            string log = "ERROR : WRITE_ERR (" + to_string(res) + "). Exiting\n";
            write(log_d, log.c_str(), log.size());
            exit(EXIT_FAILURE);
        }
        else{
            string log =  "file writen on the receiver end(" + to_string(res) + ")\n";
            write(log_d, log.c_str(), log.size());
        }
        close(fd);
    }    
}

void rcv_files(char* path, int read_d, int write_d, int log_d){
    size_t n = 0;
    while(true){
        if(read(read_d, &n, sizeof(n))!=-1){
            RESPONSE res = NUM_FILES_RCVD;
            string log = "Number of files in the dir received (" + to_string(res) + ")\n";
            write(log_d, log.c_str(), log.size());
            write(write_d, &res, sizeof(res));
            break;
        }
        else{
            RESPONSE res = NUM_FILES_ERR;
            string log = "ERROR: NUM_FILES_ERR(" + to_string(res) + ")\n";
            write(log_d, log.c_str(), log.size());
            write(write_d, &res, sizeof(res));
        }
    }
    for(int i = 0; i < n; i++){
        char* filename = new char[PATH_MAX];
        while(true){
            int readsize = 0;
            if((readsize = read(read_d, filename, PATH_MAX))!=-1){
                //cout << "readsize " << readsize << endl;
                //printf("fileee = %s\n", filename);
                RESPONSE res = FILENAME_RCVD;
                string log = string(filename) + " : filename received (" + to_string(res) + ")\n";
                write(log_d, log.c_str(), log.size());
                write(write_d, &res, sizeof(res));
                break;
            }
            else{
                RESPONSE res = FILENAME_ERR;
                string log = "ERROR: FILENAME_ERR(" + to_string(res) + ")\n";
                write(log_d, log.c_str(), log.size());
                write(write_d, &res, sizeof(res));
            }
        }

        string filepath(path);
        filepath = filepath + "/"+ filename;
        //cout << "filename received = " << filepath << endl;
        int fd = open(filepath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if(fd == -1){
            RESPONSE res = CREATE_ERR;
            string log = "ERROR: CREATE_ERR(" + to_string(res) + ")\n";
            write(log_d, log.c_str(), log.size());
            write(write_d, &res, sizeof(res));
            exit(EXIT_FAILURE);
        }
        else{
            RESPONSE res = FILE_CREATED;
            string log = string(filename) + " : file created(" + to_string(res) + ")\n";
            write(log_d, log.c_str(), log.size());
            write(write_d, &res, sizeof(res));
        }

        off_t filesize;
        while(true){
            if(read(read_d, &filesize, sizeof(filesize))!=-1){
                RESPONSE res = FILESIZE_RCVD;
                string log = string(filename) + " : filesize received (" + to_string(res) + ")\n";
                //cout << "file size = " << filesize << endl;
                write(log_d, log.c_str(), log.size());
                write(write_d, &res, sizeof(res));
                break;
            }
            else{
                RESPONSE res = FILESIZE_ERR;
                string log = "ERROR: FILESIZE_ERR(" + to_string(res) + ")\n";
                write(log_d, log.c_str(), log.size());
                write(write_d, &res, sizeof(res));
            }
        }

        char* content = new char[filesize+1];
        while(true){
            if(read(read_d, content, filesize)==filesize){
                RESPONSE res = FILE_RCVD;
                string log = string(filename) + " : file received (" + to_string(res) + ")\n";
                write(log_d, log.c_str(), log.size());
                write(write_d, &res, sizeof(res));
                //printf("content = %s\n", content);
                break;
            }
            else{
                RESPONSE res = FILE_ERR;
                string log = "ERROR: FILE_ERR(" + to_string(res) + ")\n";
                write(log_d, log.c_str(), log.size());
                write(write_d, &res, sizeof(res));
            }
        }

        int writeret = write(fd, content, strlen(content));
        if(writeret != strlen(content)){
            RESPONSE res = WRITE_ERR;
            string log = "ERROR: WRITE_ERR(" + to_string(res) + ")\n";
            write(log_d, log.c_str(), log.size());
            write(write_d, &res, sizeof(res));
            exit(EXIT_FAILURE);
        }
        else{
            RESPONSE res = FILE_WRITTEN;
            string log = string(filename) + " : file written(" + to_string(res) + ")\n";
            write(log_d, log.c_str(), log.size());
            write(write_d, &res, sizeof(res));
        }
        delete[] content;
        delete[] filename;
        close(fd);
    }
}

int main(int argc, char* argv[]){
    if(argc < 5){
        fprintf(stderr, "provide paths to directories and the log files\n");
        exit(EXIT_FAILURE);
    }

    int pipe1to2[2];
    int pipe2to1[2];

    if(pipe(pipe1to2) == -1){
        fprintf(stderr, "error creating pipe\n");
        exit(EXIT_FAILURE);
    }

    if(pipe(pipe2to1) == -1){
        fprintf(stderr, "error creating pipe\n");
        exit(EXIT_FAILURE);
    }


    //cout << "forking\n";    

    int forkret = fork();

    if(forkret == 0){
        //cout << "Child1\n";
        if(close(pipe1to2[0])==-1){
            fprintf(stderr, "closing read end of first pipe failed\n");
            exit(EXIT_FAILURE);
        }

        if(close(pipe2to1[1])==-1){
            fprintf(stderr, "closing write end of second pipe failed\n");
            exit(EXIT_FAILURE);
        }

        int logfd = open(argv[3], O_CREAT | O_WRONLY, 0666);
        // << "log 1 " << logfd << endl;
        string log1 = "sending files to child 2..\n\n";
        write(logfd, log1.c_str(), log1.size());
        send_files(argv[1], pipe2to1[0], pipe1to2[1], logfd);

        string log2 = "\n\nsending files to child 2..\n\n";
        write(logfd, log2.c_str(), log2.size());
        rcv_files(argv[1], pipe2to1[0], pipe1to2[1], logfd);
        exit(EXIT_SUCCESS);
    }

    forkret = fork();

    if(forkret==0){
        //cout << "Child2\n";
        if(close(pipe2to1[0])==-1){
            fprintf(stderr, "closing read end of second pipe failed\n");
            exit(EXIT_FAILURE);
        }

        if(close(pipe1to2[1])==-1){
            fprintf(stderr, "closing write end of first pipe failed\n");
            exit(EXIT_FAILURE);
        }

        int logfd = open(argv[4], O_CREAT | O_WRONLY | O_TRUNC, 0666);

        string log2 = "receiving files from child 21..\n\n";
        write(logfd, log2.c_str(), log2.size());
        rcv_files(argv[2], pipe1to2[0], pipe2to1[1], logfd);
        string log1 = "\n\nsending files to child 1..\n\n";
        write(logfd, log1.c_str(), log1.size());
        send_files(argv[2], pipe1to2[0], pipe2to1[1], logfd);
        exit(EXIT_SUCCESS);
    }
}