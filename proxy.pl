#!/usr/bin/perl
use IO::Socket::Forwarder qw(forward_sockets);
use IO::Socket::INET;
use IO::Socket::SSL;
use Getopt::Long;

my $local_ssl_enabled = 0;
my $remote_ssl_enabled = 0;
my $proxy_buffer_size = 64 * 1024;
my $proxy_chunk_size = 16 * 1024;

GetOptions (
  "buffer-size=i" => \$proxy_buffer_size,
  "chunck-size=i"   => \$proxy_chunk_size,
  "remote-ssl"  => \$remote_ssl_enabled,
  "local-ssl"  => \$local_ssl_enabled,
);

my $local_port = shift;
my $remote_address = shift;
my $remote_port = shift;

# Socket connected to remote application
my $remote_socket = undef;
my $remote_host = "$remote_address:$remote_port";
if($remote_ssl_enabled) {
  $remote_socket = IO::Socket::SSL->new(
    PeerAddr => $remote_host,
    Blocking => 0,
    SSL_verify_mode => SSL_VERIFY_NONE,
  ) or die "failed connect or ssl handshake: $!,$SSL_ERROR";;
} else {
  $remote_socket = IO::Socket::INET->new(
    PeerAddr => $remote_host,
    Blocking => 0,
  );
}
defined($remote_socket) or die "Unable to connect to $remote_host";

# Socket listening to local application
my $local_socket = undef;
my $new_socket = undef;
if($local_ssl_enabled) {
  $local_socket = IO::Socket::SSL->new(
    LocalPort => $local_port,
    LocalAddr => '0.0.0.0',
    Listen    => 1,
    SSL_cert_file => '/etc/qvd/certs/cert.pem',
    SSL_key_file => '/etc/qvd/certs/key.pem',
  ) or die "failed to listen: $!";
  $new_socket = $local_socket->accept or die
        "failed to accept or ssl handshake: $!,$SSL_ERROR";
} else {
  $local_socket = IO::Socket::INET->new(
    LocalPort => $local_port,
    LocalAddr => '0.0.0.0',
    Listen    => 1,
    Reuse => 1,
  );
  $new_socket = $local_socket->accept() or die "Connection failed";
}

# Forward sockets
forward_sockets(
  $new_socket,
  $remote_socket,
  io_buffer_size => $proxy_buffer_size,
  io_chunk_size => $proxy_chunk_size
);

