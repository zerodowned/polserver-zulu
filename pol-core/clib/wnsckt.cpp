/*
History
=======

Notes
=======

*/

#include "stl_inc.h"

#include "strutil.h"

#include <errno.h>
#include <stdio.h>

#ifdef _WIN32
#	include <windows.h>
#	include <winsock.h>
#	define SOCKET_ERRNO(x) WSA##x
#	define socket_errno WSAGetLastError()
	typedef int socklen_t;

#else

#	include <netinet/in.h>
#	include <netinet/tcp.h>
#	include <arpa/inet.h>
#	include <sys/socket.h>
#	include <sys/time.h>
#	include <sys/types.h>
#	include <netdb.h>
#	include <sys/time.h>
#	include <unistd.h>
#	include <fcntl.h>
#	include <errno.h>
#	define SOCKET_ERRNO(x) x
#	define socket_errno errno

	typedef int SOCKET;
#	ifndef INVALID_SOCKET
#		define INVALID_SOCKET (SOCKET)(-1)
#	endif

#endif

#include "esignal.h"
#include "wnsckt.h"

#ifndef SCK_WATCH
#define SCK_WATCH 0
#endif

Socket::Socket() :
	_sck(-1),
	_options(none)
{
#ifdef _WIN32
	static bool init;
	if (!init)
	{
		init = true;
		WSADATA dummy;
		WSAStartup( MAKEWORD( 1, 0 ), &dummy );
	}
#endif
}

Socket::Socket(int sock) :
	_sck(sock),
	_options(none)
{
}

Socket::Socket( Socket& sock ) :
	_sck( sock._sck ),
	_options(none)
{
	sock._sck = -1;
}


Socket::~Socket()
{
	close();
}

void Socket::setsocket( int sock )
{
	close();
	_sck = sock;
}

void Socket::set_options( option opt )
{
	_options = opt;
}

void Socket::setpeer( struct sockaddr peer )
{
	_peer = peer;
}

string Socket::getpeername() const
{
	struct sockaddr client_addr; // inet_addr
	socklen_t addrlen = sizeof client_addr;
	if (::getpeername( _sck, &client_addr, &addrlen ) == 0)
	{
		struct sockaddr_in *in_addr = (struct sockaddr_in *) &client_addr;
		if (client_addr.sa_family == AF_INET)
			return inet_ntoa( in_addr->sin_addr );
		else
			return "(display error)";
	}
	else
		return "Error retrieving peer name";
}
struct sockaddr Socket::peer_address() const
{
	return _peer;
}

int Socket::handle() const
{
	return _sck;
}

int Socket::release_handle()
{
	int s = _sck;
	_sck = -1;
	return s;
}


bool Socket::open( const char* ipaddr, unsigned short port )
{
	close();
	_sck = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

	struct sockaddr_in sin;
	memset( &sin, 0, sizeof sin );
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr( ipaddr );
	sin.sin_port = htons( port );


	int res = connect( _sck, (struct sockaddr *)&sin, sizeof sin );

	if (res == 0)
	{
		return true;
	}
	else
	{
#ifdef _WIN32
		closesocket( _sck );
#else
		::close( _sck );
#endif
		_sck = -1;
		return false;
	}
}

void Socket::apply_socket_options( int sck )
{
	if (_options & nonblocking)
	{
#ifdef _WIN32
		u_long nonblocking = 1;
		int res = ioctlsocket( sck, FIONBIO, &nonblocking );
#else
		int flags= fcntl( sck, F_GETFL );
		flags |= O_NONBLOCK;
		int res = fcntl( sck, F_SETFL, O_NONBLOCK );
#endif
		if (res < 0)
		{
			throw runtime_error( "Unable to set listening socket to nonblocking mode, res=" + decint(res) );
		}
	}
}

void Socket::apply_prebind_socket_options( int sck )
{
	if (_options & reuseaddr)
	{
#ifndef WIN32
		int reuse_opt = 1;
		int res = setsockopt( sck, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuse_opt, sizeof(reuse_opt) );
		if (res < 0)
		{
			throw runtime_error( "Unable to setsockopt (SO_REUSEADDR) on listening socket, res = " + decint(res) );
		}
#endif
	}
}

