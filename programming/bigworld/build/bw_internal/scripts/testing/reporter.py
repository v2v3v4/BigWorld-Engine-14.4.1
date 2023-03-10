import os
import socket
import smtplib

from email.mime.text import MIMEText

EMAIL_SERVER = 'mail.bigworldtech.com'
EMAIL_SENDER = 'automation@bigworldtech.com'
EMAIL_ADDRESSES = [ "automation@bigworldtech.com" ]

def _sendEmail( to, subject, emailBody ):
	if type( to ) is not list:
		to = [ to ]	
	to = list( set( to ) )
	
	msg = MIMEText( emailBody )
	msg[ 'Subject' ] = subject
	
	s = smtplib.SMTP( EMAIL_SERVER )
	s.sendmail( EMAIL_SENDER, to, msg.as_string() )
	s.quit()
	

class Report:
	def __init__( self, success, tag, reportBody, branchName ):
		self.success = success
		self.tag = tag
		self.reportBody = reportBody
		self.branchName = branchName

class ReportHolder:
	def __init__( self, category, name, buildUrl, changelist ):
		self.reports = []
		self.category = category
		self.name = name
		self.buildUrl = buildUrl
		self.changelist = changelist
		
	def sendMail( self ):
		if len( self.reports ) == 0 or len( EMAIL_ADDRESSES ) == 0:
			return
		
		self.failCount = 0
		mailBody = "" + str(self.buildUrl) + "\n\n"
		mailBody += "Changelist %s\n" % (self.changelist)
		
		indentation = "    "
		print "Sending e-mail (%d reports)..." % len(self.reports)
		for report in self.reports:
			status = "succeeded"
			if not report.success:
				status = "FAILED!!"
				self.failCount += 1
			mailBody += "*" * 70 + "\n"
			mailBody += "%s %s %s\n" % (report.tag, report.branchName, status)
			mailBody += "*" * 70 + "\n"
			mailBody += indentation + report.reportBody.replace( "\n","\n%s"%indentation )
			mailBody += "\n"
			
		mailBody = "%d tests failed\n\n" % self.failCount + mailBody
		mailBody += "#"*70 + "\n"
		mailBody += "This message is generated by %s, from host %s" % (__file__, socket.gethostname())
		
		if self.failCount:
			mailTitle = "[%s] FAIL: %s" % ( self.category, self.name )
		else:
			mailTitle = "[%s] OK: %s" % ( self.category, self.name )
			
		_sendEmail( EMAIL_ADDRESSES, mailTitle, mailBody )
		print "Finished sending report!"
	
	def addReport( self, report ):
		assert( isinstance( report, Report ) )
		self.reports.append( report )

