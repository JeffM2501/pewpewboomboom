#include "chat_filter_processor.h"
#include <vector>

bool ChatFilterProcessor::FilterMessage(PendingChatMessage& message)
{
    std::vector<std::string> badWords;
    badWords.push_back("badword");
    badWords.push_back("hack");
    badWords.push_back("cheat");
    bool found = false;
    for (const auto& word : badWords)
    {
        size_t pos = 0;
        while ((pos = message.Message.find(word, pos)) != std::string::npos)
        {
            message.Message.replace(pos, word.length(), std::string(word.length(), '*'));
            pos += word.length();
            found = true;
        }
    }
    if (found)
    {
        message.WasFiltered = true;
    }
    return found;
}

ChatFilterProcessor::ChatFilterProcessor()
{
    WorkerThread = std::thread(&ChatFilterProcessor::WorkerLoop, this);
}

ChatFilterProcessor::~ChatFilterProcessor()
{
    Flush();
    {
        std::lock_guard<std::mutex> lock(QueueMutex);
        StopThread = true;
    }
    QueueCondition.notify_all();
    if (WorkerThread.joinable())
    {
        WorkerThread.join();
    }
}

void ChatFilterProcessor::Flush()
{
    std::unique_lock<std::mutex> lock(QueueMutex);
    FlushCondition.wait(lock, [this]()
    {
        return ActiveMessages == 0;
    });
}

void ChatFilterProcessor::PushChatMessage(uint64_t senderId, std::string_view message)
{
    {
        std::lock_guard<std::mutex> lock(QueueMutex);
        PendingChatMessage msg;
        msg.WasFiltered = false;
        msg.ShouldSend = true;
        msg.SenderID = senderId;
        msg.Message = std::string(message);

        PendingMessages.push_back(std::move(msg));
        ActiveMessages++;
    }
    QueueCondition.notify_one();
}

void ChatFilterProcessor::WorkerLoop()
{
    while (true)
    {
        PendingChatMessage message;
        bool hasMessage = false;

        {
            std::unique_lock<std::mutex> lock(QueueMutex);
            QueueCondition.wait(lock, [this]()
            {
                return StopThread || !PendingMessages.empty();
            });

            if (StopThread && PendingMessages.empty())
            {
                break;
            }

            if (!PendingMessages.empty())
            {
                message = std::move(PendingMessages.front());
                PendingMessages.pop_front();
                hasMessage = true;
            }
        }

        if (hasMessage)
        {
            if (FilterMessage(message))
                MessageWasFiltered.Invoke(message);

            if (message.ShouldSend)
                SentFilteredMessage.Invoke(message);

            {
                std::lock_guard<std::mutex> lock(QueueMutex);
                ActiveMessages--;
                if (ActiveMessages == 0)
                {
                    FlushCondition.notify_all();
                }
            }
        }
    }
}
