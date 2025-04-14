#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

// Helper function: HMAC-SHA256 using OpenSSL.
// Returns a binary string.
std::string hmac_sha256(const std::string &key, const std::string &data) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    HMAC(
        EVP_sha256(),
        reinterpret_cast<const unsigned char*>(key.data()),
        key.size(),
        reinterpret_cast<const unsigned char*>(data.data()),
        data.size(),
        digest,
        &digest_len
    );
    return std::string(reinterpret_cast<char*>(digest), digest_len);
}

// Helper function: Convert a binary string to its hexadecimal representation.
std::string toHex(const std::string &data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char c : data) {
        ss << std::setw(2) << static_cast<int>(c);
    }
    return ss.str();
}

// Helper function: Compute SHA256 hash of input and return as lowercase hex string.
std::string sha256_hex(const std::string &input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), hash);
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::setw(2) << static_cast<int>(hash[i]);
    }
    return ss.str();
}

// AWS Signature V4 signing function using OpenSSL.
// It derives the signing key from the secret key, date, region, and service,
// then computes the final signature for the given string-to-sign.
std::string aws_v4_sign(
    const std::string &secret_key,
    const std::string &date,
    const std::string &region,
    const std::string &service,
    const std::string &string_to_sign)
{
    // Derive signing key:
    // kSecret = "AWS4" + secret_key
    // kDate = HMAC-SHA256(kSecret, date)
    // kRegion = HMAC-SHA256(kDate, region)
    // kService = HMAC-SHA256(kRegion, service)
    // kSigning = HMAC-SHA256(kService, "aws4_request")
    std::string kSecret = "AWS4" + secret_key;
    std::string kDate = hmac_sha256(kSecret, date);
    std::string kRegion = hmac_sha256(kDate, region);
    std::string kService = hmac_sha256(kRegion, service);
    std::string kSigning = hmac_sha256(kService, "aws4_request");

    // Compute the final signature:
    std::string signature = hmac_sha256(kSigning, string_to_sign);
    return toHex(signature);
}

int main() {
    // =========================
    // Parameters for S3 GET request
    // =========================
    // Replace these with your actual credentials and request details.
    std::string secret_key = "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY";
    std::string access_key = "AKIDEXAMPLE";
    std::string region = "us-east-1";
    std::string service = "s3";

    // Dates in two formats are needed:
    // - short date: YYYYMMDD (for credential scope)
    // - long date: YYYYMMDD'T'HHMMSS'Z' (for headers)
    std::string date = "20150830";          // Example: 20150830
    std::string amz_date = "20150830T123600Z";  // Example: 20150830T123600Z

    // S3 request details:
    // For a GET or HEAD request the payload is empty. The payload hash for an empty payload is:
    std::string payload_hash = sha256_hex(""); // yields the SHA256 of an empty string

    // The canonical URI: for example, to get "example-object" from bucket "example-bucket"
    std::string canonicalURI = "/example-object";
    // If you have query parameters, build and URL-encode them in sorted order; here we assume none.
    std::string canonicalQueryString = "";

    // Host header: For S3, the host is typically "<bucket>.s3.amazonaws.com"
    std::string bucket = "example-bucket";
    std::string host = bucket + ".s3.amazonaws.com";

    // Build canonical headers.
    // At a minimum include host, x-amz-content-sha256 and x-amz-date.
    std::stringstream canonicalHeaders;
    canonicalHeaders << "host:" << host << "\n";
    canonicalHeaders << "x-amz-content-sha256:" << payload_hash << "\n";
    canonicalHeaders << "x-amz-date:" << amz_date << "\n";

    // Build the list of signed headers (lowercase, sorted by character code).
    std::string signedHeaders = "host;x-amz-content-sha256;x-amz-date";

    // HTTP method: GET or HEAD.
    std::string httpMethod = "GET";

    // Construct the canonical request.
    // CanonicalRequest =
    //   HTTPMethod + '\n' +
    //   CanonicalURI + '\n' +
    //   CanonicalQueryString + '\n' +
    //   CanonicalHeaders + '\n' +
    //   SignedHeaders + '\n' +
    //   HexEncode(Hash(RequestPayload))
    std::stringstream canonicalRequestStream;
    canonicalRequestStream << httpMethod << "\n"
                           << canonicalURI << "\n"
                           << canonicalQueryString << "\n"
                           << canonicalHeaders.str() << "\n"
                           << signedHeaders << "\n"
                           << payload_hash;
    std::string canonicalRequest = canonicalRequestStream.str();

    // Compute SHA256 hash of the canonical request.
    std::string canonicalRequestHash = sha256_hex(canonicalRequest);

    // =========================
    // Build the String-to-Sign
    // =========================
    // The string-to-sign has the following format:
    // StringToSign =
    //   Algorithm + '\n' +
    //   RequestDate + '\n' +
    //   CredentialScope + '\n' +
    //   HexEncode(Hash(CanonicalRequest))
    std::string algorithm = "AWS4-HMAC-SHA256";
    std::stringstream credentialScopeStream;
    credentialScopeStream << date << "/" << region << "/" << service << "/aws4_request";
    std::string credentialScope = credentialScopeStream.str();

    std::stringstream stringToSignStream;
    stringToSignStream << algorithm << "\n"
                       << amz_date << "\n"
                       << credentialScope << "\n"
                       << canonicalRequestHash;
    std::string stringToSign = stringToSignStream.str();

    // =========================
    // Compute the signature using our helper function.
    // =========================
    std::string signature = aws_v4_sign(secret_key, date, region, service, stringToSign);

    // =========================
    // Build the Authorization header
    // =========================
    // The header is of the form:
    // Authorization: AWS4-HMAC-SHA256 Credential=<access_key>/<credential_scope>, SignedHeaders=<signed_headers>, Signature=<signature>
    std::stringstream authHeaderStream;
    authHeaderStream << algorithm << " "
                     << "Credential=" << access_key << "/" << credentialScope << ", "
                     << "SignedHeaders=" << signedHeaders << ", "
                     << "Signature=" << signature;
    std::string authorizationHeader = authHeaderStream.str();

    // For demonstration, print out all the parts:
    std::cout << "Canonical Request:\n" << canonicalRequest << "\n\n";
    std::cout << "Canonical Request Hash:\n" << canonicalRequestHash << "\n\n";
    std::cout << "String-to-Sign:\n" << stringToSign << "\n\n";
    std::cout << "Authorization Header:\n" << authorizationHeader << "\n";

    // In a real HTTP client you would now include the following headers in your GET or HEAD request:
    //   Host: <bucket>.s3.amazonaws.com
    //   x-amz-content-sha256: <payload_hash>
    //   x-amz-date: <amz_date>
    //   Authorization: <authorizationHeader>
    //
    // And then send the request to:
    //   https://<bucket>.s3.amazonaws.com/<object>

    return 0;
}
