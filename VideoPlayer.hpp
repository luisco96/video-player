#pragma once
extern "C"
{
#include <SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}
#include "Queues.hpp"
#include <string>

class VideoPlayer
{
    private:
        int width, height;
        struct _video {
            AVCodecContext* codec_ctx;
            AVStream* stream;
            AVPacket pkt;
            AVFrame* frame;
            int stream_index;
        } video;

        struct _audio {
            AVCodecContext* codec_ctx;
            AVStream* stream;
            AVPacket pkt;
            AVFrame* frame;
            int stream_index;
            SwrContext* swr_ctx;

        } audio;

        struct _screen {
            SDL_Surface *main_screen;
            SDL_Window *window;
            SDL_Renderer *renderer;
            SDL_RendererInfo renderer_info;
            SDL_Texture *texture; // change to SDL_Texture after you know what to do
            SDL_Rect rec;
        } screen;

        AVFormatContext* format_ctx;
        PacketQueue queue_pkt_audio;
        PacketQueue queue_pkt_video;
        FrameQueue queue_frame_picture;
        std::string file_name;
        int default_width = 640;
        int default_height = 480;


    public:
        VideoPlayer();
        ~VideoPlayer();

        int open_file(std::string file);
        int open_codec_context();
        //int create_thread();
        int read_video_packet();
        static int decode_video_pkt(void *args);
        // Screen functions 
        int screen_init();
        int refresh_screen();
        SDL_Thread* audio_tid;
        SDL_Thread* video_tid;
        SDL_Thread* read_tid;

    private:
//        static int read_thread(void* args);
//        static int video_thread(void* args);
//        static int audio_thread(void* args);
        int video_open();
};

