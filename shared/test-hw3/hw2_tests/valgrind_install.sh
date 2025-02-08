#!/bin/bash
# cd into valgrind zip directory

# Check if Valgrind is already installed
if ! command -v valgrind &> /dev/null; then
    echo "Valgrind is not installed. Proceeding with installation..."
    
    # cd into valgrind zip directory
    mkdir /usr/my
    cp valgrind-3.4.1.tar.bz2 /usr/my/
    tar -xjvf valgrind-3.4.1.tar.bz2
    cd valgrind-3.4.1
    ./configure
    make
    sudo make install
    
    echo "Valgrind installation completed."
else
    echo "Valgrind is already installed."
fi

echo "Done! Now checking if Valgrind exists"
sleep 3
valgrind
