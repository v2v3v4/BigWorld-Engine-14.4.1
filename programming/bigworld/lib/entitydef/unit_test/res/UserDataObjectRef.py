# --------------------------------------------------------------------------
# This class implements a very basic User Data Object, necesary for the User
# Data Object linking system to work properly. Might be removed in the
# future if an alternative solution is found.
# --------------------------------------------------------------------------


import BigWorld


class UserDataObjectRef(BigWorld.UserDataObject):	
	def __init__(self):
		BigWorld.UserDataObject.__init__(self)
