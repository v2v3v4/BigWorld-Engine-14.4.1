#!/bin/sh

# PackTextures.sh
#
# Created by license on 30/11/10.
# Copyright 2010 __MyCompanyName__. All rights reserved.

# modified version from PR

#! /bin/sh
# NOTE: change this to the location of Texture packer if we need to re-do the
# spritesheets. location: /usr/local/bin/TexturePacker
TP=TexturePacker

which ${TP} > /dev/null 2>&1

if [ $? != 0 ]; then
	echo ${TP} does not exist. Consider installing it.
	echo Skipping Texture Packing stage
	exit 0
fi

if [ "${ACTION}" = "clean" ]
then
	# remove all files
	rm ${SRCROOT}/Resources/entities/EntitySprites.png
	rm ${SRCROOT}/Resources/entities/EntitySprites.plist
	rm ${SRCROOT}/Resources/entities/EntitySprites-hd.png
	rm ${SRCROOT}/Resources/entities/EntitySprites-hd.plist
else
	# create HD assets
	${TP} --smart-update ${SRCROOT}/Resources/EntitySource \
		--format cocos2d \
		--shape-padding 1 \
		--data ${SRCROOT}/Resources/entities/EntitySprites-hd.plist \
		--sheet ${SRCROOT}/Resources/entities/EntitySprites-hd.png

	# create sd assets from same sprites
	# NOTE: we are capping the size at 1024 due to the non-retina devices not supporting textures larger than 1024.
	${TP} --smart-update --scale 0.5 ${SRCROOT}/Resources/EntitySource \
		--max-size 1024 \
		--format cocos2d \
		--data ${SRCROOT}/Resources/entities/EntitySprites.plist \
		--shape-padding 1 \
		--sheet ${SRCROOT}/Resources/entities/EntitySprites.png
fi
exit 0
