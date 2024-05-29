
// https://stackoverflow.com/questions/2616011/easy-way-to-parse-a-url-in-c-cross-platform

#ifndef URL_HH_
#define URL_HH_
#include <string>

namespace AWSV4 {

struct url_parser {
public:
    // omitted copy, ==, accessors, ...
    explicit url_parser(const std::string &url_s) {
        parse(url_s);
    }

    std::string protocol() const { return protocol_; }

    std::string host() const { return host_; }

    std::string path() const { return path_; }

    std::string query() const { return query_; }

private:
    void parse(const std::string &url_s);

    std::string protocol_, host_, path_, query_;
};

} // namespace AWSV4
#endif /* URL_HH_ */