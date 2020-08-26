#include "Queues.hpp"
int quit = 0;

PacketQueue::PacketQueue()
    : pkt_queue()
    , byte_size(0)
{
    mutex = SDL_CreateMutex();
    cond = SDL_CreateCond();
}

PacketQueue::~PacketQueue()
{
    SDL_DestroyCond(cond);
    SDL_DestroyMutex(mutex);
    // Nothing to do, std queue will call its desctructor
}

bool PacketQueue::push(AVPacket* pkt)
{
    if (byte_size >= MAX_QUEUE_SIZE) return false;

    SDL_LockMutex(mutex);
    pkt_queue.push(*pkt);
    byte_size += pkt->size;
    SDL_CondSignal(cond);
    SDL_UnlockMutex(mutex);

    return true;
}

bool PacketQueue::get(AVPacket* pkt, int block)
{
    int ret;

    SDL_LockMutex(mutex);

    for (;;) {

        if (quit) {
            ret = -1;
            break;
        }

        // if there are packets in the queue procced to get the next one
        //cout << "Size of queue = " << this->packet_queue.size() << endl;
        if (pkt_queue.size() > 0) {
            *pkt = pkt_queue.front();
            pkt_queue.pop();
            byte_size -= pkt->size;
            ret = 1;
            break;
        } // want to block?
        else if (!block) {
            ret = 0;
            break;
        } // blocking, this will release the mutex and wait for 'cond' to happen 
        else {
            SDL_CondWait(this->cond, this->mutex);
        }
    }
    SDL_UnlockMutex(this->mutex);
    return ret;
}

size_t PacketQueue::size()
{
    return byte_size;
}

int PacketQueue::count()
{
    return pkt_queue.size();
}

bool PacketQueue::full()
{
    return (byte_size >= MAX_QUEUE_SIZE);
}

bool PacketQueue::empty()
{
    return (pkt_queue.size() == 0);
}

FrameQueue::FrameQueue()
    : frame_queue()
{
    mutex = SDL_CreateMutex();
    cond = SDL_CreateCond();
}

FrameQueue::~FrameQueue()
{
    SDL_DestroyCond(cond);
    SDL_DestroyMutex(mutex);
    // Nothing to do, std queue will call its desctructor
}

bool FrameQueue::push(AVFrame** pkt) {
    if (this->frame_queue.size() >= 5) return false;

    AVFrame *pkt1 = *pkt;
    SDL_LockMutex(mutex);
    frame_queue.push(pkt1);
    SDL_CondSignal(cond);
    SDL_UnlockMutex(mutex);

    return true;
}

bool FrameQueue::get(AVFrame* pkt, int block)
{
    int ret;

    SDL_LockMutex(mutex);

    for (;;) {

        if (quit) {
            ret = -1;
            break;
        }

        // if there are packets in the queue procced to get the next one
        //cout << "Size of queue = " << this->packet_queue.size() << endl;
        if (frame_queue.size() > 0) {
            pkt = frame_queue.front();
            frame_queue.pop();
            ret = 1;
            break;
        } // want to block?
        else if (!block) {
            ret = 0;
            break;
        } // blocking, this will release the mutex and wait for 'cond' to happen 
        else {
            SDL_CondWait(this->cond, this->mutex);
        }
    }
    SDL_UnlockMutex(this->mutex);
    return ret;
}

int FrameQueue::count()
{
    return frame_queue.size();
}

bool FrameQueue::full()
{
    return (this->frame_queue.size() >= 5);
}

bool FrameQueue::empty()
{
    return (frame_queue.size() == 0);
}
