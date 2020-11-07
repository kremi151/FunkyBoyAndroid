#!/bin/sh

cd /github/workspace
chmod +x ./gradlew
./gradlew assembleRelease -x test
