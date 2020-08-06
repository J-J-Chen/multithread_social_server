#include <iostream>
#include <queue>
#include <string>
#include <cstdlib>

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;

typedef boost::shared_ptr<tcp::socket> socket_ptr;
typedef boost::shared_ptr<string> string_ptr;
typedef boost::shared_ptr<queue<string_ptr>> message_queue_ptr;

io_service service;
message_queue_ptr messsageQueue(new queue<string_ptr>);
tcp::endpoint ep(ip::adddress::from_string("127.0.0.1"), 8001);
const int inputSize = 256;
string_ptr console_prompt;

bool isOwnMessage(string_ptr);
void displayLoop(socket_ptr);
void inboundLoop(socket_ptr, string_ptr);
void writeLoop(socket_ptr, string_ptr);
string* buildPrompt();


