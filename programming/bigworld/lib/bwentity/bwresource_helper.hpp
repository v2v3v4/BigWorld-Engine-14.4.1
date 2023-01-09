#ifndef BWENTITY_BWRESOURCE_HELPER_HPP
#define BWENTITY_BWRESOURCE_HELPER_HPP

BWENTITY_BEGIN_NAMESPACE

class BWResourceHelper
{
public:
	BWResourceHelper();
	~BWResourceHelper();
	
	/**
	 * This method creates and initialize BW::BWResource singleton 
	 */ 
	bool create( const char ** resPathArray );
	
	/**
	 * This method opens a BW::BWResource section by path  
	 */ 
	static DataSectionPtr openSection( char * pathToResource );
	
private:
	/**
	 * This method deletes initialize BW::BWResource singleton  
	 */ 
	void destroy();
	
	bool initialized_;
};

BWENTITY_END_NAMESPACE

#endif /* BWENTITY_BWRESOURCE_HELPER_HPP */
