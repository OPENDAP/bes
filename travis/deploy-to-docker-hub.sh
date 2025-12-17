#!/bin/bash
#
# Called from the travis.yml. This depends on env vars set by the 
# travis yaml (snapshot image tags) and the Travis repo settings
# (docker hub credentials).
set -eux

echo "Logging into Docker Hub"
echo $DOCKER_HUB_PWSD | docker login -u $DOCKER_HUB_UID --password-stdin

echo "Deploying ${SNAPSHOT_IMAGE_TAG} to Docker Hub"
docker push ${SNAPSHOT_IMAGE_TAG}
echo "Deploying ${BUILD_VERSION_TAG} to Docker Hub"
docker push ${BUILD_VERSION_TAG}

echo "Docker Hub deployment complete."
