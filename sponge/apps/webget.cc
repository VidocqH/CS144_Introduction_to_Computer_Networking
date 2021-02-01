#include "socket.hh"
// My own tcp implementation
#include "tcp_sponge_socket.hh"
#include "util.hh"

#include <cstdlib>
#include <iostream>

using namespace std;

void get_URL(const string &host, const string &path) {
    // Your code here.
    const Address webserver(host, "http");
    // TCPSocket sock;
    CS144TCPSocket sock;
    sock.connect(webserver);
    sock.write("GET " + path + " HTTP/1.1\r\n");
    sock.write("Host: " + host + "\r\n");
    sock.write("Connection: close\r\n");
    sock.write("\r\n");
    auto recvd = sock.read();
    // HTTP/1.1 200 OK
    // Content-type: text/plain
    string res = "";
    for (auto c : recvd) {
        if (c != '\n' && c != '\r')
            res.push_back(c);
    }
    while (recvd.length() != 0) {
        recvd = sock.read();
        if (recvd == "" || recvd == "\n" || recvd == "\r\n" || recvd == "\r")
            continue;
        for (auto c : recvd) {
            if (c != '\n' && c != '\r')
                res.push_back(c);
        }
    }
    cout << res.substr(56) << endl;
    sock.close();

    sock.wait_until_closed();  // Own implementation needed only
    // You will need to connect to the "http" service on
    // the computer whose name is in the "host" string,
    // then request the URL path given in the "path" string.

    // Then you'll need to print out everything the server sends back,
    // (not just one call to read() -- everything) until you reach
    // the "eof" (end of file).

    // cerr << "Function called: get_URL(" << host << ", " << path << ").\n";
    // cerr << "Warning: get_URL() has not been implemented yet.\n";
}

int main(int argc, char *argv[]) {
    try {
        if (argc <= 0) {
            abort();  // For sticklers: don't try to access argv[0] if argc <= 0.
        }

        // The program takes two command-line arguments: the hostname and "path" part of the URL.
        // Print the usage message unless there are these two arguments (plus the program name
        // itself, so arg count = 3 in total).
        if (argc != 3) {
            cerr << "Usage: " << argv[0] << " HOST PATH\n";
            cerr << "\tExample: " << argv[0] << " stanford.edu /class/cs144\n";
            return EXIT_FAILURE;
        }

        // Get the command-line arguments.
        const string host = argv[1];
        const string path = argv[2];

        // Call the student-written function.
        get_URL(host, path);
    } catch (const exception &e) {
        cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
