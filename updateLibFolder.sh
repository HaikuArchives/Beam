#! /bin/sh

# create links to shared-libs in a subfolder name 'libs':
if ! [ -d ./lib ]; then 
	mkdir ./lib
fi
ln -sf $PWD/bmBase.so lib/
ln -sf $PWD/libregexx.so lib/
ln -sf $PWD/SantaPartsForBeam.so lib/

# make sure that the add-ons are installed automatically:
if ! [ -d /boot/home/config/add-ons/Beam ]; then 
	mkdir /boot/home/config/add-ons/Beam
fi
if ! [ -d /boot/home/config/add-ons/Beam/Filters ]; then 
	mkdir /boot/home/config/add-ons/Beam/Filters
fi
ln -sf $PWD/filter-addons/Sieve /boot/home/config/add-ons/Beam/Filters/
