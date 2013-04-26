#ifndef NPBGETPUT_HH
#define NPBGETPUT_HH
/**
 * httpgetput.hh
 * HTTP GET and PUT functionality
 * @author Antti Nilakari 66648T <antti.nilakari@aalto.fi>
 */

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "addrinfo.hh"
#include "file.hh"
#include "filedescriptor.hh"
#include "tcpclientsocket.hh"
#include "url.hh"

using std::string;
using std::vector;

namespace http {

int http_copy(string const &action,
              string const &source,
              string const &target) {
    try {
        if (action == "GET") {
            Url::Parse source_url(source);
            Addrinfo source_addrs(source_url.host(),
                                  source_url.port(),
                                  SOCK_STREAM);
            TcpClientSocket source_socket(source_addrs);
            File target_file(target, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            
            string header_out = "GET " + source_url.path() + " HTTP/1.1\r\n" +
                                "Connection: close\r\n" + 
                                "Iam: anilakar\r\n\r\n";
            source_socket.write(header_out);
            
            vector<char> buffer(512); // for reading header
            
            // Run only once anyway, there's a break in the end
            while(true) {
            
                // Read the header, the only field that interests us now
                // is the content-length field
                std::string header_in;
                size_t content_len = 0;
                while (true) {
                    size_t bytes_read = source_socket.read(buffer);
                    // Read bytes until the content-length header is found and parsed
                    if (bytes_read > 0) {
                        header_in.append(&buffer[0], buffer.size());
                        
                        size_t content_len_begin = header_in.find("Content-Length:");
                        if (content_len_begin != string::npos) {
                            content_len_begin = header_in.find(':', content_len_begin) + 1;
                            size_t content_len_end = header_in.find_first_of("\r\n", content_len_begin);
                            if (content_len_end == string::npos)
                                continue; // Read more!
                            std::string content_len_str = std::string(header_in,
                                                                      content_len_begin,
                                                                      content_len_end - content_len_begin);
                            content_len = atoll(content_len_str.c_str());
                            break; // Have the content-length, next find the content beginning
                        }
                        else {
                            continue; // Read more!
                        }
                    }
                    else {
                        throw std::runtime_error("Error reading HTTP header from host");
                    }
                }
                
                // Finished reading header, find payload (content) beginning
                while (true) {
                    size_t payload_begin = header_in.find("\r\n\r\n");
                    if (payload_begin == string::npos) {
                        size_t bytes_read = source_socket.read(buffer);
                        if (bytes_read <= 0)
                            throw std::runtime_error("Error reading HTTP header from host");
                        header_in.append(&buffer[0], buffer.size());
                        continue; // read more!
                    }
                    payload_begin += ::strlen("\r\n\r\n");
                    
                    // Copy the initial data after the header into the buffer
                    vector<char> content_buffer((buffer.begin() + payload_begin), buffer.end());
                    
                    // Write the initial content/payload into the file.
                    // and keep track of how much has been read.
                    content_len -= content_buffer.size();
                    target_file.write(content_buffer);
                    break;
                }
                
                vector<char> content_buffer(512);
                // Alternate reading and writing until no more content is expected
                while (content_len > 0) {
                    // Avoid hanging on keepalive. The last read will be shorter.
                    if (content_len < content_buffer.size())
                        content_buffer.resize(content_len);
                    std::cout << "attempting to read " << content_buffer.size() << std::endl;
                    ssize_t bytes_read = source_socket.read(content_buffer);
                    if (bytes_read > 0) {
                        content_len -= bytes_read;
                        target_file.write(content_buffer);
                    }
                    else {
                        throw std::runtime_error("Error reading data from socket");
                    }   
                }           
            break; // Run the while loop only once.
            }
            
            
        }
        
        else if (action == "PUT") {
            Url::Parse target_url(target);
            Addrinfo target_addrs(target_url.host(),
                                  target_url.port(),
                                  SOCK_STREAM);
            TcpClientSocket target_socket(target_addrs);
            File source_file(source, O_RDONLY);
            
            // Get the input file size
            size_t file_size = lseek(source_file, 0, SEEK_END);
            lseek(source_file, 0, SEEK_SET);
            std::stringstream file_size_str;
            file_size_str << file_size;
            
            string header_out = "PUT " + target_url.path() + " HTTP/1.1\r\n" +
                                "Connection: close\r\n" +
                                "Content-Type: text/plain\r\n" +
                                "Content-Length: " + file_size_str.str() + "\r\n" +
                                "Iam: anilakar\r\n\r\n";
            target_socket.write(header_out);
            
            vector<char> buffer(512);
            // Alternate reading and writing until no more content is expected
            while (true) {
                size_t read_from_file = source_file.read(buffer);
                if (read_from_file > 0) {
                    buffer.resize(read_from_file);
                    size_t written = target_socket.write(buffer);
                    if (written <= 0)
                        throw std::runtime_error("Error writing into socket");
                }
                else if (read_from_file == 0) {
                    break;
                }
                else { // -1
                    throw std::runtime_error("Error writing into socket");
                }
            }                  
        }
        
        // Only GET and PUT are supported
        else {
            std::cerr << "Unknown action " << action << std::endl;
            return EXIT_FAILURE;
        }
    }
    catch (Url::ParseError e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (std::runtime_error e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return 0;
}

} // namespace npb

#endif // NPBGETPUT_HH

