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

We have an S3 bucket that our CICD scripts use for interim build products. I would like to limit the number of those files in the bucket, deleting older files as newer ones are added.

### S3 Lifecycle rules (zero code)

**Best for “delete anything older than N days/months.”**
Create a lifecycle rule that targets your CI artifacts by **prefix** (e.g., `builds/`) and/or **tag** (e.g., `purpose=ci-temp`):

* **Expire current objects after N days** (e.g., 90 days).
* **Abort incomplete multipart uploads** after a short time (e.g., 1–3 days) to avoid orphaned parts.
* If you use **versioning**, you can also:

    * **Expire noncurrent versions after N days**.
    * **Limit the number of noncurrent versions** (keep only the newest K noncurrent versions).
 
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

**Why this is nice:** dead simple, free, and completely automatic.

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
    * If the bucket is **versioned** and you overwrite the *same key*:

        * **Permanently delete noncurrent versions** after **30 days** and **retain only the newest N noncurrent versions** (e.g., **5**).
5. **Review & create**. Lifecycle runs **once per day**, and existing objects already older than the threshold get queued for removal when the rule first evaluates. ([AWS Documentation][1])

> Note: “Keep newest 5” in Lifecycle only applies to **noncurrent versions of the same key** (versioned buckets). It **can’t** keep “newest 5 objects by prefix” like `build-123.tgz`, `build-124.tgz`, etc.—use the Lambda approach for that case.

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

* `Expiration.Days` removes objects older than 90 days.
* `AbortIncompleteMultipartUpload` cleans up stuck multipart uploads. ([AWS Documentation][3])

### 2) Versioned bucket: limit noncurrent versions (+ age)

```json
{
  "Rules": [
    {
      "ID": "limit-noncurrent-versions",
      "Status": "Enabled",
      "Filter": { "Prefix": "builds/" },
      "NoncurrentVersionExpiration": {
        "NoncurrentDays": 30,
        "NewerNoncurrentVersions": 5
      },
      "AbortIncompleteMultipartUpload": { "DaysAfterInitiation": 3 }
    }
  ]
}
```

* Keeps only the **newest 5 noncurrent versions** of each key and deletes older noncurrent versions (after they’re 30 days old). ([AWS Documentation][4])

---

## Verify / inspect

* **Get current config**

  ```bash
  aws s3api get-bucket-lifecycle-configuration --bucket YOUR-BUCKET
  ```
* Expect actions to be applied **asynchronously**; lifecycle evaluations run **daily** and S3 queues existing, already-old objects on the first run. You’re not billed once objects are marked for expiration. ([AWS Documentation][5])

---

## Practical tips

* **Start narrow**: first target a sub-prefix (e.g., `builds/dev/`) or require the tag `purpose=ci-temp`.
* **Don’t overlap rules** for the same objects unless you mean to (prefix+tag filters help). ([AWS CLI Command Reference][6])
* **Multipart uploads**: always add the abort-MPU action; it stops orphaned parts from piling up. ([AWS Documentation][7])

If you share your exact bucket/prefix scheme (and whether versioning is on), I can tailor the JSON so you can drop it in verbatim.

[1]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/how-to-set-lifecycle-configuration-intro.html?utm_source=chatgpt.com "Setting an S3 Lifecycle configuration on a bucket"
[2]: https://docs.aws.amazon.com/cli/latest/reference/s3api/put-bucket-lifecycle-configuration.html?utm_source=chatgpt.com "put-bucket-lifecycle-configuration — AWS CLI 2.31.0 Command Reference"
[3]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/lifecycle-configuration-examples.html?utm_source=chatgpt.com "Examples of S3 Lifecycle configurations - Amazon Simple Storage Service"
[4]: https://docs.aws.amazon.com/AmazonS3/latest/API/API_NoncurrentVersionExpiration.html "NoncurrentVersionExpiration - Amazon Simple Storage Service"
[5]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/object-lifecycle-mgmt.html?utm_source=chatgpt.com "Managing the lifecycle of objects - Amazon Simple Storage Service"
[6]: https://awscli.amazonaws.com/v2/documentation/api/2.4.18/reference/s3api/put-bucket-lifecycle-configuration.html?utm_source=chatgpt.com "put-bucket-lifecycle-configuration — AWS CLI 2.4.18 Command Reference"
[7]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/mpu-abort-incomplete-mpu-lifecycle-config.html?utm_source=chatgpt.com "Configuring a bucket lifecycle configuration to delete incomplete ..."

