#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <unordered_map>
#include <string>
#include <fstream>

const char* LOCK_FILE = "/home/andry/Downloads/MyFlow/Server/daemon_server.lock";
char buffer[1024];

void cleanup() {
    // Remove the lock file
    if (remove(LOCK_FILE) != 0) {
        std::cerr << "Failed to remove lock file! " << strerror(errno) << std::endl;
    }
}

void handleClient(int clientSocket) {
    //const char* welcomeMessage = "Welcome to Daemon Terminal!\n";
    //send(clientSocket, welcomeMessage, strlen(welcomeMessage), 0);
	strcpy(buffer, "");
    
    int bytesRead;

    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        // Process received data (you can modify this part)
        buffer[bytesRead] = '\0';  // Null-terminate the received data
        std::cout << "Received: " << buffer;
        
        //function "hello"
		if( memcmp( buffer, "hello", strlen("hello")) == 0)
		{
			std::cout << "Hello function \n";
		}
		
		//function "stop"
		if( memcmp( buffer, "stop", strlen("stop")) == 0)
		{
			std::cout << "Stop command received. Stopping the server..." << std::endl;
            break;
		}
		
        // Send a response back to the client
        const char* response = "Message received by daemon.\n";
        send(clientSocket, response, strlen(response), 0);
    }

    close(clientSocket);
}

bool isServerRunning() {
    std::ifstream file(LOCK_FILE);

    if (!file.is_open()) {
        // The file doesn't exist, create it and write 1
        std::ofstream createFile(LOCK_FILE);
        if (!createFile.is_open()) {
            std::cerr << "Failed to create lock file! " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
        createFile << 1;
        return false;
    }

    int value;
    file >> value;

    if (value == 1) {
        file.close();
        return true;
    }

    file.close();
    return false;
}

int main(int argc, char *argv[]) {
	
	std::string command = "no_arg";
	if(argc == 2)
	{
		command = argv[1];
	}
	std::cout<< "command: "<< command << "\n";
	
    pid_t pid, sid;
    bool server_running = false;
    if (isServerRunning()) {
        server_running = true;
    }

    // Fork off the parent process
    pid = fork();
    if (pid < 0) {
        std::cerr << "Fork failed!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // If we got a good PID, then we can exit the parent process
    if (pid > 0) {
        // Parent process (create a client)
        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0) {
            std::cerr << "Failed to create client socket!" << std::endl;
            exit(EXIT_FAILURE);
        }

        // Set up the client address struct
        struct sockaddr_in clientAddress;
        memset(&clientAddress, 0, sizeof(clientAddress));
        clientAddress.sin_family = AF_INET;
        clientAddress.sin_port = htons(54000);  // Use the same port as the server

        // Connect to the server
        if (connect(clientSocket, (struct sockaddr*)&clientAddress, sizeof(clientAddress)) < 0) {
            std::cerr << "Failed to connect to the server!" << std::endl;
            close(clientSocket);
            exit(EXIT_FAILURE);
        }

        // Send a message to the server
        //const char* message = "Hello from client!\n";
        //send(clientSocket, message, strlen(message), 0);
        if (argc > 1) {
            char* data = strcat(argv[1], "\n");
            send(clientSocket, data, strlen(data), 0);
        }
        // Receive and print the server's response
        char response[1024];
        int bytesRead = recv(clientSocket, response, sizeof(response), 0);
        response[bytesRead] = '\0';  // Null-terminate the received data
        std::cout << "Server Response: " << response;

        // Close the client socket
        close(clientSocket);

        exit(EXIT_SUCCESS);
    }

    // Change the file mode mask
    umask(0);

    // Open any logs here (optional)

    // Create a new SID for the child process
    sid = setsid();
    if (sid < 0) {
        std::cerr << "Failed to create a new session!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Change the current working directory to a safe place
    if ((chdir("/")) < 0) {
        std::cerr << "Failed to change working directory!" << std::endl;
        exit(EXIT_FAILURE);
    }
    /*
    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    */
    std::cout << server_running <<"\n";
    int serverSocket = -1;
    if (server_running == false) {

        // Create a socket
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            std::cerr << "Failed to create socket!" << std::endl;
            exit(EXIT_FAILURE);
        }
        else std::cout << "Socket created \n";
        //std::cout<<"Welcome\n";
        // Set up the server address struct
        struct sockaddr_in serverAddress;
        memset(&serverAddress, 0, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = INADDR_ANY;
        serverAddress.sin_port = htons(54000);  // You can choose any available port

        // Bind the socket to the specified port
        if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
            std::cerr << "Failed to bind socket!" << std::endl;
            exit(EXIT_FAILURE);
        }
        else std::cout << "Socket binded \n";
        // Listen for incoming connections
        if (listen(serverSocket, 5) < 0) {
            std::cerr << "Failed to listen for connections!" << std::endl;
            exit(EXIT_FAILURE);
        }

        std::cout << "Daemon is listening for connections on port 54000..." << std::endl;
	}
	if(serverSocket >= 0){
		//bool running = true;
        // Accept connections and handle clients
        while (true) {
            struct sockaddr_in clientAddress;
            socklen_t clientAddressSize = sizeof(clientAddress);

            // Accept a client connection
            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
            if (clientSocket < 0) {
                std::cerr << "Failed to accept client connection!" << std::endl;
                continue;
            }
            else std::cout << "Connection accepted! \n";
            
            if (strcmp(buffer, "stop") == 0) {
		    	std::cout << "Stop command received. Stopping the server..." << std::endl;
		   		break;
    		}
    		
    		// Handle the client in a separate function
            handleClient(clientSocket);
            
            // Check if "stop" is provided as an argument
            std::cout << argv[1] << "\n";
            std::cout << "second command " << command << "\n";
			if (argc == 2 && strcmp(argv[1], "stop") == 0) {
				// If "stop" is provided, stop the server
				std::cout << "Stopping the server..." << std::endl;
				break;
   			}
        }
        
        std::cout<<"Hello!\n";
		//Cleanup lock file
		cleanup();
		std::cout<<"Clean!\n";
        // Close any open logs (optional)
        close(serverSocket);
        std::cout<<"Server stoped!\n";
    }
    return 0;
}

