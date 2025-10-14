# About S3 bucket lifecycle policies

## How to apply the policy to a bucket
tl;dr 

```bash
aws s3api put-bucket-lifecycle-configuration \
  --bucket YOUR-BUCKET \
  --lifecycle-configuration file://lifecycle.json
```
where for us YOUR-BUCKET is ```opendap.travis.tests``` (see upload-test-results.sh in this directory)

## Backstory 

We have an S3 bucket that our CICD scripts use for interim build products. This describes S3 bucket policies that limit the number of those files in the bucket, deleting older files as newer ones are added.

### S3 Lifecycle rules (zero code)

**Best for “delete anything older than N days/months.”**
Create a lifecycle rule that targets your CI artifacts by **prefix** (e.g., `builds/`) and/or **tag** (e.g., `purpose=ci-temp`):

* **Expire current objects after N days** (e.g., 90 days).
* **Abort incomplete multipart uploads** after a short time (e.g., 1–3 days) to avoid orphaned parts.
 
* Note that our buckets like ```opendap.travis.test``` don't currently use versioning and we're not pushing objects using versioning.

**Example lifecycle JSON (age-based delete, no versioning):**

```json
{
  "Rules": [
    {
      "ID": "expire-ci-builds-90d",
      "Status": "Enabled",
      "Filter": { "Prefix": "builds/" },
      "Expiration": { "Days": 90 },
      "AbortIncompleteMultipartUpload": { "DaysAfterInitiation": 3 }
    }
  ]
}
```
---
## How to do this

### Option A — Console (fastest)

1. **Open** S3 → **Buckets** → pick your bucket → **Management** (Lifecycle).
2. **Create lifecycle rule** → give it a name (e.g., `expire-ci-builds-90d`).
3. **Choose scope**

    * Filter by **Prefix**: `builds/` (or your CI path)
    * (Optional) add a **Tag** like `purpose=ci-temp` to target only CI objects.
4. **Actions**

    * **Expire current versions** after **90 days** (or whatever you want).
    * **Abort incomplete multipart uploads** after **3 days**.

5. **Review & create**. Lifecycle runs **once per day**, and existing objects already older than the threshold get queued for removal when the rule first evaluates. ([AWS Documentation][1])
---

### Option B — CLI + JSON (repeatable, IaC-friendly)

Create a file `lifecycle.json` and apply it with:

```bash
aws s3api put-bucket-lifecycle-configuration \
  --bucket YOUR-BUCKET \
  --lifecycle-configuration file://lifecycle.json
```

([AWS Documentation][2])

### 1) Non-versioned bucket: expire after 90 days (+ abort MPUs)

```json
{
  "Rules": [
    {
      "ID": "expire-ci-builds-90d",
      "Status": "Enabled",
      "Filter": {
        "And": {
          "Prefix": "builds/",
          "Tags": [
            { "Key": "purpose", "Value": "ci-temp" }
          ]
        }
      },
      "Expiration": { "Days": 90 },
      "AbortIncompleteMultipartUpload": { "DaysAfterInitiation": 3 }
    }
  ]
}
```
----
[1]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/how-to-set-lifecycle-configuration-intro.html?utm_source=chatgpt.com "Setting an S3 Lifecycle configuration on a bucket"
[2]: https://docs.aws.amazon.com/cli/latest/reference/s3api/put-bucket-lifecycle-configuration.html?utm_source=chatgpt.com "put-bucket-lifecycle-configuration — AWS CLI 2.31.0 Command Reference"