bool Socket::listen(unsigned short port)
{
	struct sockaddr_in local; 

	close();
	local.sin_family = AF_INET; 
	local.sin_addr.s_addr = INADDR_ANY;  
	/* Port MUST be in Network Byte Order */ 
	local.sin_port = htons(port); 

	_sck = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if (_sck == -1)
	{
		close();
		return false;
	}

	apply_socket_options( _sck );
	apply_prebind_socket_options( _sck );

	if (bind(_sck,(struct sockaddr*)&local, sizeof(local) ) == -1)
	{
		HandleError();
		return false;
	}
	if( ::listen( _sck, SOMAXCONN ) == -1 ) 
	{ 
		HandleError();
		return false; 
	} 
	return true;
}

bool Socket::select( unsigned long seconds, unsigned long useconds )
{
	fd_set fd;
	struct timeval timeout = { 0, 0 };
	int nfds = 0;
	FD_ZERO( &fd );
	FD_SET( _sck, &fd );
#ifndef _WIN32
	nfds = _sck+1;
#endif

	int res;
	
	do
	{
		timeout.tv_sec = seconds;
		timeout.tv_usec = useconds;
		res = ::select( nfds, &fd, NULL, NULL, &timeout );
	} while (res < 0 && !exit_signalled && socket_errno == SOCKET_ERRNO(EINTR));

	return (res > 0 && FD_ISSET( _sck, &fd ));
}

bool Socket::accept(int *s, unsigned long mstimeout)
{
	*s = ::accept(_sck, NULL, NULL);
	if (*s >= 0)
	{
		apply_socket_options( *s );
		return true;
	}
	else
	{
		*s=-1;
		return false;
	}
}

bool Socket::accept( Socket& newsocket )
{
	struct sockaddr client_addr;
	socklen_t addrlen = sizeof client_addr;
	int s = ::accept( _sck, &client_addr, &addrlen );
	if (s >= 0)
	{
		apply_socket_options( s );
		newsocket.setsocket( s );
		newsocket.setpeer( client_addr );
		return true;
	}
	else
	{
		return false;
	}
}

bool Socket::connected() const
{
	return (_sck != -1);
}

/* Read and clear the error value */
void Socket::HandleError()
{
#ifdef _WIN32
	int ErrVal;
	static char ErrorBuffer[80];

	ErrVal = WSAGetLastError();
	WSASetLastError(0);

	switch(ErrVal)
	{
		case WSAENOTSOCK: /* Software caused connection to abort */
		case WSAECONNRESET: /* Arg list too long */
			close();
			break;
		
		default:
			close();	/*
							gee, we'll close here,too.
							if you want to _not_ close for _specific_ error codes,
							feel more than free to make exceptions; but the general
							rule should be, close on error.
						*/
			break;
	}

#if SCK_WATCH
	if (FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
					   GetModuleHandle(TEXT("wsock32")),
					   ErrVal,
					   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					   ErrorBuffer,
					   sizeof ErrorBuffer,
					   NULL) == 0)
	{
		sprintf(ErrorBuffer, "Unknown error code 0x%08x", ErrVal);
	}
	printf("%s\n",ErrorBuffer);
#endif
#else
	close();
#endif
}

bool Socket::recvbyte( unsigned char* ch, unsigned long waitms )
{
	fd_set fd;

#if SCK_WATCH
	printf( "{L;1}" );
#endif	
	FD_ZERO( &fd );
	FD_SET( _sck, &fd );
	int nfds = 0;
#ifndef _WIN32
	nfds = _sck + 1;
#endif
	struct timeval tv;
	int res;
	
	do
	{
		tv.tv_sec = 0;
		tv.tv_usec = waitms * 1000;
		res = ::select( nfds, &fd, NULL, NULL, &tv );
	} while (res < 0 && exit_signalled && socket_errno == SOCKET_ERRNO(EINTR));

	if (res == 0 )
	{
#if SCK_WATCH
		printf( "{TO}" );
#endif
		return false;
	}
	else if (res == -1)
	{
		HandleError();
		close(); /* FIXME: very likely unrecoverable */
	}

	res = recv( _sck, (char *) ch, 1, 0 );
	if (res == 1)
	{
#if SCK_WATCH
		printf( "{%02.02x}", ch );
#endif
		return true;
	}
	else if (res == 0)
	{
#if SCK_WATCH
		printf( "{CLOSE}" );
#endif
		close();
		return false;
	}
	else 
	{
		/* Can't time out here this is an ERROR! */
		HandleError();
		return false;
	}

}

