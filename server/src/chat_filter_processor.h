#pragma once

#include "enet.h"
#include <deque>
#include <string>


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

  bool FilterMessage(PendingChatMessage &message);

public:
  EventSource<PendingChatMessage> MessageWasFiltered;
  EventSource<PendingChatMessage> SentFilteredMessage;

  ChatFilterProcessor();
  ~ChatFilterProcessor();

  void Flush();

  void PushChatMessage(uint64_t senderId, std::string_view message);
};