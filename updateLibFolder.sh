#! /bin/sh

# create links to shared-libs in a subfolder name 'libs':
if ! [ -d ./lib ]; then 
	mkdir ./lib
fi
if ! [ -h ./lib/bmBase.so ]; then 
	ln -s $PWD/bmBase.so lib/
fi
if ! [ -h ./lib/libregexx.so ]; then 
	ln -s $PWD/libregexx.so lib/
fi
if ! [ -h ./lib/SantaPartsForBeam.so ]; then 
	ln -s $PWD/SantaPartsForBeam.so lib/
fi

# make sure that the add-ons are installed automatically:
if ! [ -d /boot/home/config/add-ons/Beam ]; then 
	mkdir /boot/home/config/add-ons/Beam
fi
if ! [ -d /boot/home/config/add-ons/Beam/Filters ]; then 
	mkdir /boot/home/config/add-ons/Beam/Filters
fi
ln -sf $PWD/filter-addons/Sieve /boot/home/config/add-ons/Beam/Filters/
