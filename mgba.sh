#!/bin/bash

(
	sleep 2
	echo "started"
)&

/mnt/c/Program\ Files/mGBA/mGBA.exe $1 -g