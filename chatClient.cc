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
typedef boost::shared_ptr<std::string> string_ptr;
typedef boost::shared_ptr<std::queue<string_ptr>> message_queue_ptr;

io_service service;
message_queue_ptr messageQueue(new std::queue<string_ptr>);
tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8001);
const int inputSize = 256;
string_ptr console_prompt;

bool isOwnMessage(string_ptr);
void displayLoop(socket_ptr);
void inboundLoop(socket_ptr, string_ptr);
void writeLoop(socket_ptr, string_ptr);
std::string* buildPrompt();

int main(int argc, char** argv) {
  try {
    boost::thread_group threads;
    socket_ptr sock(new tcp::socket(service));
    string_ptr prompt(buildPrompt());
    console_prompt = prompt;

    sock->connect(ep);

    std::cout << "Welcome to the chat! \n Type [!exit] to quit. Have fun \n";

    threads.create_thread(boost::bind(displayLoop, sock));
    threads.create_thread(boost::bind(inboundLoop, sock, prompt));
    threads.create_thread(boost::bind(writeLoop, sock, prompt));

    threads.join_all();
  } catch(std::exception e) {
    std::cerr << e.what() << std::endl;
  }

  std::puts("Press any key to continue");
  std::getc(stdin);
  return EXIT_SUCCESS;
}

// Handles display of terminal inputs
std::string* buildPrompt() {
  char inputBuf[inputSize] = {0};
  char nameBuf[inputSize] = {0};
  std::string* prompt = new std::string(": ");
  std::cout << "Please choose a new username: ";
  std::cin.getline(nameBuf, inputSize);
  *prompt = (std::string)nameBuf + *prompt;
  boost::algorithm::to_lower(*prompt);

  return prompt;
}

// Constantly checks socket for new messages
void inboundLoop(socket_ptr sock, string_ptr prompt) {
  int bytesRead = 0;
  char readBuf[1024] = {0};

  while(true) {
    if(sock->available()) {
      bytesRead = sock->read_some(buffer(readBuf, inputSize));
      string_ptr msg(new std::string(readBuf, bytesRead));

      messageQueue->push(msg);
    }

    // Delay to allow time to write to socket
    boost::this_thread::sleep(boost::posix_time::millisec(1000));
  }
}

// Accepts user input and sends it to the socket
void writeLoop(socket_ptr sock, string_ptr prompt) {
  char inputBuf[inputSize] = {0};
  std::string inputMsg;

  while(true) {
    std::cin.getline(inputBuf, inputSize);
    inputMsg = *prompt + (std::string)inputBuf + "\n";

    if(!inputMsg.empty()) {
      sock->write_some(buffer(inputBuf, inputSize));
    }

    if(inputMsg.find("!exit") != std::string::npos) {
      // TODO: Better clean up and error checking
      exit(1);
    }

    inputMsg.clear();
    memset(inputBuf, 0, inputSize);
  }
}

// Displays content for client
void displayLoop(socket_ptr sock) {
  while(true) {
    if(!messageQueue->empty()) {
      // TODO: Add better multi-user support
      if(!isOwnMessage(messageQueue->front())) {
        std::cout << "\n" + *(messageQueue->front());
      }
      messageQueue->pop();
    }

    // Wait to allow for write to socket
    boost::this_thread::sleep(boost::posix_time::millisec(10000));
  }
}

bool isOwnMessage(string_ptr message) {
  return (message->find(*console_prompt) != std::string::npos);
}

