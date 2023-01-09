#!/usr/bin/env python

import xml.etree.ElementTree as ET
import build_common
import os

# This class handles iterating over a set of Visual Studio build configurations
# for one or more Visual Studio project solutions. For example:
#   for project in [ world_editor.sln, particle_editor.sln  ]:
#      for config in [ "Editor_Hybrid", "Editor_Debug", "Editor_Release" ]
#          # generate (project, config)
class ProjectConfigIterator( object ):

	def __init__( self, projects, configs, vsVersion ):

		if len( projects ) == 0:
			raise ValueError( "Invalid project list" )

		if len( configs ) == 0:
			raise ValueError( "Invalid config list" )

		self.projects = projects
		self.configs  = configs

		self.vsVersion = vsVersion

		self.currProjIndex = 0
		self.currConfIndex = 0


	def __iter__( self ):
		return self


	def _incCounters( self ):
		self.currConfIndex += 1


	def next( self ):
		isEndOfProjects = self.currProjIndex >= len( self.projects )
		isEndOfConfigs  = self.currConfIndex >= len( self.configs )

		if isEndOfProjects and isEndOfConfigs:
			raise StopIteration

		if isEndOfConfigs:
			self.currProjIndex += 1
			self.currConfIndex = 0

			if self.currProjIndex >= len( self.projects ):
				raise StopIteration

		# Perform a replacement on the project string with the Visual Studio
		# version (if required)
		projectString = self.projects[ self.currProjIndex ].replace( "%(vsVersion)s", self.vsVersion )
		retData = (projectString, self.configs[ self.currConfIndex ])

		self._incCounters()

		return retData

###############################
# Main module accessor methods
def generateProjectConfig( vsVersion, buildType, xmlElement ):
	
	if xmlElement is None:
		return None
		
	isContinuousBuild = build_common.isContinuousBuild( buildType )
	pathList = []
	configList = []
	itemPaths = xmlElement.find( "paths" )
	if itemPaths != None:
		pathList = [ os.path.normpath( path.text.strip() ) for path in itemPaths.findall( "path" ) ]
		
	configs = xmlElement.find( "build_configs" )
	if configs != None:
		if isContinuousBuild:
			for config in configs.findall( "config" ):
				if 'continuous_build' in config.attrib and \
					config.attrib['continuous_build'] == "True":
					configList.append( config.text.strip() )
		else:
			configList = [ config.text.strip() for config in configs.findall( "config" ) ]

	return ProjectConfigIterator( pathList, configList, vsVersion )
			
	
def getProjects( vsVersion, buildType, projectConfigPath, tag = None ):
	projectList = []
	
	doc = ET.parse( projectConfigPath )
	root = doc.getroot()
	childrenList = root.getchildren()
	for item in childrenList:
		if tag != None:
			if tag == item.tag:
				projectList.append( generateProjectConfig( vsVersion, buildType, item ) )
				break
		else:
			projectList.append( generateProjectConfig( vsVersion, buildType, item ) )
		
	return projectList

# project_manager.py
