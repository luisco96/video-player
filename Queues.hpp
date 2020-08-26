#pragma once
extern "C"
{
#include <SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}
#include <queue>

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)

class PacketQueue
{
private:
	std::queue<AVPacket> pkt_queue;
	size_t	byte_size;
	SDL_mutex* mutex;
	SDL_cond* cond;

public:
	PacketQueue();
	~PacketQueue();

	bool push(AVPacket* pkt);
	bool get(AVPacket* pkt, int block);
	size_t size();
	int count();
	bool full();
	bool empty();
};

class FrameQueue
{
	// not defined yet
    std::queue<AVFrame *> frame_queue;
    SDL_mutex *mutex;
    SDL_cond *cond;

    public:
        FrameQueue();
        ~FrameQueue();

	bool push(AVFrame** pkt);
	bool get(AVFrame* pkt, int block);
	int count();
	bool full();
	bool empty();

};
