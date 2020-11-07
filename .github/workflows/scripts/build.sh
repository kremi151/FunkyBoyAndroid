#!/bin/sh

cd /github/workspace
makeLocalProperties
chmod +x ./gradlew
./gradlew assembleRelease -x test
