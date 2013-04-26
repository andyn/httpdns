/**
 * The client component for the HttpDns exercise assignment
 * in S-38.3610 Network Programming spring 2012
 *
 * @author Antti Nilakari 66648T <antti.nilakari@aalto.fi>
 */

// We do want ISO C++ pedantic-ness but with certain POSIX & GNU extensions
// Seemed to be enabled by default in GCC despite of -std=c++98.
// Wasn't enabled when -std=c99 was specified...
// #define _GNU_SOURCE 

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "httpgetput.hh"
#include "socket.hh"

// Put the command line parameters into a bit more C++ish format
std::vector<std::string> argv_to_vector(char ** argv) {
    std::vector<std::string> params;
    while (*argv) {
        params.push_back(std::string(*argv));
        ++argv;
    }
    return params;
}

int main(int argc, char * argv[]) {
    (void) argc; // Not needed
    
    // Vectorize command line params
    std::vector<std::string> params = argv_to_vector(argv);
    
    // Check for right number of params
    if (params.size() != 4) {
        std::cout << "Usage: " << params.at(0) << " ACTION SOURCE TARGET\n"
                  << "  ACTION              GET or PUT\n"
                  << "  SOURCE, TARGET      file name or a URL\n"
                  << std::endl;
        return EXIT_SUCCESS;
    }
       
    return http::http_copy(params.at(1), params.at(2), params.at(3));
}
