#!/bin/bash

find ./* -type f | grep -v rplidar-sdk | grep -v ".pro.user" | grep -v "./img/" | grep -v "./src/oscpack/" | xargs wc -l


