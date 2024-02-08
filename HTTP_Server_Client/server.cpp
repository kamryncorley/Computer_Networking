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

using namespace std;
// Check end of string
inline bool ends_with(std::string const &value, std::string const &ending)
{
    if (ending.size() > value.size())
        return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}
// Converts file path to a simplified version
string simplifyPath(string path)
{
    // using vector in place of stack
    vector<string> v;
    int n = path.length();
    string ans;
    for (int i = 0; i < n; i++)
    {
        string dir = "";
        // forming the current directory
        while (i < n && path[i] != '/')
        {
            dir += path[i];
            i++;
        }

        if (dir == "..")
        {
            if (!v.empty())
                v.pop_back();
        }
        else if (dir == "." || dir == "")
        {
            // nothing but for better understanding
        }
        else
        {
            // push the current directory into the vector
            v.push_back(dir);
        }
    }
    // Create simplified path
    for (auto i : v)
    {
        ans += "/" + i;
    }

    return ans == "" ? "/" : ans;
}
// Extracts the file path from the HTTP request
inline string get_filepath(string const &request)
{
    string newLineDelimiter = "\r\n";
    string firstLine = request.substr(0, request.find(newLineDelimiter));

    string spaceDelimiter = " ";
    int firstSpace = firstLine.find(spaceDelimiter);
    int secondSpace = firstLine.find(spaceDelimiter, firstSpace + 1);
    return firstLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);
}
// Reads file contents from the specified file path
inline string read_file(string const &filepath)
{
    string cwd = get_current_dir_name();
    ifstream MyFile(simplifyPath(cwd + filepath));
    stringstream overallResponse;
    overallResponse << MyFile.rdbuf();
    MyFile.close();
    return overallResponse.str();
}
// Checks if the file exists
bool file_exists(string const &filepath)
{
    string cwd = get_current_dir_name();
    ifstream myFile(simplifyPath(cwd + filepath));
    // Check if the file exists.
    return !myFile.fail();
}
// checks if the file path can be accessed
bool file_allowed(string const &filepath)
{
    string cwd = get_current_dir_name();
    string cwdWithSlash = cwd + "/";
    string simplePath = simplifyPath(cwd + filepath);
    return simplePath.find(cwdWithSlash) == 0;
}
// Checks if the file is authorized
bool file_authorized(string const &filepath)
{
    string cwd = get_current_dir_name();
    string simplePath = simplifyPath(cwd + filepath);
    string unauthorizedFileName = cwd + "/MySecret.html";
    return unauthorizedFileName.compare(simplePath) != 0;
}
// Creates an HTTP 200 OK response
string build_ok_response(string const &contents)
{
    std::ostringstream ss;
    ss << "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " << contents.size() << "\r\n\r\n"
       << contents;
    return ss.str();
}
// Creates an HTTP 403 Forbidden response
string build_forbidden_response(string const &filepath)
{
    std::ostringstream contentStream;
    contentStream << "Path <b>" << filepath << "</b> forbidden!";
    string contents = contentStream.str();
    std::ostringstream ss;
    ss << "HTTP/1.1 403 Forbidden\r\nContent-Type: text/html\r\nContent-Length: " << contents.size() << "\r\n\r\n"
       << contents;
    return ss.str();
}
// Creates an HTTP 401 Unauthorized response
string build_unauthorized_response(string const &filePath)
{
    std::ostringstream contentStream;
    contentStream << "Path <b>" << filePath << "</b> not authorized!";
    string contents = contentStream.str();
    std::ostringstream ss;
    ss << "HTTP/1.1 401 Unauthorized\r\nContent-Type: text/html\r\nContent-Length: " << contents.size() << "\r\n\r\n"
       << contents;
    return ss.str();
}
// Creates an HTTP 404 Not Found response
string build_not_found_response(string const &filePath)
{
    std::ostringstream contentStream;
    contentStream << "Path <b>" << filePath << "</b> not found!";
    string contents = contentStream.str();
    std::ostringstream ss;
    ss << "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: " << contents.size() << "\r\n\r\n"
       << contents;
    return ss.str();
}
// Creates an HTTP 400 Bad Request response
string build_bad_request_response()
{
    string contents = "400 Bad Request: The server could not understand the request. ";
    std::ostringstream ss;
    ss << "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: " << contents.size() << "\r\n\r\n"
       << contents;
    return ss.str();
}

int main()
{
    // Creates server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in acceptSock;
    int addrlen = sizeof(acceptSock);
    acceptSock.sin_family = AF_INET;
    acceptSock.sin_addr.s_addr = INADDR_ANY;
    acceptSock.sin_port = htons(8888);

    const int on = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(int));

    bind(server_fd, (struct sockaddr *)&acceptSock, sizeof(acceptSock));
    listen(server_fd, 3);
    cout << "listening..." << endl;

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        // Accepts client connection
        int newSd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        char buf[150000];
        // Reads from the client request
        int nRead = read(newSd, buf, sizeof(buf));

        if (nRead == 0)
        {
            close(newSd);
            continue;
        }
        // Parse client's request
        string request(buf, nRead);
        // Checks request format
        if (request.substr(0, 3) != "GET" || request.find("HTTP/1.1") == string::npos)
        {
            string response = build_bad_request_response();
            send(newSd, response.c_str(), response.length(), 0);
            close(newSd);
            continue;
        }
        // Extracts requested file path from the client's request
        string requestedPath = get_filepath(request);
        // If Path is invalid, respond with bad request
        if (requestedPath.size() == 0)
        {
            string response = build_bad_request_response();
            send(newSd, response.c_str(), response.length(), 0);
            close(newSd);
            continue;
        }

        // cout << "requested file is '" << requestedPath << "'" << endl;
        string response;

        // gives response based on file path
        if (!file_allowed(requestedPath))
        {
            response = build_forbidden_response(requestedPath);
        }
        else if (!file_authorized(requestedPath))
        {
            response = build_unauthorized_response(requestedPath);
        }
        else if (!file_exists(requestedPath))
        {
            response = build_not_found_response(requestedPath);
        }
        else
        {
            response = build_ok_response(read_file(requestedPath));
        }
        // Send HTTP response to client
        send(newSd, response.c_str(), response.length(), 0);
        close(newSd);
    }

    return 0;
}