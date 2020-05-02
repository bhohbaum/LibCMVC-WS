#!/bin/bash

find ./* -type f | grep -v ".qm$" | grep -v ".pro.user" | grep -v "./img/" | grep -v "./src/oscpack/" | xargs wc -l