bool Socket::recvdata( void *vdest, unsigned len, unsigned long waitms )
{
	fd_set fd;
	char* pdest = (char*) vdest;
	while (len)
	{
#if SCK_WATCH
		printf( "{L:%d}", len );
#endif		
		FD_ZERO( &fd );
		FD_SET( _sck, &fd );
		struct timeval tv;
		int res;
		do
		{
			tv.tv_sec = 0;
			tv.tv_usec = waitms * 1000;
			res = ::select( 0, &fd, NULL, NULL, &tv );
		} while (res < 0 && exit_signalled && socket_errno == SOCKET_ERRNO(EINTR));
		
		if (res == 0)
		{
#if SCK_WATCH
			printf( "{TO}" );
#endif
			return false;
		}
		else if (res == -1)
		{
			HandleError();
			close(); /* FIXME: very likely unrecoverable */
		}


		res = ::recv( _sck, pdest, len, 0 );
		if (res > 0)
		{

#if SCK_WATCH
			char* tmp = pdest;
			printf( "{R:%d[%d]}", res,len );
#endif
			len -= res;
			pdest += res;

#if SCK_WATCH
			while (res--)
				printf( "{%02.02x}", (unsigned char)(*tmp++) );
#endif
		}
		else if (res == 0)
		{
#if SCK_WATCH
			printf( "{CLOSE}" );
#endif
			close();
			return false;
		}
		else 
		{
			/* Can't time out here this is an ERROR! */
			HandleError();
			return false;
		}
	}
	return true;
}

unsigned Socket::peek( void *vdest, unsigned len, unsigned long wait_sec )
{
	fd_set fd;
	char* pdest = (char*) vdest;

#if SCK_WATCH
		printf( "{L:%d}", len );
#endif		
	FD_ZERO( &fd );
	FD_SET( _sck, &fd );
	struct timeval tv;
	int res;
	
	do
	{
		tv.tv_sec = wait_sec;
		tv.tv_usec = 0;
		res = ::select( 0, &fd, NULL, NULL, &tv );
	} while (res < 0 && exit_signalled && socket_errno == SOCKET_ERRNO(EINTR));

	if (res == 0)
	{
#if SCK_WATCH
		printf( "{TO}" );
#endif
		return 0;
	}
	else if (res == -1)
	{
		HandleError();
		close(); /* FIXME: very likely unrecoverable */
		return 0;
	}


	res = ::recv( _sck, pdest, len, MSG_PEEK );
	if (res > 0)
	{
		return res;
	}
	else if (res == 0)
	{
#if SCK_WATCH
		printf( "{CLOSE}" );
#endif
		close();
		return 0;
	}
	else 
	{
		/* Can't time out here this is an ERROR! */
		HandleError();
		return 0;
	}
}

void Socket::send( const void* vdata, unsigned datalen )
{
	const char* cdata = static_cast<const char*>(vdata);
		
	while (datalen)
	{
		int res = ::send( _sck, cdata, datalen, 0 );
		if (res < 0)
		{
			int sckerr = socket_errno;
			if (sckerr == SOCKET_ERRNO(EWOULDBLOCK))
			{
				// FIXME sleep
				continue;
			}
			else
			{
				
				cout << "Socket::send() error: " << sckerr << endl;
				HandleError();
				return;
			}
		}
		else
		{
			datalen -= res;
			cdata += res;
		}
	}
}

bool Socket::send_nowait( const void* vdata, unsigned datalen, unsigned* nsent )
{
	const char* cdata = static_cast<const char*>(vdata);

	*nsent = 0;
		
	while (datalen)
	{
		int res = ::send( _sck, cdata, datalen, 0 );
		if (res < 0)
		{
			int sckerr = socket_errno;
			if (sckerr == SOCKET_ERRNO(EWOULDBLOCK))
			{
				// FIXME sleep
				return false;
			}
			else
			{				
				cout << "Socket::send_nowait() error: " << sckerr << endl;
				HandleError();
				return true;
			}
		}
		else
		{
			datalen -= res;
			cdata += res;
			*nsent += res;
		}
	}
	return true;
}

void Socket::write( const string& s )
{
	send( (void *) s.c_str(), s.length() );
}


void Socket::close()
{
	if (_sck != INVALID_SOCKET)//-1)
	{
#ifdef _WIN32
		shutdown( _sck,2 ); //2 is both sides. defined in winsock2.h ...
		closesocket( _sck );
#else
		shutdown( _sck,SHUT_RDWR );
		::close( _sck );
#endif
		_sck = INVALID_SOCKET;//-1;
	}
}

bool Socket::is_local()
{
	string s= getpeername();
	return (s == "127.0.0.1");
}
