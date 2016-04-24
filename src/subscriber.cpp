/** @file    subscriber.cpp 
 *  @author  Pranav Srinivas Kumar
 *  @date    2016.04.24
 *  @brief   This file contains definitions for the Subscriber class
 */

#include "subscriber.hpp"

namespace zcm {

  Subscriber::Subscriber(std::string name, 
			 unsigned int priority, 
			 std::string filter,
			 std::vector<std::string> endpoints, 
			 std::function<void(const std::string&)> operation_function, 
			 Operation_Queue * operation_queue_ptr) : 
    name(name),
    priority(priority),
    filter(filter),
    endpoints(endpoints),
    operation_function(operation_function),
    operation_queue_ptr(operation_queue_ptr) {
    context = new zmq::context_t(2);
    subscriber_socket = new zmq::socket_t(*context, ZMQ_SUB);
    for (auto endpoint : endpoints)
      subscriber_socket->connect(endpoint);
    subscriber_socket->setsockopt(ZMQ_SUBSCRIBE, filter.c_str(), filter.length());
  }

  Subscriber:: ~Subscriber() {
    subscriber_socket->close();
    delete context;
    delete subscriber_socket;
  }

  void Subscriber::connect(std::vector<std::string> new_endpoints) {
    endpoints = new_endpoints;
    context = new zmq::context_t(2);
    subscriber_socket = new zmq::socket_t(*context, ZMQ_SUB);
    for (auto endpoint : endpoints)
      subscriber_socket->connect(endpoint);
    subscriber_socket->setsockopt(ZMQ_SUBSCRIBE, filter.c_str(), filter.length());
  }

  std::string Subscriber::get_name() {
    return name;
  }

  unsigned int Subscriber::get_priority() {
    return priority;
  }

  void Subscriber::add_connection(std::string new_connection) {
    subscriber_socket->connect(new_connection);
  }

  void Subscriber::recv() {
    while(true) {
      zmq::message_t received_message; 
      subscriber_socket->recv(&received_message);
      std::string message = std::string(static_cast<char*>(received_message.data()), 
					received_message.size());
      if (message.length() > 0) {
	func_mutex.lock();      
	Subscriber_Operation * new_operation
	  = new Subscriber_Operation(name, priority, std::bind(operation_function, message));
	operation_queue_ptr->enqueue(new_operation);
	func_mutex.unlock();      
      }
    }
  }

  void Subscriber::rebind_operation_function(std::function<void(const std::string&)> 
					     new_operation_function) {
    func_mutex.lock();
    operation_function = new_operation_function;
    func_mutex.unlock();
  }

  std::thread Subscriber::spawn() {
    return std::thread(&Subscriber::recv, this);
  }

  void Subscriber::start() {
    std::thread subscriber_thread = spawn();
    subscriber_thread.detach();
  }

}
