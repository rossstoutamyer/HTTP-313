#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <errno.h>
#include <fstream>

using namespace std;

void directHelper(int &sockfd, char* msgbuf, int &sizeBuffer, int &numValues, int &numBytes, int &index, int &index2, string &request, string &header, string &body, ofstream& newFile, string &stringHost, string &stringPath, string &stringFile) {
    request = "GET " + stringPath + " HTTP/1.1" + "\r\n" +"Host: " + stringHost + "\r\n\r\n";

    struct addrinfo c;
    struct addrinfo *p;
    memset(&c, 0, sizeof(c));

    c.ai_family = AF_UNSPEC;
    c.ai_socktype = SOCK_STREAM;
    int status;

    if((status = getaddrinfo((char*) stringHost.c_str(), "80", &c, &p)) != 0) {
        cerr << "getaddrinfo" << endl;
        exit(-1);
    }

     if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
        //perror("Failed socket client");
        close(sockfd);
        exit(-1);
    }
    if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
        perror("Failed connect client");
        close(sockfd);
        exit(-1);
    }

    send(sockfd, (char*) request.c_str(), request.size(), 0);

    while((numBytes = recv(sockfd, msgbuf + numValues, sizeBuffer - numValues, 0)) != 0) {
        numValues += numBytes;
        if (sizeBuffer == numValues) {
            char* newBuffer = new char[sizeBuffer * 2];
            memcpy(newBuffer, msgbuf, sizeBuffer);
            sizeBuffer *= 2;
            delete[] msgbuf;
            msgbuf = newBuffer;
        }
    }
    string stringBuf = string(msgbuf);
    index = stringBuf.find("\r\n\r\n", 0);
    header = stringBuf.substr(0, index + 4);
    body = stringBuf.substr(index + 4, stringBuf.size() - index - 5);
    
    int ind = body.find("\r\n", 0); //remove initial amount
    bool chunking = false;
    if (ind >= 0) {
        chunking = true;
    }
    int ind2 = -1;
    if (ind != -1) {
        body.erase(0, ind + 2);
    }

    while ((ind = body.find("\r\n", 0)) > 0) {
        if (ind == body.size() - 2 || (ind2 = body.find("\r\n", ind + 2)) < 0) {
            break;
        }
        body.erase(ind, ind2 - ind + 2);
    }

    //remove \r\n at the end of file
    if (chunking) {
        body.erase(body.size() - 2, 2);
    }

    delete msgbuf;

    if (header[9] == '2') {
        newFile.open(stringFile, std::ofstream::out);
        newFile << body;
        newFile << "\n";
        newFile.close();
    } else if (header[9] == '3') {
        cout << "Redirecting..." << endl;
       
        index = header.find("location: ");
        index2 = header.find("\n", index);
        stringHost = header.substr(index + 17, header.size() - (index + 22));

        if ((index = stringHost.find("/", 0)) > 1) {
            stringHost.erase(index, stringHost.size() - 1- index);
        }

        msgbuf = new char[256];
        sizeBuffer = sizeof(msgbuf);
        numValues = 0;
        numBytes = 0;
        index = 0;
        index2 = 0;
        string request = "";
        string header = "";
        string body = "";
        ofstream newFile;

        close(sockfd);

        directHelper(sockfd, msgbuf, sizeBuffer, numValues, numBytes, index, index2, request, header, body, newFile, stringHost, stringPath, stringFile);
    } else if (header[9] == '4' || header[9] == '5') {
        cout << "Error response: " << header.substr(9, 3) << endl;
        exit(1);
    }
}

int main(int argc, char** argv) {
    char* urlVal = argv[1];
    char* fileName;
    if (argv[2] != nullptr) {
        fileName = argv[2];
    }

    char* iteration = urlVal;
    int counter = 0;
    while((*iteration) != '/') {
        iteration++;
    }
    iteration++;
    iteration++;

    char* hostStart = iteration;
    char* host;

    while((*iteration) != '/') {
        iteration++;
        counter++;
        if ((*iteration) == '\0') {
            break;
        }
    }

    host = new char[counter];
    for (int i = 0; i < counter; ++i) {
        host[i] = (*hostStart);
        hostStart++;
    }

    counter = 1;
    char* path;
    while((*iteration) != '\0') {
        iteration ++;
        counter++;
    }

    if (counter == 1) {
        path = "/";
    } else {
        path = hostStart;
    }

    string stringPath = string(path);
    string stringHost = string(host);
    int hash;
    if ((hash = stringPath.find("#")) > 0) {
        stringPath = stringPath.substr(0, hash);
    }

    int sockfd;
    char* msgbuf = new char[256];
    int sizeBuffer = sizeof(msgbuf);
    int numValues = 0;
    int numBytes = 0;
    int index = 0;
    int index2 = 0;
    string request = "";
    string header = "";
    string body = "";
    ofstream newFile;
    string stringFile = "";
    if (argv[2] == nullptr) {
        if (stringPath != "/") {
            int currLocation = stringPath.size() - 1;
            while (stringPath[currLocation] != '/') {
                stringFile = stringPath[currLocation] + stringFile;
                currLocation--;
            }
        } else {
            stringFile = "default_" + stringHost + ".html";
        }
    } else {
        stringFile = string(fileName);
    }

    directHelper(sockfd, msgbuf, sizeBuffer, numValues, numBytes, index, index2, request, header, body, newFile, stringHost, stringPath, stringFile);

    delete host;
}