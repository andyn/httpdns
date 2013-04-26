#ifndef URL_HH
#define URL_HH

#include <sstream>
#include <stdexcept>
#include <string>

namespace Url {

using std::string;
using std::ostringstream;

struct ParseError : public std::runtime_error {
    ParseError(std::string const &str) : std::runtime_error(str) {}
};

class Parse {
public:
    /// @short Parse an url to its components
    /// The fields are scheme://host[:port][/path]
    Parse(const string &url);

    operator std::string() {
        ostringstream url;
        
        url << scheme() << "://" << host();
        if (port() != "")
            url << ":" << port();
        url << path();
        
        return url.str();
    }

    string const &scheme() const {
        return m_scheme;
    }
    
    string const &host() const {
        return m_host;
    }
    
    string const &port() const {
        return m_port;
    }
    
    string const &path() const {
        return m_path;
    }
    
private:
    string m_scheme;
    string m_host;
    string m_port;
    string m_path;
};

} // namespace Url 

#endif // URL_HH
