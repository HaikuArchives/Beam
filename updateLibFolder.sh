#! /bin/sh

# create links to shared-libs in a subfolder name 'libs':
if ! [ -d $PWD/lib ]; then 
	mkdir $PWD/lib
fi
ln -sf $PWD/bmBase.so lib/
ln -sf $PWD/libregexx.so lib/
ln -sf $PWD/SantaPartsForBeam.so lib/

# create links to add-ons in a subfolder named 'add-ons':
if ! [ -d $PWD/add-ons ]; then 
	mkdir $PWD/add-ons
fi
if ! [ -d $PWD/add-ons/Filters ]; then 
	mkdir $PWD/add-ons/Filters
fi
ln -sf $PWD/filter-addons/Sieve $PWD/add-ons/Filters/
