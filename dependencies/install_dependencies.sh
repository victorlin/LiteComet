# Install boost libraries we needed
sudo apt-get install -y libboost-dev libboost-date-time-dev libboost-thread-dev libboost-system-dev libboost-filesystem-dev libboost-regex-dev libboost-signals-dev libboost-iostreams-dev libboost-test-dev

# Install other libraries we needed
sudo apt-get install -y libbz2-dev libzip-dev openssl libxml2-dev

# Install Log4CPlus
cd log4cplus-1.0.4-rc10
cmake -G "Unix Makefiles"
make 
sudo make install
