import os
import sys
import socket
import optparse

try:
	from P4 import P4, P4Exception
except ImportError, ve:
	print "Cannot find P4Python, please istall p4Python first."
	
SERVER = "pershap4p:1669"

WORKSPACES = {
	"WowpBranch5":
	{
		'Root': '',
		'Client':'bigworld.wowp.branch5', 
		'Options': 'allwrite noclobber nocompress unlocked nomodtime normdir',
		'View': ['//depot/branches/BigWorld/Bigworld_work5/game/bigworld/... //bigworld.wowp.branch5/bigworld/...']
	},
	"WowpBranch6":
	{
		'Root': '',
		'Client':'bigworld.wowp.branch6', 
		'Options': 'allwrite noclobber nocompress unlocked nomodtime normdir',
		'View': ['//depot/branches/BigWorld/Bigworld_work6/game/bigworld/... //bigworld.wowp.branch6/bigworld/...']
	},
	"WowpBranch7":
	{
		'Root': '',
		'Client':'bigworld.wowp.branch7', 
		'Options': 'allwrite noclobber nocompress unlocked nomodtime normdir',
		'View': ['//depot/branches/BigWorld/Bigworld_work7/game/bigworld/... //bigworld.wowp.branch7/bigworld/...']
	},
	"WowpTrunc":
	{
		'Root': '',
		'Client':'bigworld.wowp.trunc', 
		'Options': 'allwrite noclobber nocompress unlocked nomodtime normdir',
		'View': ['//depot/trunc/game/bigworld/... //bigworld.wowp.trunc/bigworld/...']
	},
}

class perforceManager:
	def __init__( self, server, userName, password ):
		self.p4 = P4()
		self.p4.port = server
		self.p4.user = userName
		self.p4.password = password
	
	def connect( self ):
		print "Connecting to Perforce server..."
		self.p4.connect()
		print "Logging in..."
		self.p4.run_login()
		
	def disconnect( self ):
		print "Disconnecting..."
		self.p4.disconnect()
		
	def verifyWorkSpace( self, workSpaceConfig ):
		print "Verifying workSpaces..."
		
		needSave = False
		client = self.p4.fetch_client( workSpaceConfig['Client'] )

		for key, val in workSpaceConfig.items():
			if client[key] != val:
				client[key] = val
				needSave = True
		if needSave:
			print "Workspace " + client['Client'] + " has been changed, saving to perforce..."
			self.p4.save_client( client )
		
	def sync( self, workSpace ):
		print "Syncing " + workSpace + "..."
		self.p4.client = workSpace
		try:
			#The fastest way to revert changed files is to delete and recover them,
			#as far as I know( Bad Perforce!!-_- )
			modified = self.p4.run_diff( "-se" );
			for modifiedItem in modified:
				os.unlink( modifiedItem['clientFile'] )

			if len( self.p4.run_diff( "-sd" ) ):
				print "The client has been changed, reverting all..."
				self.p4.run_sync( "-f" )
			else:
				self.p4.run_sync()
		except P4Exception, e:
			print e.value
			
				
		print workSpace + " synced!"
		
		return True
	
def syncFromPerforce( server, username, password, workSpaceConfig ):
	print "Starting..."

	result = False
	try:
		p = perforceManager( server, username, password )
		p.connect()
		p.verifyWorkSpace( workSpaceConfig )
		result = p.sync( workSpaceConfig['Client'] )
		p.disconnect()
	except P4Exception, e:
		if len(e.value):
			print "Perforce exception: ", e.value
			return False
	
	if result:
		print "Done!"
		
	return result

def main():

	opt = optparse.OptionParser()

	opt.add_option( "-u", "--user",
					dest = "user", default=None,
					help = "The user name to login Perforce server" )

	opt.add_option( "-p", "--password",
					dest = "password", default = None,
					help = "Password for Perforce user" )

	opt.add_option( "-w", "--workspace",
					dest = "workspace", default = None,
					help = "Workspace to sync" )
					
	opt.add_option( "-r", "--rootpath",
					dest = "rootpath", default = None, 
					help = "root path to be synced")

	(options, args) = opt.parse_args()

	if options.user == None or options.password == None:
		print "ERROR: Please specify username and password to login the Perforce server"
		return False
	
	if options.workspace == None:
		print "ERROR: Please specify the workspace to sync"
		return False
	
	if not WORKSPACES.has_key( options.workspace ):
		print "ERROR: The specified workspace doesn't exist"
		return False
		
	if options.rootpath == None:
		options.rootpath = os.getcwd()
		
	WORKSPACES[options.workspace]['Root'] = options.rootpath
	WORKSPACES[options.workspace]['Host'] = socket.gethostname()

	return syncFromPerforce( SERVER, options.user, options.password, WORKSPACES[options.workspace] );
	
	
if __name__ == "__main__":
	sys.exit( not main() )
	
