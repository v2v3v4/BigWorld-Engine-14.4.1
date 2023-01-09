#include "pch.hpp"

#include "cuckoo_cycle_login_challenge_factory.hpp"

#include "login_challenge.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/watcher.hpp"

namespace BWOpenSSL
{
#include "openssl/rand.h"
} // end namespace BWOpenSSL

namespace Cuckoo
{

// This is the original default implementation setting.
#define SIZESHIFT 20

// -----------------------------------------------------------------------------
// Section: cuckoo.h
// -----------------------------------------------------------------------------

#include "openssl/sha.h"
#include <stdint.h>
#include <string.h>

// proof-of-work parameters
#ifndef SIZESHIFT 
#define SIZESHIFT 25
#endif
#ifndef PROOFSIZE
#define PROOFSIZE 42
#endif

#define SIZE (1UL<<SIZESHIFT)
#define HALFSIZE (SIZE/2)
#define NODEMASK (HALFSIZE-1)

typedef uint32_t u32;
typedef uint64_t u64;
#if SIZESHIFT <= 32
typedef u32 nonce_t;
typedef u32 node_t;
#else
typedef u64 nonce_t;
typedef u64 node_t;
#endif

typedef struct {
  u64 v[4];
} siphash_ctx;
 
#define U8TO64_LE(p) \
  (((u64)((p)[0])      ) | ((u64)((p)[1]) <<  8) | \
   ((u64)((p)[2]) << 16) | ((u64)((p)[3]) << 24) | \
   ((u64)((p)[4]) << 32) | ((u64)((p)[5]) << 40) | \
   ((u64)((p)[6]) << 48) | ((u64)((p)[7]) << 56))
 
// derive siphash key from header
void setheader(siphash_ctx *ctx, const char *header) {
  unsigned char hdrkey[32];
  SHA256((unsigned char *)header, strlen(header), hdrkey);
  u64 k0 = U8TO64_LE(hdrkey);
  u64 k1 = U8TO64_LE(hdrkey+8);
  ctx->v[0] = k0 ^ 0x736f6d6570736575ULL;
  ctx->v[1] = k1 ^ 0x646f72616e646f6dULL;
  ctx->v[2] = k0 ^ 0x6c7967656e657261ULL;
  ctx->v[3] = k1 ^ 0x7465646279746573ULL;
}

#define ROTL(x,b) (u64)( ((x) << (b)) | ( (x) >> (64 - (b))) )
#define SIPROUND \
  do { \
    v0 += v1; v2 += v3; v1 = ROTL(v1,13); \
    v3 = ROTL(v3,16); v1 ^= v0; v3 ^= v2; \
    v0 = ROTL(v0,32); v2 += v1; v0 += v3; \
    v1 = ROTL(v1,17);   v3 = ROTL(v3,21); \
    v1 ^= v2; v3 ^= v0; v2 = ROTL(v2,32); \
  } while(0)
 
// SipHash-2-4 specialized to precomputed key and 8 byte nonces
u64 siphash24(siphash_ctx *ctx, u64 nonce) {
  u64 v0 = ctx->v[0], v1 = ctx->v[1], v2 = ctx->v[2], v3 = ctx->v[3] ^ nonce;
  SIPROUND; SIPROUND;
  v0 ^= nonce;
  v2 ^= 0xff;
  SIPROUND; SIPROUND; SIPROUND; SIPROUND;
  return v0 ^ v1 ^ v2  ^ v3;
}

// generate edge endpoint in cuckoo graph
node_t sipnode(siphash_ctx *ctx, nonce_t nonce, u32 uorv) {
  return siphash24(ctx, 2*nonce + uorv) & NODEMASK;
}

void sipedge(siphash_ctx *ctx, nonce_t nonce, node_t *pu, node_t *pv) {
  *pu = sipnode(ctx, nonce, 0);
  *pv = sipnode(ctx, nonce, 1);
}

// verify that (ascending) nonces, all less than easiness, form a cycle in header-generated graph
int verify(nonce_t nonces[PROOFSIZE], const char *header, u64 easiness) {
  siphash_ctx ctx;
  setheader(&ctx, header);
  node_t us[PROOFSIZE], vs[PROOFSIZE];
  u32 i = 0, n;
  for (n = 0; n < PROOFSIZE; n++) {
    if (nonces[n] >= easiness || (n && nonces[n] <= nonces[n-1]))
      return 0;
    sipedge(&ctx, nonces[n], &us[n], &vs[n]);
  }
  do {  // follow cycle until we return to i==0; n edges left to visit
    u32 j = i;
    for (u32 k = 0; k < PROOFSIZE; k++) // find unique other j with same vs[j]
      if (k != i && vs[k] == vs[i]) {
        if (j != i)
          return 0;
        j = k;
    }
    if (j == i)
      return 0;
    i = j;
    for (u32 k = 0; k < PROOFSIZE; k++) // find unique other i with same us[i]
      if (k != j && us[k] == us[j]) {
        if (i != j)
          return 0;
        i = k;
    }
    if (i == j)
      return 0;
    n -= 2;
  } while (i);
  return n == 0;
}


// -----------------------------------------------------------------------------
// Section: simple_miner.cpp (with modifications)
// -----------------------------------------------------------------------------

// BigWorld changes are marked using this macro.
#define BW_CHANGES 1

// ok for size up to 2^32
#define MAXPATHLEN 8192

class cuckoo_ctx {
public:
  siphash_ctx sip_ctx;
  nonce_t easiness;
  node_t *cuckoo;

