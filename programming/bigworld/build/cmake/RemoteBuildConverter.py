import sys
import xml.etree.ElementTree as ET

if len(sys.argv) != 4:
	if len(sys.argv) > 1:
		print "Invalid command line"
	print sys.argv[0]+" <binary_path> <proj_name> <proj_path>"
	sys.exit(1)
binary_path = sys.argv[1]
proj_name = sys.argv[2]
proj_path = sys.argv[3]
vcxproj_filename = binary_path+'/'+proj_path+'/'+proj_name+'.vcxproj'
build_cmdline = binary_path + "/_remote_build.bat \""+proj_path+"\""
rebuild_cmdline = binary_path + "/_remote_rebuild.bat \""+proj_path+"\""
clean_cmdline = binary_path + "/_remote_clean.bat \""+proj_path+"\""


build_ns='http://schemas.microsoft.com/developer/msbuild/2003'
namespaces = {'msbuild' : build_ns}
ET.register_namespace('',build_ns)

def removeAll(et, search):
	while True:
		idg = tree.find("msbuild:ItemDefinitionGroup", namespaces=namespaces)
		if idg is None:
			break
		et.remove(idg)

def copyConditional(propertyGroup, newElement, tree, oldElement):
	items = tree.findall("msbuild:ItemDefinitionGroup", namespaces=namespaces)
	for item in items:
		element = item.find('msbuild:'+'/msbuild:'.join(oldElement), namespaces=namespaces)
		if element is not None:
			attribs = {}
			if item.attrib.has_key("Condition"):
				attribs["Condition"]=item.attrib["Condition"]
			ET.SubElement(propertyGroup, newElement, attribs).text = \
				element.text.replace("%("+oldElement[-1]+")","%("+newElement+")")

tree = ET.parse(vcxproj_filename)

# Set configuration type
items = tree.findall("msbuild:PropertyGroup/msbuild:ConfigurationType", namespaces=namespaces)
for item in items:
	if item.text == 'Makefile':
		# Already processed
		sys.exit(0)
	item.text = 'Makefile'

propertyGroup = tree.find("msbuild:PropertyGroup/msbuild:_ProjectFileVersion/..", namespaces=namespaces)

# Set build
# <NMakeBuildCommandLine>
ET.SubElement(propertyGroup,'NMakeBuildCommandLine', Condition="'$(Configuration)'=='Hybrid'").text=build_cmdline+" hybrid"
ET.SubElement(propertyGroup,'NMakeBuildCommandLine', Condition="'$(Configuration)'=='Debug'").text=build_cmdline+" debug"

# Set rebuild
# <NMakeReBuildCommandLine>
ET.SubElement(propertyGroup,'NMakeReBuildCommandLine', Condition="'$(Configuration)'=='Hybrid'").text=rebuild_cmdline+" hybrid"
ET.SubElement(propertyGroup,'NMakeReBuildCommandLine', Condition="'$(Configuration)'=='Debug'").text=rebuild_cmdline+" debug"

# Set clean
# <NMakeCleanCommandLine>
ET.SubElement(propertyGroup,'NMakeCleanCommandLine', Condition="'$(Configuration)'=='Hybrid'").text=clean_cmdline+" hybrid"
ET.SubElement(propertyGroup,'NMakeCleanCommandLine', Condition="'$(Configuration)'=='Debug'").text=clean_cmdline+" debug"

# Set output file to use what CMake sets
ET.SubElement(propertyGroup,'NMakeOutput').text="$(OutDir)$(TargetName)$(TargetExt)"
# Otherwise it would be a Copy output:
#copyConditional(propertyGroup, 'NMakeOutput', tree, 'msbuild:Link/msbuild:OutputFile')

## IntelliSense only:
# Copy Preprocessor
copyConditional(propertyGroup, 'NMakePreprocessorDefinitions', tree, ('ClCompile','PreprocessorDefinitions'))

# Copy Include search path
copyConditional(propertyGroup, 'NMakeIncludeSearchPath', tree, ('ClCompile','AdditionalIncludeDirectories'))

# Copy Force include
copyConditional(propertyGroup, 'NMakeForcedIncludes', tree, ('ClCompile','ForcedIncludeFiles'))

# Copy Additional options
copyConditional(propertyGroup, 'AdditionalOptions', tree, ('ClCompile','AdditionalOptions'))


# Not handled:
# Assembly search path ( NMakeAssemblySearchPath )
# Force using assemblies ( NMakeForcedUsingAssemblies )


# Clean up
#removeAll(tree.getroot(), "msbuild:ItemDefinitionGroup")

tree.write(vcxproj_filename,
			xml_declaration=True,
			encoding='utf-8')
