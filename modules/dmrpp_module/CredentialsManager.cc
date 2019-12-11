//
// Created by ndp on 12/11/19.
//

#include <curl/multi.h>
#include <curl/curl.h>
#include <ctime>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <locale>
#include <string>

#include "DmrppCommon.h"
#include "WhiteList.h"
#include <TheBESKeys.h>
#include "BESForbiddenError.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "BESLog.h"
#include "util.h"   // long_to_string()
#include "config.h"
#include "Chunk.h"
#include "CurlHandlePool.h"
#include "awsv4.h"

#include "CredentialsManager.h"

#define MODULE "dmrpp:creds"


std::string get_env_value(const string &key){
    string value;
    const char *cstr = getenv(key.c_str());
    if(cstr){
        value.assign(cstr);
        BESDEBUG(MODULE, __FILE__ << " " << __LINE__ << " From system environment - " << key << ": " << value << endl);
    }
    else {
        value.clear();
    }
    return value;
}

std::string get_config_value(const string &key){
    string value;
    bool key_found=false;
    TheBESKeys::TheKeys()->get_value(key, value, key_found);
    if (key_found) {
        BESDEBUG(MODULE, __FILE__ << " " << __LINE__ << " Using " << key << " from TheBESKeys" << endl);
    }
    else {
        value.clear();
    }
    return value;
}


struct access_credentials{
    std::string id;    // = "AKIA24JBYMSH64NYGEIE";
    std::string key;    // = "*************WaaQ7";
    std::map<std::string, std::string> kvp;

    access_credentials(): id(""), key(""){}
    access_credentials(const std::string &vid, const std::string &vkey): id(vid), key(vkey){
        add("id",id);
        add("key",key);
    }
    access_credentials(const access_credentials &ac):id(ac.id),key(ac.key),kvp(ac.kvp){}

    std::string get_id(){return get("id");}
    std::string get_key(){return get("key");}

    std::string get(const std::string &vkey){
        std::map<std::string, std::string>::iterator it;
        std::string value("");
        it = kvp.find(vkey);
        if (it != kvp.end())
          value = it->second;
        return value;
    }

    void add(const std::string &key, const std::string &value){
        kvp.insert(std::pair<std::string, std::string>(key, value));
    }
};

struct s3_access_credentials: public access_credentials {
    std::string region;    // = "us-east-1";
    std::string bucket_name;    // = "muhbucket";

    s3_access_credentials(): access_credentials(), region(""), bucket_name(""){}
    s3_access_credentials(const std::string &vid, const std::string &vkey, const std::string &vregion, const std::string &vbucket)
            : access_credentials(vid, vkey), region(vregion), bucket_name(vbucket) {
        add("region",region);
        add("bucket_name",bucket_name);
    }
    s3_access_credentials(const s3_access_credentials &sac): access_credentials(sac), region(), bucket_name(sac.bucket_name){}

    std::string get_region(){ return get("region");}
    std::string get_bucket_name(){ return get("bucket_name");}
};

class CrdMngr {
private:
    CrdMngr();
    static CrdMngr *theMngr;
    std::map<std::string, access_credentials* > creds;

public:
    ~CrdMngr();

    static CrdMngr *theCM(){
        if(!theMngr){
            theMngr= new CrdMngr();
        }
        return theMngr;
    }

    void add(const std::string &url, access_credentials *ac);

    access_credentials* get(const std::string &url);

    void load_credentials();

};
CrdMngr::~CrdMngr() {
    for (std::map<std::string,access_credentials*>::iterator it=creds.begin(); it != creds.end(); ++it){
        delete it->second;
    }
    creds.clear();
}

CrdMngr::CrdMngr(){
    load_credentials();
}


void
CrdMngr::add(const std::string &key, access_credentials *ac){
    creds.insert(std::pair<std::string,access_credentials *>(key, ac));
}

