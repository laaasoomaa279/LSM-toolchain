#ifndef LSM_TOOLS_JSON_RPC_HPP
#define LSM_TOOLS_JSON_RPC_HPP

#include <string>
#include <unordered_map>
#include <iostream>
#include <sstream>

struct JSONRPCRequest {
    std::string jsonrpc = "2.0";
    std::string id;
    std::string method;
    std::unordered_map<std::string, std::string> params;
};

class JSONRPC {
public:
    static std::string makeResponse(const std::string& id, const std::string& resultJson) {
        std::ostringstream body;
        body << "{\"jsonrpc\":\"2.0\",\"id\":" << id << ",\"result\":" << resultJson << "}";
        
        std::string bodyStr = body.str();
        std::ostringstream response;
        response << "Content-Length: " << bodyStr.size() << "\r\n\r\n" << bodyStr;
        return response.str();
    }

    static std::string makeNotification(const std::string& method, const std::string& paramsJson) {
        std::ostringstream body;
        body << "{\"jsonrpc\":\"2.0\",\"method\":\"" << method << "\",\"params\":" << paramsJson << "}";
        
        std::string bodyStr = body.str();
        std::ostringstream response;
        response << "Content-Length: " << bodyStr.size() << "\r\n\r\n" << bodyStr;
        return response.str();
    }

    static bool readRequest(std::istream& input, std::string& rawBody) {
        std::string line;
        size_t contentLength = 0;

        while (std::getline(input, line) && line != "\r" && !line.empty()) {
            if (line.rfind("Content-Length: ", 0) == 0) {
                contentLength = std::stoull(line.substr(16));
            }
        }

        if (contentLength == 0) return false;

        rawBody.resize(contentLength);
        input.read(&rawBody[0], contentLength);
        return true;
    }
};

#endif 