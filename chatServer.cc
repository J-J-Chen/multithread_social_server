#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <cstdlib>

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

using namespace boost::asio;
using namespace boost::asio::ip;

typedef boost::shared_ptr<tcp::socket> socket_ptr;
typedef boost::shared_ptr<std::string> string_ptr;
typedef std::map<socket_ptr, string_ptr> clientMap;
typedef boost::shared_ptr<clientMap> clientMap_ptr;
typedef boost::shared_ptr<std::list<socket_ptr>> clientList_ptr;
typedef boost::shared_ptr<std::queue<clientMap_ptr>> message_queue_ptr;

io_service service;
tcp::acceptor acceptor(service, tcp::endpoint(tcp::v4(), 8001));
boost::mutex mu;
clientList_ptr clientList(new std::list<socket_ptr>);
message_queue_ptr messageQueue(new std::queue<clientMap_ptr>);

const int bufSize = 1024;
enum sleepLen {
  SMALL = 100,
  LONG = 200
};

bool clientSentExit(string_ptr);
void disconnectClient(socket_ptr);
void acceptorLoop();
void requestLoop();
void responseLoop();

int main(int argc, char** argv) {
  boost::thread_group threads;

  threads.create_thread(boost::bind(acceptorLoop));
  boost::this_thread::sleep(boost::posix_time::millisec(sleepLen::SMALL));

  threads.create_thread(boost::bind(requestLoop));
  boost::this_thread::sleep(boost::posix_time::millisec(sleepLen::SMALL));

  threads.create_thread(boost::bind(responseLoop));
  boost::this_thread::sleep(boost::posix_time::millisec(sleepLen::SMALL));

  threads.join_all();

  std::puts("Press any key to continue...");
  std::getc(stdin);

  return EXIT_SUCCESS;
}

// Checks for new clients that want to connect
void acceptorLoop() {
  std::cout << "Waiting for clients to connect...\n";

  while(true) {
    socket_ptr clientSock(new tcp::socket(service));
    acceptor.accept(*clientSock);

    std::cout << "New client joined!\n";

    mu.lock();
    clientList->emplace_back(clientSock);
    mu.unlock();

    std::cout << "There are a total of " << clientList->size() <<
      "clients conneted.\n";
  }
}

// Check for new messages
void requestLoop() {
  while(true) {
    if(!clientList->empty()) {
      mu.lock();
      for(auto& clientSock : *clientList) {
        if(clientSock->available()) {
          char readBuf[bufSize] = {0};
          int bytesRead = clientSock->read_some(buffer(readBuf, bufSize));
          string_ptr msg(new std::string(readBuf, bytesRead));

          if(clientSentExit(msg)) {
            disconnectClient(clientSock);
            break;
          }

          clientMap_ptr cm(new clientMap);
          cm->insert(std::pair<socket_ptr, string_ptr>(clientSock, msg));

          messageQueue->push(cm);

          std::cout << "Chat: " << *msg << std::endl;
        }
      }
      mu.unlock();
    }
    boost::this_thread::sleep(boost::posix_time::millisec(sleepLen::LONG));
  }
}

bool clientSentExit(string_ptr message) {
  return *message == "!exit";
}

void disconnectClient(socket_ptr sock) {
  auto position = find(clientList->begin(), clientList->end(), sock);

  sock->shutdown(tcp::socket::shutdown_both);
  sock->close();

  clientList->erase(position);

  std::cout << "Client Disconnected! There are now " << clientList->size() <<
    " total clients connected." << std::endl;
}

void responseLoop() {
  while(true) {
    if(!messageQueue->empty()) {
      auto message = messageQueue->front();

      mu.lock();
      for(auto& sock : *clientList) {
        sock->write_some(buffer(*(message->begin()->second), bufSize));
      }

      messageQueue->pop();
      mu.unlock();
    }
    boost::this_thread::sleep(boost::posix_time::millisec(sleepLen::LONG));
  }
}

