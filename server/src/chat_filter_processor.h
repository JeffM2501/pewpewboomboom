#pragma once

#include "enet.h"
#include <deque>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "event_source.h"

struct PendingChatMessage
{
  bool WasFiltered = false;
  bool ShouldSend = true;

  uint64_t SenderID = 0;
  std::string Message;
};

class ChatFilterProcessor
{
private:
  std::deque<PendingChatMessage> PendingMessages;

  std::thread WorkerThread;
  std::mutex QueueMutex;
  std::condition_variable QueueCondition;
  std::condition_variable FlushCondition;
  int ActiveMessages = 0;
  bool StopThread = false;

  bool FilterMessage(PendingChatMessage &message);
  void WorkerLoop();

public:
  EventSource<PendingChatMessage> MessageWasFiltered;
  EventSource<PendingChatMessage> SentFilteredMessage;

  ChatFilterProcessor();
  ~ChatFilterProcessor();

  void Flush();

  void PushChatMessage(uint64_t senderId, std::string_view message);
};