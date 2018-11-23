#!/bin/bash

output=$( lspci | grep 'MosChip')

if [ ${#output} -eq 0 ]
then
	echo "$(tput setaf 1)Info: Not all devices were connected sucessfully.$(tput sgr0)"
fi
	
