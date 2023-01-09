import glob
import optparse
import os
import sys
import reporter
import subprocess
import util
import shutil

SIKULIX_EXE="c:\\Sikulix\\runIDE.cmd"
MODELEDITOR_OPTION_FILE = "modeleditor.options"
MODELEDITOR_LAYOUT_FILE = "modeleditor.layout"

def runInternalTest( testDir, sikulixExe, url, branchName ):
	
	reportHolder = reporter.ReportHolder( "Automated Testing",
			"%s on %s" % ( "modeleditor UI test", "WIN64" ) )
	
	testDir = testDir.replace( "\\", "/" )
	sikulixExe = sikulixExe.replace( "\\", "/" )
	
	
	if not os.environ.has_key("JAVA_HOME"):
		print "JAVA_HOME Variable does not exist\n"
		return 1
	basePath = os.environ["JAVA_HOME"]
	
	tests = glob.glob( "%s/*.sikuli" % ( testDir ) )
	
	#bakup model editor option file
	currDir = os.getcwd()
	modelEditorOption_file = os.path.join( currDir, MODELEDITOR_OPTION_FILE )
	modelEditorLayout_file = os.path.join( currDir, MODELEDITOR_LAYOUT_FILE )
	if os.path.exists( modelEditorOption_file ):
		shutil.move( modelEditorOption_file, modelEditorOption_file+"_cp" )
	if os.path.exists( modelEditorLayout_file ):
		shutil.move( modelEditorLayout_file, modelEditorLayout_file+"_cp" )

	for test in tests:
		res = 0
		subprocess_output = ""
		
		#copy model editor option file for the test folder
		if os.path.exists( os.path.join( test, MODELEDITOR_OPTION_FILE ) ):
			shutil.copy(os.path.join( test, MODELEDITOR_OPTION_FILE ), modelEditorOption_file)
		elif os.path.exists( modelEditorOption_file+"_cp" ):
			#use defualt
			print "using default ME option file"
			shutil.copy(modelEditorOption_file+"_cp", modelEditorOption_file)

		if os.path.exists( os.path.join( test, MODELEDITOR_LAYOUT_FILE ) ):
			shutil.copy(os.path.join( test, MODELEDITOR_LAYOUT_FILE ), modelEditorLayout_file)
		elif os.path.exists( modelEditorLayout_file+"_cp" ):
			#use defualt
			print "using default ME layout file"
			shutil.copy(modelEditorLayout_file+"_cp", modelEditorLayout_file)
		
		cmd = [sikulixExe, "-r", test, "--args", os.getcwd(),"-c"]
		print cmd
		
		stdout, stderr = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
		try:
			subprocess.check_output("taskkill /im " + "modeleditor.exe" + " /f")
		except subprocess.CalledProcessError, e:
			pass

		report_test = ""
		print stdout
		output = stdout.split("\r")
		for line in output:
			if "[error]" in line:
				report_test += line+"\n"
				res = 1
		
		testFile = os.path.basename(test)
		testName = os.path.splitext(testFile)[0]
		if ( res == 0 ):
			report_test += "SUCCEEDED\n"
		reportHolder.addReport( reporter.Report( res == 0, testName, report_test, branchName ) )
	
		os.remove( modelEditorOption_file )
		os.remove( modelEditorLayout_file )
	
	#return the oreginal model editor option file
	if os.path.exists( modelEditorOption_file+"_cp" ):
		shutil.move( modelEditorOption_file+"_cp", modelEditorOption_file )
	if os.path.exists( modelEditorLayout_file+"_cp" ):
		shutil.move( modelEditorLayout_file+"_cp", modelEditorLayout_file )
		
	reportHolder.buildUrl = url
	reportHolder.sendMail()
	return reportHolder.failCount


def run_modeleditor_UItest():
	bwversion = util.bigworldVersion()
	usage = "\n%prog -t [test directory]"
	
	parser = optparse.OptionParser( usage )
	
	parser.add_option( "-t", "--test",
			dest = "testDir", default=None,
			help = "Source directory for the tests\n")
	parser.add_option( "-s", "--sikulix",
			dest = "sikulix", default=SIKULIX_EXE,
			help = "Set Sikulix executables path, default=" + SIKULIX_EXE + "\n")
	parser.add_option( "-u", "--url ",
					   dest = "url", default=None,
					   help = "The URL for the results in jenkins" )
	parser.add_option( "-b", "--branchName",
		dest = "branchName", default="bw_%d_%d" % ( bwversion[0], bwversion[1] ),
		help = "specify branch name, default=bw_%d_%d" % ( bwversion[0], bwversion[1] ) )
						
	(options, args) = parser.parse_args() 
	
	if None == options.testDir:
		print "\nPlease specify the test directory\n"
		parser.print_help()
		return 1
	
	if not os.path.exists( options.testDir ):
		print options.testDir + " is not a valid path.\n"
		parser.print_help()
		return 1
	
	if not os.path.exists( options.sikulix ):
		print options.sikulix + " is not a valid path.\n"
		parser.print_help()
		return 1
		
	return runInternalTest( options.testDir, options.sikulix, options.url, options.branchName )
	
if __name__ == "__main__":
	sys.exit( run_modeleditor_UItest() )