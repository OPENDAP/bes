//
// Created by James Gallagher on 3/4/25.
//

#include <aws/core/Aws.h>

#include "AWS_SDK.h"

using namespace std;

int main() {
    const Aws::SDKOptions options;
    bool status;
    Aws::InitAPI(options); {
        AWS_SDK aws_sdk;
        aws_sdk.init();

        status = aws_sdk.s3_head("cloudydap", "/samples/chunked_twoD.h5");
        std::cerr << "S3 HEAD status: " << (status ? "true" : "false") << "\n";

        const string response = aws_sdk.s3_get_as_string("cloudydap", "/samples/chunked_twoD.h5");
        std::cerr << "S3 GET status: " << (!response.empty() ? "true" : "false") << "\n";
    }
    Aws::ShutdownAPI(options);
    return status ? 0 : 1;
}
