./build.sh
rm output/index.html
rm output/server.cpp
./server &
SERVER_PID=$!
sleep 2 

echo "1) Real Web browser accessing your server"
echo "Please open a web browser and navigate to 'http://localhost:8888/server.cpp'"
read -p "Press enter to continue"

echo "2) Your retriever accessing a real server"
./retriever google.com/index.html
read -p "Press enter to continue"

echo "3) Your retriever accessing a file from your server"
./retriever 127.0.0.1:8888/server.cpp
read -p "Press enter to continue"

echo "Showing file output"
ls output/
read -p "Press enter to continue" 

echo "4) Your retriever requesting an unauthorized file from your server"
./retriever 127.0.0.1:8888/MySecret.html
read -p "Press enter to continue"

echo "5) Your retriever requesting a forbidden file from your server"
./retriever 127.0.0.1:8888/../forbidden.txt
read -p "Press enter to continue "

echo "6) Your retriever requesting a non-existent file from your server"
./retriever  127.0.0.1:8888/non_existent_file.txt
read -p "Press enter to continue "

echo "7) Your retriever sending a malformed request to your server"
echo "GE /server.cpp HTTP/1.1\r\n\r\n" | nc 127.0.0.1 8888
read -p "Press enter to continue "

kill -9 $SERVER_PID
