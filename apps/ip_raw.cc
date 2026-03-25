#include "socket.hh"
#include <iostream>
#include <span>
#include <string_view>

using namespace std;

class RawSocket : public DatagramSocket
{
public:
  RawSocket() : DatagramSocket( AF_INET, SOCK_RAW, IPPROTO_RAW ) {}
  RawSocket( int protocol ) : DatagramSocket( AF_INET, SOCK_RAW, protocol ) {}
};
class Example
{
public:
  static void sendIP_Protocol5()
  {
    RawSocket socket( 5 );
    Address localHost( "127.0.0.0" );
    socket.connect( localHost );
    socket.send( "Hello!" );
    cout << "already send";
    socket.shutdown( SHUT_WR );
    socket.close();
    return;
  }
  static void sendIP_Protocol17()
  {
    UDPSocket socket;
    Address localhost( "127.0.0.1", 8080 );
    socket.connect( localhost );

    string msg;
    while ( msg != "quit_client" ) {
      cin >> msg;
      socket.send( msg );
    }

    // cout << "already send: Hello\n";
    // socket.shutdown(SHUT_WR);
    socket.close();
    return;
  }
  static void reciveIP_Protocol17()
  {
    UDPSocket socket;
    Address localhost( "127.0.0.1", 8080 );
    socket.bind( localhost );

    string payload;
    while ( payload != "quit_server" ) {
      // cout << "loop\n";
      payload.clear();
      socket.recv( localhost, payload );
      cout << "already recieve: " << payload << endl;
    }
    // socket.shutdown(SHUT_RD);
    socket.close();
    return;
  }
};

int main( int argc, char** argv )
{

  // construct an Internet or user datagram here, and send using the RawSocket as in the Jan. 10 lecture
  auto args = span( argv, argc );

  const string mode { args[1] };

  if ( mode == "client" ) {
    Example::sendIP_Protocol17();
  } else {
    Example::reciveIP_Protocol17();
  }
  return 0;
}