access_credentials*
CrdMngr::get(const std::string &url){
    access_credentials *best_match = NULL;
    std::string best_key("");

    for (std::map<std::string,access_credentials*>::iterator it=creds.begin(); it != creds.end(); ++it){
        std::string key = it->first;
        if (url.rfind(key, 0) == 0) {
            // url starts with key
            if(key.length() > best_key.length()){
                best_key = key;
                best_match = it->second;
            }
        }
    }
    return best_match;
}

void CrdMngr::load_credentials( ){
    string aws_akid, aws_sak, aws_region, aws_s3_bucket;

    const string KEYS_CONFIG_PREFIX("DMRPP");

    const string ENV_AKID_KEY("AWS_ACCESS_KEY_ID");
    const string CONFIG_AKID_KEY(KEYS_CONFIG_PREFIX+"."+ENV_AKID_KEY);

    const string ENV_SAK_KEY("AWS_SECRET_ACCESS_KEY");
    const string CONFIG_SAK_KEY(KEYS_CONFIG_PREFIX+"."+ENV_SAK_KEY);

    const string ENV_REGION_KEY("AWS_REGION");
    const string CONFIG_REGION_KEY(KEYS_CONFIG_PREFIX+"."+ENV_REGION_KEY);

    const string ENV_S3_BUCKET_KEY("AWS_S3_BUCKET");
    const string CONFIG_S3_BUCKET_KEY(KEYS_CONFIG_PREFIX+"."+ENV_S3_BUCKET_KEY);

#ifndef NDEBUG

    // If we are in developer mode then we compile this section which
    // allows us to inject credentials via the system environment

    aws_akid.assign(     get_env_value(ENV_AKID_KEY));
    aws_sak.assign(      get_env_value(ENV_SAK_KEY));
    aws_region.assign(   get_env_value(ENV_REGION_KEY));
    aws_s3_bucket.assign(get_env_value(ENV_S3_BUCKET_KEY));

    BESDEBUG(MODULE, __FILE__ << " " << __LINE__
                              << " From ENV aws_akid: '" << aws_akid << "' "
                              << "aws_sak: '" << aws_sak << "' "
                              << "aws_region: '" << aws_region << "' "
                              << "aws_s3_bucket: '" << aws_s3_bucket << "' "
                              << endl);

#endif

    // In production mode this is the single point of ingest for credentials.
    // Developer mode enables the piece above which allows the environment to
    // overrule the configuration

    if(aws_akid.length()){
        BESDEBUG(MODULE, __FILE__ << " " << __LINE__ << " Using " << ENV_AKID_KEY << " from the environment." << endl);
    }
    else {
        aws_akid.assign(get_config_value(CONFIG_AKID_KEY));
    }

    if(aws_sak.length()){
        BESDEBUG(MODULE, __FILE__ << " " << __LINE__ << " Using " << ENV_SAK_KEY << " from the environment." << endl);
    }
    else {
        aws_sak.assign(get_config_value(CONFIG_SAK_KEY));
    }

    if(aws_region.length()){
        BESDEBUG(MODULE, __FILE__ << " " << __LINE__ << " Using " << ENV_REGION_KEY << " from the environment." << endl);
    }
    else {
        aws_region.assign(get_config_value(CONFIG_REGION_KEY));
    }

    if(aws_s3_bucket.length()){
        BESDEBUG(MODULE, __FILE__ << " " << __LINE__ << " Using " << ENV_S3_BUCKET_KEY << " from the environment." << endl);
    }
    else {
        aws_s3_bucket.assign(get_config_value(CONFIG_S3_BUCKET_KEY));
    }

    BESDEBUG(MODULE, __FILE__ << " " << __LINE__
                              << " END aws_akid: '" << aws_akid << "' "
                              << "aws_sak: '" << aws_sak << "' "
                              << "aws_region: '" << aws_region << "' "
                              << "aws_s3_bucket: '" << aws_s3_bucket << "' "
                              << endl);

    //best_creds = unique_ptr<access_credentials> creds(access_credentials);


    add("https://", new s3_access_credentials(aws_akid,aws_sak,aws_region,aws_s3_bucket));


}
