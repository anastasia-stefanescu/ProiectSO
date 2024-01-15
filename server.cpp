#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h> 

const char* LOCK_FILE = "/var/run/daemon_server.lock";

void handleClient(int clientSocket) {
    const char* welcomeMessage = "Welcome to Daemon Terminal!\n";
    send(clientSocket, welcomeMessage, strlen(welcomeMessage), 0);

    char buffer[1024];
    int bytesRead;

    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        // Process received data (you can modify this part)
        buffer[bytesRead] = '\0';  // Null-terminate the received data
        std::cout << "Received: " << buffer;

        // Send a response back to the client
        const char* response = "Message received by daemon.\n";
        send(clientSocket, response, strlen(response), 0);
    }

    close(clientSocket);
}

bool isServerRunning() {
    // Check if the lock file exists
    int fd = open(LOCK_FILE, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        std::cerr << "Failed to open lock file!" << std::endl;
        exit(EXIT_FAILURE);
    }

    int lockResult = flock(fd, LOCK_EX | LOCK_NB);
    if (lockResult == -1) {
        // Another instance is already running
        close(fd);
        return true;
    }

    // This instance obtained the lock, write the process ID to the lock file
    ftruncate(fd, 0);  // Truncate the file to zero length
    char pidStr[20];
    snprintf(pidStr, sizeof(pidStr), "%ld", (long)getpid());
    write(fd, pidStr, strlen(pidStr));

    return false;
}

int main(int argc, char *argv[]) {
    pid_t pid, sid;
	
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
        const char* message = "Hello from client!\n";
        send(clientSocket, message, strlen(message), 0);
		if (argc > 1)
		{
			const char* data = strcat(argv[1], "\n");
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
    
    // Create a socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create socket!" << std::endl;
        exit(EXIT_FAILURE);
    }
    else std::cout<<"Socket created \n";
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
	else std::cout<<"Socket binded \n";
    // Listen for incoming connections
    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Failed to listen for connections!" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Daemon is listening for connections on port 54000..." << std::endl;

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
		else std::cout<<"Connection accepted! \n";
        // Handle the client in a separate function
        handleClient(clientSocket);
    }

    // Close any open logs (optional)
    close(serverSocket);

    return 0;
}
