#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <netdb.h>
#include <bits/stdc++.h>

using namespace std;
// Check end of string
inline bool ends_with(std::string const &value, std::string const &ending)
{
    if (ending.size() > value.size())
        return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

// Builds the HTTP GET request for the given contents
string buildRequest(const string &contents)
{
    return "GET " + contents + " HTTP/1.1\r\n\r\n";
}

// Extracts the status code from the HTTP response
string get_statusCode(const string &response)
{
    stringstream ss(response);
    string line;
    getline(ss, line);
    return line.substr(line.find(" ") + 1, 3);
}

// Extracts the content body from the HTTP response
string get_content(const string &response)
{
    size_t emptyLinePos = response.find("\r\n\r\n");
    if (emptyLinePos != string::npos)
    {
        return response.substr(emptyLinePos + 4);
    }
    return "";
}
// Extracts the server IP address from a given URL
const char *get_server_address(string url)
{
    size_t colonPos = url.find(":");
    size_t slashPos = url.find("/");

    if (colonPos == string::npos)
    {
        colonPos = url.size();
    }

    if (slashPos == string::npos)
    {
        slashPos = url.size();
    }

    const char *hostname = url.substr(0, colonPos < slashPos ? colonPos : slashPos).c_str();
    hostent *myHostent = gethostbyname(hostname);
    if (!myHostent)
    {
        cerr << "gethostbyname() failed"
             << "\n";
    }
    else
    {
        char ip[INET6_ADDRSTRLEN];
        for (unsigned int i = 0; myHostent->h_addr_list[i] != NULL; ++i)
        {
            return inet_ntop(myHostent->h_addrtype, myHostent->h_addr_list[i], ip, sizeof(ip));
        }
    }
    return nullptr;
}
// Extracts the file path from a given URL
string get_file_path(string url)
{
    size_t slashPos = url.find("/");
    if (slashPos == string::npos)
    {
        return "/";
    }
    return url.substr(slashPos, url.size() - slashPos);
}
// Extracts the port number from a given URL
int get_port(string url)
{
    size_t colonPos = url.find(":");
    if (colonPos == string::npos)
    {
        return 80;
    }

    size_t slashPos = url.find("/");

    if (slashPos == string::npos)
    {
        slashPos = url.size();
    }

    const char *port = url.substr(colonPos + 1, slashPos - colonPos - 1).c_str();

    return atoi(port);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: ./retriever <url>" << endl;
        return 1;
    }
    // Parse the URL to get the server's address, port, and file path
    const string server_address(argv[1]);
    string hostname(get_server_address(server_address));
    int port = get_port(string(server_address));
    string file_path = get_file_path(server_address);

    // Creates a socket 
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, hostname.c_str(), &(serv_addr.sin_addr));
    // Socket error handling 
    if (sockfd < 0)
    {
        cerr << "Failed to create socket" << endl;
        return -1;
    }
    // Connection to server 
    int connectStatus = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    // Server error handling 
    if (connectStatus < 0)
    {
        cout << "Failed to connect" << endl;
        close(sockfd);
        return -1;
    }
    // Sends HTTP GET request to server 
    string request = buildRequest(file_path);
    if (write(sockfd, request.c_str(), request.size()) < 0)
    {
        cerr << "Error writing to socket" << endl;
        close(sockfd);
        return -1;
    }

    string response;
    char buffer[1500000];
    bzero(buffer, 1500000);
    ssize_t n;
    // Reads server's response 
    while ((n = read(sockfd, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[n] = '\0';
        response += buffer;
        if (ends_with(response, "\r\n\r\n") || ends_with(response, "\n\n"))
        {
            break;
        }
    }
    // Handles reading error
    if (n < 0)
    {
        cerr << "ERROR reading from socket" << endl;
        close(sockfd);
        return -1;
    }
    // Gets status code 
    string statusCode = get_statusCode(response);
    // Save the response content to a file if the status code is 200 OK
    if (statusCode == "200")
    {
        string outputFilename = "output/" + file_path.substr(file_path.find_last_of("/\\") + 1);
        ofstream outFile(outputFilename);
        outFile << get_content(response);
        outFile.close();
    }
    // else handle response accordingly 
    else
    {
        cerr << get_content(response) << endl;
    }
    // Close socket
    close(sockfd);
    return 0;
}
