//
// Created by James Gallagher on 2/28/25.
//

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <fstream>
#include <iostream>

bool s3_head(const Aws::S3::S3Client &s3_client) {
    Aws::S3::Model::HeadObjectRequest head_request;
    head_request.SetBucket("cloudydap");
    head_request.SetKey("/samples/chunked_twoD.h5");

    auto const head_outcome = s3_client.HeadObject(head_request);
    if (head_outcome.IsSuccess()) {
        std::cout << "Object exists in S3." << std::endl;
        return true;
    }

    std::cerr << "Error: "
            << head_outcome.GetError().GetExceptionName() << " - "
            << head_outcome.GetError().GetMessage() << std::endl;
    return false;
}

bool s3_get(const Aws::S3::S3Client &s3_client) {
    Aws::S3::Model::GetObjectRequest object_request;
    object_request.SetBucket("cloudydap");
    object_request.SetKey("/samples/chunked_twoD.h5");

    auto get_object_outcome = s3_client.GetObject(object_request);
    if (get_object_outcome.IsSuccess()) {
        auto const &retrieved_file = get_object_outcome.GetResultWithOwnership().GetBody();
        std::ofstream output_file("output_file", std::ios::binary);
        output_file << retrieved_file.rdbuf();
        output_file.close();
        std::cout << "Object retrieved and saved successfully." << std::endl;
        return true;
    }

    std::cerr << "Error: "
            << get_object_outcome.GetError().GetExceptionName() << " - "
            << get_object_outcome.GetError().GetMessage() << std::endl;
    return false;
}

int main() {
    const Aws::SDKOptions options;
    bool status;
    Aws::InitAPI(options); {
        Aws::S3::S3ClientConfiguration clientConfig;
        clientConfig.region = "us-east-1"; // Set your region

        // Create a shared pointer to a SimpleAWSCredentialsProvider using your key and secret.
        auto credentialsProvider = Aws::Auth::AWSCredentials(getenv("CMAC_ID"), getenv("CMAC_ACCESS_KEY"));

        // Construct the S3 client with the credentials provider and client configuration.
        // std::shared_ptr<S3EndpointProviderBase> is nullptr in the following call.
        const Aws::S3::S3Client s3_client(credentialsProvider, nullptr, clientConfig);

        status = s3_head(s3_client);
        std::cerr << "S3 HEAD status: " << (status ? "true" : "false") << "\n";

        status = s3_get(s3_client);
        std::cerr << "S3 GET status: " << (status ? "true" : "false") << "\n";
    }
    Aws::ShutdownAPI(options);
    return status ? 0: 1;
}
