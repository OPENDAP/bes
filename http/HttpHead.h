
#include <string>

namespace curl {

bool http_head(const std::string &target_url, int tries, unsigned long wait_time_us);

}
