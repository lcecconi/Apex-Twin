# Makefile to download datasheets from a list file
# Usage: make all
#        make clean

.PHONY: all clean datasheets

all: datasheets

datasheets:
	@mkdir -p datasheets
	@wget -nc -P datasheets/ -i datasheets/datasheets.txt

clean:
	@rm -f datasheets/*.pdf datasheets/*.rar datasheets/*.zip
