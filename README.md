
# NO proxy

./server 5678 2>/dev/null &
./client 127.0.0.1 5678 files/elquijote.txt 100 2>/dev/null

# WITH proxy 

./server 5678 2>/dev/null &
./proxy.pl 5679 127.0.0.1 5678 &
./proxy.pl 5679 127.0.0.1 5678 &
./client 127.0.0.1 5670 files/elquijote.txt 100 2>/dev/null 

# WITH SSL proxy

./server 5678 2>/dev/null &
./proxy.pl 5679 127.0.0.1 5678 --local-ssl 2>/dev/null 2>&1 &
./proxy.pl 5670 127.0.0.1 5679 --remote-ssl 2>/dev/null 2>&1 &
./client 127.0.0.1 5670 files/elquijote.txt 100 2>/dev/null

