#ifndef ADDRINFO_HH
#define ADDRINFO_HH

#include <cstring>
#include <errno.h>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/**
 * @short Exception type for class Addrinfo
 */
struct NoAddressException : public std::runtime_error {
    NoAddressException(std::string const &str) : runtime_error(str) {}
};

/**
 * @short A wrapper for getaddrinfo / freeaddrinfo.
 */
class Addrinfo {
public:
    Addrinfo(std::string const &host,
             std::string const &service,
             int socktype = 0,
             int flags = AI_V4MAPPED | AI_ALL,
             int family = AF_INET6
             ) {

        // Temporary hints for getaddrinfo
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = family;
        hints.ai_socktype = socktype;
        hints.ai_flags = flags;
            
        int success = getaddrinfo(host.c_str(),
                                  service.c_str(),
                                  &hints,
                                  &m_ai);
        
        if (success == EAI_NONAME)
            throw NoAddressException("Could not resolve host " + host);
        else if (success == EAI_SERVICE)
            throw NoAddressException("Service or port " + service + " is unknown");
        else if (success != 0)
            throw std::runtime_error(gai_strerror(success));
    }
    
    virtual ~Addrinfo() {
        freeaddrinfo(m_ai);
    }
    
    struct addrinfo *ai() {
        return m_ai;
    }
    
    struct addrinfo const *ai() const {
        return m_ai;
    }
    
private:
    struct addrinfo *m_ai;
};

#endif // ADDRINFO_HH

