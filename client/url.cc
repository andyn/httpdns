#include <cstring>

#include "url.hh"

Url::Parse::Parse(const std::string &url) {
    // Offsets used while parsing different components
    std::string::size_type begin;
    std::string::size_type end;

    // Find the scheme:// part
    begin = 0;
    end = url.find("://");
    if (end == std::string::npos)
        throw ParseError(url + " does not specify a scheme");
    else
        m_scheme = std::string(url, begin, end - begin);
    begin += end + strlen("://");

    // Find the host:port part end (npos or position of /)
    end = url.find('/', begin);

    // Further split the host and port part
    // std::cout << "->" << std::string(str, begin, end - begin) << std::endl;
    std::string::size_type hostportdelim = url.rfind(':', end);
    if (hostportdelim <= begin) {
        // Port was not specified, return "www"
        m_host = std::string(url, begin, end - begin);
        if (m_host == "")
            throw ParseError(url + " does not specify a hostname");
        m_port = "";
    }
    else if (hostportdelim > begin) {
        m_host = std::string(url, begin, hostportdelim - begin);
        m_port = std::string(url, hostportdelim + 1, end - hostportdelim - 1);
        if (m_port == "")
            throw std::runtime_error(url + " specifies an empty port");
    }

    // Everything thereafter including the / is the path part.
    // If a slash is not found, assume it is /
    if (end == std::string::npos)
        m_path = "/";
    else
        m_path = std::string(url, end);

}
