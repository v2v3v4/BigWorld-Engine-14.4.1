#ifndef UMBRA_PROXIES_HPP
#define UMBRA_PROXIES_HPP

#include "umbra_config.hpp"

#if UMBRA_ENABLE

namespace Umbra
{
	namespace OB 
	{
		class Model;
		class Object;
	}
};


BW_BEGIN_NAMESPACE

/**
 *	This class implements the umbra model proxy.
 *	The umbra model proxy keeps a reference counted reference to a
 *	umbra model. The umbra model proxy registers itself in a list
 *	so that the umbra model can be destroyed before umbra is destroyed.
 */
class UmbraModelProxy : public SafeReferenceCount
{
public:

	void invalidate();

	Umbra::OB::Model *	model() const { return pModel_; }
	void				model(Umbra::OB::Model * pModel){ pModel_ = pModel; }

	static UmbraModelProxy* getObbModel( const Matrix& obb );
	static UmbraModelProxy* getObbModel( const Vector3* pVertices, uint32 nVertices );
	static UmbraModelProxy* getObbModel( const Vector3& min, const Vector3& max );
	static void invalidateAll();
private:
	static UmbraModelProxy * get( Umbra::OB::Model* pModel );
	static void tryDestroy( const UmbraModelProxy * pProxy );
	virtual void destroy() const;
	UmbraModelProxy();
	~UmbraModelProxy();
	Umbra::OB::Model * pModel_;

	typedef BW::vector<UmbraModelProxy*> Proxies;
	static Proxies proxies_;

};

typedef SmartPointer<UmbraModelProxy> UmbraModelProxyPtr;

/**
 *	This class implements the umbra object proxy.
 *	The umbra object proxy keeps a reference counted reference to a
 *	umbra object. The umbra object proxy registers itself in a list
 *	so that the umbra object can be destroyed before umbra is destroyed.
 */
class UmbraObjectProxy : public SafeReferenceCount
{
public:
	void invalidate();
	void				pModelProxy( UmbraModelProxyPtr pModel );
	UmbraModelProxyPtr	pModelProxy() const { return pModel_; }
	Umbra::OB::Object*	object() const { return pObject_; }
	void				object(Umbra::OB::Object* pObject){ pObject_ = pObject; }

	static UmbraObjectProxy* get( UmbraModelProxyPtr pModel, const BW::string& identifier = "" );
	static UmbraObjectProxy* getCopy( const BW::string& identifier );
	static UmbraObjectProxy* get( Umbra::OB::Object* pObject );

	static void invalidateAll();
private:
	static void tryDestroy( const UmbraObjectProxy * pProxy );
	virtual void destroy() const;
	UmbraObjectProxy(UmbraModelProxyPtr pModel = NULL);
	~UmbraObjectProxy();
	Umbra::OB::Object* pObject_;

	UmbraModelProxyPtr pModel_;
	typedef BW::vector<UmbraObjectProxy*> Proxies;
	static Proxies proxies_;
	typedef BW::map<BW::string, UmbraObjectProxy*> NamedProxies;
	static NamedProxies namedProxies_;
};

typedef SmartPointer<UmbraObjectProxy> UmbraObjectProxyPtr;

BW_END_NAMESPACE

#endif // UMBRA_ENABLE

#endif // UMBRA_PROXIES_HPP
/*umbra_proxies.hpp*/