  cuckoo_ctx(const char* header, nonce_t easy_ness) {
    setheader(&sip_ctx, header);
    easiness = easy_ness;
#if !BW_CHANGES
    assert(cuckoo = (node_t *)calloc(1+SIZE, sizeof(node_t)));
#else // BW_CHANGES
    cuckoo = (node_t *)calloc(1+SIZE, sizeof(node_t));
#endif // !BW_CHANGES
  }
  ~cuckoo_ctx() {
    free(cuckoo);
  }
};

int path(node_t *cuckoo, node_t u, node_t *us) {
  int nu;
  for (nu = 0; u; u = cuckoo[u]) {
    if (++nu >= MAXPATHLEN) {
      while (nu-- && us[nu] != u) ;
#if !BW_CHANGES
      if (nu < 0)
        printf("maximum path length exceeded\n");
      else printf("illegal % 4d-cycle\n", MAXPATHLEN-nu);
      exit(0);
#else // BW_CHANGES
	  if (nu < 0)
	  {
		return nu;
	  }
#endif // !BW_CHANGES
    }
    us[nu] = u;
  }
  return nu;
}

typedef std::pair<node_t,node_t> edge;
#if BW_CHANGES
typedef BW::vector< nonce_t > Solution;
#endif // BW_CHANGES

#if !BW_CHANGES
void solution(cuckoo_ctx *ctx, node_t *us, int nu, node_t *vs, int nv) {
#else // BW_CHANGES
void solution(cuckoo_ctx *ctx, node_t *us, int nu, node_t *vs, int nv,
		Solution & sol) {
#endif // !BW_CHANGES
  std::set<edge> cycle;
  unsigned n;
  cycle.insert(edge(*us, *vs));
  while (nu--)
    cycle.insert(edge(us[(nu+1)&~1], us[nu|1])); // u's in even position; v's in odd
  while (nv--)
    cycle.insert(edge(vs[nv|1], vs[(nv+1)&~1])); // u's in odd position; v's in even
#if !BW_CHANGES
  printf("Solution");
#endif // !BW_CHANGES
  for (nonce_t nonce = n = 0; nonce < ctx->easiness; nonce++) {
    edge e(1+sipnode(&ctx->sip_ctx, nonce, 0), 1+HALFSIZE+sipnode(&ctx->sip_ctx, nonce, 1));
    if (cycle.find(e) != cycle.end()) {
#if !BW_CHANGES
      printf(" %lx", nonce);
#else
      sol.push_back( nonce );
#endif // !BW_CHANGES
      cycle.erase(e);
    }
  }
#if !BW_CHANGES
  printf("\n");
#endif // !BW_CHANGES
}

#if !BW_CHANGES
void worker(cuckoo_ctx *ctx) {
#else
void worker(cuckoo_ctx *ctx, Solution & sol ) {
#endif
  node_t *cuckoo = ctx->cuckoo;
  node_t us[MAXPATHLEN], vs[MAXPATHLEN];
  for (node_t nonce = 0; nonce < ctx->easiness; nonce++) {
    node_t u0, v0;
    sipedge(&ctx->sip_ctx, nonce, &u0, &v0);
    u0 += 1        ;  // make non-zero
    v0 += 1 + HALFSIZE;  // make v's different from u's
    node_t u = cuckoo[u0], v = cuckoo[v0];
    if (u == v0 || v == u0)
      continue; // ignore duplicate edges
    us[0] = u0;
    vs[0] = v0;

#if !BW_CHANGES
#ifdef SHOW
    for (unsigned j=0; j<SIZE; j++)
      if (!cuckoo[1+j]) printf("%2d:   ",j);
      else            printf("%2d:%02ld ",j,cuckoo[1+j]-1);
    printf(" %lx (%ld,%ld)\n", nonce,*us-1,*vs-1);
#endif
#endif // !BW_CHANGES
    int nu = path(cuckoo, u, us), nv = path(cuckoo, v, vs);
#if BW_CHANGES
	if ((nu < 0) || (nv < 0))
	{
		// path returned error.
		return;
	}
#endif // BW_CHANGES
    if (us[nu] == vs[nv]) {
      int min = nu < nv ? nu : nv;
      for (nu -= min, nv -= min; us[nu] != vs[nv]; nu++, nv++) ;
      int len = nu + nv + 1;
#if !BW_CHANGES
      printf("% 4d-cycle found at %d%%\n", len, (int)(nonce*100L/ctx->easiness));
#endif // !BW_CHANGES

#if !BW_CHANGES
      if (len == PROOFSIZE)
        solution(ctx, us, nu, vs, nv);
#else
	// We just need one solution.
      if (len == PROOFSIZE)
	  {
        solution(ctx, us, nu, vs, nv, sol);
        return;
      }
#endif // !BW_CHANGES
      continue;
    }
    if (nu < nv) {
      while (nu--)
        cuckoo[us[nu+1]] = us[nu];
      cuckoo[u0] = v0;
    } else {
      while (nv--)
        cuckoo[vs[nv+1]] = vs[nv];
      cuckoo[v0] = u0;
    }
  }
}

#if !BW_CHANGES
int main(int argc, char **argv) {
  const char *header = "";
  int c, easipct = 50;
  while ((c = getopt (argc, argv, "e:h:")) != -1) {
    switch (c) {
      case 'e':
        easipct = atoi(optarg);
        break;
      case 'h':
        header = optarg;
        break;
    }
  }
  assert(easipct >= 0 && easipct <= 100);
  printf("Looking for %d-cycle on cuckoo%d(\"%s\") with %d%% edges\n",
               PROOFSIZE, SIZESHIFT, header, easipct);
  u64 easiness = easipct * SIZE / 100;
  cuckoo_ctx ctx(header, easiness);
  worker(&ctx);
}
#endif // !BW_CHANGES

} // end namespace Cuckoo

BW_BEGIN_NAMESPACE


// -----------------------------------------------------------------------------
// Section: CuckooCycleLoginChallenge
// -----------------------------------------------------------------------------

class CuckooCycleLoginChallenge : public LoginChallenge
{
public:
	CuckooCycleLoginChallenge( uint64 maxNonce );

	bool writeChallengeToStream( BinaryOStream & data ) /* override */;
	bool readChallengeFromStream( BinaryIStream & data ) /* override */;
	bool writeResponseToStream( BinaryOStream & data ) /* override */;
	bool readResponseFromStream( BinaryIStream & data ) /* override */;

private:
	BW::string prefix_;
	uint64 maxNonce_;
};


/**
 *	Constructor.
 *
 *	@param maxNonce 	The maximum nonce.
 */
CuckooCycleLoginChallenge::CuckooCycleLoginChallenge( uint64 maxNonce ) :
		LoginChallenge(),
		prefix_(),
		maxNonce_( maxNonce )
{
	uint64 prefixValue;

	BWOpenSSL::RAND_bytes( reinterpret_cast< unsigned char * >( &prefixValue ),
		sizeof( prefixValue ) );

	BW::ostringstream stream;
	stream.fill( '0' );
	stream.width( sizeof( prefixValue ) / 8 * 2 );
	stream << std::hex << std::right << prefixValue << ":";

	prefix_ = stream.str();
}


/*
 *	Override from LoginChallenge.
 */
bool CuckooCycleLoginChallenge::writeChallengeToStream(
		BinaryOStream & data ) /* override */
{
	data << prefix_ << maxNonce_;

	return true;
}


/*
 *	Override from LoginChallenge.
 */
bool CuckooCycleLoginChallenge::readChallengeFromStream(
		BinaryIStream & data ) /* override */
{
	data >> prefix_ >> maxNonce_;

	if (data.error())
	{
		ERROR_MSG( "CuckooCycleLoginChallenge::readChallengeFromStream: "
			"Failed to de-stream parameters\n" );
		return false;
	}

	DEBUG_MSG( "CuckooCycleLoginChallenge::readChallengeToStream: "
			"Prefix: \"%s\", easiness = %.0f\n",
		prefix_.c_str(), double( maxNonce_ ) * 100.0 / SIZE );

	return true;
}


/*
 *	Override from LoginChallenge.
 */
bool CuckooCycleLoginChallenge::writeResponseToStream(
		BinaryOStream & data ) /* override */
{
	size_t i = 0;
	Cuckoo::Solution solution;
	solution.reserve( PROOFSIZE );
	BW::string key;

	while (solution.empty())
	{
		BW::ostringstream keyStream;
		keyStream << prefix_ << i;
		key = keyStream.str();

		Cuckoo::cuckoo_ctx c( key.c_str(),
			static_cast< Cuckoo::nonce_t >( maxNonce_ ) );
		Cuckoo::worker( &c, solution );

		// TODO: Should we have a maximum time limit or iteration limit here?
		// And if so, what should we do - request a new challenge?

		++i;
	}

	data << key;

	MF_ASSERT( solution.size() == PROOFSIZE );

	for (size_t i = 0; i < solution.size(); ++i)
	{
		data << Cuckoo::nonce_t( solution[i] );
	}

	DEBUG_MSG( "CuckooCycleLoginChallenge::writeResponseToStream: "
			"Solution found after %" PRIzu " iterations\n",
		i );

	return true;
}


/*
 *	Override from LoginChallenge.
 */
bool CuckooCycleLoginChallenge::readResponseFromStream(
		BinaryIStream & data ) /* override */
{
	BW::string key;
	data >> key;

	if (data.error())
	{
		NOTICE_MSG( "CuckooCycleLoginChallenge::readResponseFromStream: "
				"Could not de-stream key\n" );
		return false;
	}

	if (key.find( prefix_ ) != 0)
	{
		NOTICE_MSG( "CuckooCycleLoginChallenge::readResponseFromStream: "
				"Invalid key\n" );
		return false;
	}

	if (data.remainingLength() != PROOFSIZE * sizeof( Cuckoo::nonce_t ))
	{
		NOTICE_MSG( "CuckooCycleLoginChallenge::readResponseFromStream: "
				"Invalid response data\n" );
		return false;
	}

	Cuckoo::nonce_t * solutionArray =
		reinterpret_cast< Cuckoo::nonce_t * >(
			const_cast< void * >( 
				data.retrieve( PROOFSIZE * sizeof( Cuckoo::nonce_t ) ) ) );

	if (!Cuckoo::verify( solutionArray, key.c_str(),
			static_cast< Cuckoo::nonce_t >( maxNonce_ ) ))
	{
		NOTICE_MSG( "CuckooCycleLoginChallenge::readResponseToStream: "
			"Verification failed\n" );
		return false;
	}

	TRACE_MSG( "CuckooCycleLoginChallenge::readResponseToStream: "
			"Key: \"%s\" and cycle verified\n",
		key.c_str() );

	return true;
}


// -----------------------------------------------------------------------------
// Section: CuckooCycleLoginChallengeFactory
// -----------------------------------------------------------------------------


const double CuckooCycleLoginChallengeFactory::DEFAULT_EASINESS = 50.0;


/**  
 *	Constructor.
 */
CuckooCycleLoginChallengeFactory::CuckooCycleLoginChallengeFactory() :
		LoginChallengeFactory(),
		easiness_( DEFAULT_EASINESS )
{
}


/*
 *	Override from LoginChallengeFactory.
 */
bool CuckooCycleLoginChallengeFactory::configure(
		const LoginChallengeConfig & config ) /* override */
{
	easiness_ = config.getDouble( "easiness", easiness_ );

	if ((easiness_ <= 0) || (easiness_ > 100.0))
	{
		ERROR_MSG( "CuckooCycleLoginChallengeFactory::configure: "
			"Invalid easiness value\n" );
		return false;
	}

	INFO_MSG( "CuckooCycleLoginChallengeFactory::configure: Easiness: %.03f\n",
		easiness_ );

	return true;
}


/*
 *	Override from LoginChallengeFactory.
 */
LoginChallengePtr CuckooCycleLoginChallengeFactory::create() /* override */
{
	return new CuckooCycleLoginChallenge( uint64( easiness_ * SIZE / 100.0 ) );
}


#if ENABLE_WATCHERS
/*
 *	Override from LoginChallengeFactory.
 */
WatcherPtr CuckooCycleLoginChallengeFactory::pWatcher() /* override */
{
	WatcherPtr pWatcher = new DirectoryWatcher();

	CuckooCycleLoginChallengeFactory * pNull = NULL;

	pWatcher->addChild( "easiness",
		makeNonRefWatcher( *pNull,
			&CuckooCycleLoginChallengeFactory::easiness,
			&CuckooCycleLoginChallengeFactory::easiness ) );

	return pWatcher;
}
#endif // ENABLE_WATCHERS


BW_END_NAMESPACE


// cuckoo_cycle_login_challenge_factory.cpp

