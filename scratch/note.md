

EffectiveUrlCache, NgapOwnedContainer: Percolate the request_headers up to from the use of curl::http_get() so that the URLs retrieved from CMR are marked as EDL authenticted as needed (TEA URLs for example) and only inject EDL auth headers to requests that are going to know EDL auth'd servuices.

Stuff that touches awsv4.cc - Remove? Seperate ticket? (Prolly yes)

Kodi - stop flogging the Travis and follow Hannah's suggestion that we checkpoint save the bes_core image right before we run the tests. Then, pull that image form docker hub and run it locally to run the tests and search for the various results to tar up. Once the test  resulsts can be collected we can choose to checkoit the bes_cor or not.



https://opendap.earthdata.nasa.gov/collections/C1276812851-GES_DISC/granules/M2T1NXRAD.5.12.4:MERRA2_100.tavg1_2d_rad_Nx.19800101.nc4.dap.csv?dap4.ce=/ALBEDO

Looking at how to disable the SignedUrlCache and I learned that it's controlled by the sam BESKeys key that also controls the EffectioveUrlCache:
```
Http.cache.effective.urls=true
Http.cache.effective.urls.skip.regex.pattern=^https:\/\/.*s3(\.|-).*\.amazonaws\.com\/.*$
```
And that got me thinking about hwo to turn it off to test my changes to the edl transaction

And that got me noticing that in the `NgapOwnedContainer::dmrpp_read_from_daac_bucket()` wether or not the presigned URL is in the SignedUrlCache the dmrpp is retrieved into memeory and then filtered: 
```
        filter_response(content_filters, dmrpp_string);
```
But the dmr++ url is not cached or signed or what ever 

The only explict use if the EffectiveUrlCache is in `Chunk::get_data_url()`:
```
    std::shared_ptr<http::EffectiveUrl> url = SignedUrlCache::TheCache()->get_presigned_s3_url(d_data_url);

    // If the url signing fails for any reason---nonexistant or bad short-term credentials, being
    // called from a region other than us-west-2, etc---it will return a nullptr, so that we can fall
    // back on using the TEA service to sign our urls
    if (url == nullptr) {
        curl_slist *req_hdrs = nullptr;
        if (d_data_url->is_trusted()) {
            req_hdrs = curl::add_edl_auth_headers(nullptr);
        }
        url = EffectiveUrlCache::TheCache()->get_effective_url(d_data_url, req_hdrs);
    }
```
Where if the presigned URL is not located in the SIgnedUrlCache then the signed URL is retrueved from TEA and placed in the EffectiveUrlCache:
```
        url = EffectiveUrlCache::TheCache()->get_effective_url(d_data_url, req_hdrs);
```