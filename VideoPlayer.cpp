#include "VideoPlayer.hpp"
#include <string>
#include <iostream>

using namespace std;

VideoPlayer::VideoPlayer()
    : video()
    , audio()
    , format_ctx(NULL)
    , queue_pkt_audio()
    , queue_pkt_video()
    , queue_frame_picture()
    , audio_tid(NULL)
    , video_tid(NULL)
    , read_tid(NULL)
{}

VideoPlayer::~VideoPlayer()
{
    if (format_ctx) 
        avformat_close_input(&format_ctx);

    if (video.codec_ctx) {
        avcodec_free_context(&video.codec_ctx);
        avcodec_free_context(&audio.codec_ctx);
    }

    // mave we have to do something else
}

int VideoPlayer::open_file(string file)
{
    int ret;

    if (file.empty()) {
        cout << "Error opening file, No file provided\n";
        return -1;
    }
    file_name = file;

    if ((ret = avformat_open_input(&this->format_ctx, file_name.c_str(), NULL, NULL)) < 0) {
        cout << "Error openinig video with ffpmeg\n";
        return -1;
    }

    /* retrieve stream information */
    if (avformat_find_stream_info(this->format_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return -1;
    }

    // open both video and audio codecs
    ret = open_codec_context();
    if (ret < 0) {
        cout << "Could not open codec for Video\n";
        return -1;
    }

    av_dump_format(this->format_ctx, 0, file_name.c_str(), 0);

    /*ret = init_SDL();
    if (ret < 0) {
        cout << "Error initializing SDL";
        return -1;
    }*/

    return 0;
}

int VideoPlayer::open_codec_context()
{
    // &this->audio_stream_index, & this->codec_ctx, this->format_ctx, AVMEDIA_TYPE_AUDIO
    int ret, stream_index;
    AVCodec* audio_dec = nullptr, *video_dec = nullptr;
    AVDictionary* opts = nullptr;

    // first lets find the audio
    ret = av_find_best_stream(format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (ret < 0) {
        cout << "Could not find audio stream from file\n";
        return ret;
    }
    else {
        stream_index = ret;
        audio.stream = format_ctx->streams[stream_index];

        // find the decoder
        audio_dec = avcodec_find_decoder(audio.stream->codecpar->codec_id);
        if (!audio_dec) {
            cout << "Failed to find decoder\n";
            return AVERROR(EINVAL);
        }

        // allocate a codec context for the decoder
        audio.codec_ctx = avcodec_alloc_context3(audio_dec);
        if (!audio.codec_ctx) {
            cout << "Error allocating codec contex\n";
            return AVERROR(ENOMEM);
        }

        // copy codec parameters from input stream to output context
        if ((ret = avcodec_parameters_to_context(audio.codec_ctx, audio.stream->codecpar)) < 0) {
            cout << "Failed to copy codec parameters to decoder context\n";
            return ret;
        }

        // Init the decoder
        if ((ret = avcodec_open2(audio.codec_ctx, audio_dec, &opts)) < 0) {
            cout << "Failed to open codec\n";
            return ret;
        }

        audio.stream_index = stream_index;

        // Know the video
        ret = av_find_best_stream(format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if (ret < 0) {
            cout << "Could not find video stream from file\n";
            return ret;
        }
        else {
            stream_index = ret;
            video.stream = format_ctx->streams[stream_index];

            // find the decoder
            video_dec = avcodec_find_decoder(video.stream->codecpar->codec_id);
            if (!video_dec) {
                cout << "Failed to find decoder\n";
                return AVERROR(EINVAL);
            }

            // allocate a codec context for the decoder
            video.codec_ctx = avcodec_alloc_context3(video_dec);
            if (!video.codec_ctx) {
                cout << "Error allocating codec contex\n";
                return AVERROR(ENOMEM);
            }

            // copy codec parameters from input stream to output context
            if ((ret = avcodec_parameters_to_context(video.codec_ctx, video.stream->codecpar)) < 0) {
                cout << "Failed to copy codec parameters to decoder context\n";
                return ret;
            }

            // Init the decoder
            if ((ret = avcodec_open2(video.codec_ctx, video_dec, &opts)) < 0) {
                cout << "Failed to open codec\n";
                return ret;
            }

            video.stream_index = stream_index;

            // get video width and height;
            this->width = video.codec_ctx->coded_width;
            this->height = video.codec_ctx->coded_height;
            //debug prints 
            cout << "Width: " << this->width << endl; 
            cout << "Height: " << this->height << endl; 
        }
        return 0;
    }
}

//int VideoPlayerCMS::create_thread()
//{
//    // This function its going to create three threads
//    // the first one is going to be the reading thread 
//    // we will use SDL threads 
//
//    return 0;
//}
// Im going to implement the video part first without threads, cuz im pretty sure im gonna
// mess up the program if I try to do it with threads, so im saving my time and a headache
int VideoPlayer::read_video_packet() 
{
    // this function will insert video packets to the 
    // video queue
    AVPacket pkt;

    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    while(av_read_frame(this->format_ctx, &pkt))
    {
        if(pkt.stream_index == this->video.stream_index) {
            this->queue_pkt_video.push(&pkt);
        } else {
            av_packet_unref(&pkt);
        }
    }
    return 0;
}

int VideoPlayer::decode_video_pkt(void* args)
{
    VideoPlayer *vid = (VideoPlayer *)args;
    AVPacket pkt;
    AVFrame *frame = av_frame_alloc();
    int ret;

    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    for(;;) {
        // first get a pkt from the queue
        if(!pkt.data) {
            ret = vid->queue_pkt_video.get(&pkt, 1); // this will block till we get a packet
            ret = avcodec_send_packet(vid->video.codec_ctx, &pkt);
            if (ret < 0) {
                printf("Error submitting a packet for decodign\n");
                return -1;
            }
        }

        while (ret >= 0) {
            ret = avcodec_receive_frame(vid->video.codec_ctx, frame);
            if (ret < 0) {
                // these two return values mean that there is no frame output available
                // but there were no errors during decoding 
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                    // set packet to defaults 
                    av_packet_unref(&pkt);
                    pkt.data = NULL;
                    pkt.size = 0;
                    break; // return to for loop
                }

                printf("Error during decoding\n");
                return -1;
            }
            // at this point we have a decoded frame 
            vid->queue_frame_picture.push(&frame);
            SDL_Delay(15);
        }
    }
}

int VideoPlayer::screen_init()
{
    // Init SDL 
    int ret;
    int flags;

    // Initialize SDL for video and audio
    flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
    ret = SDL_Init(flags);
    if(ret != 0) {
        printf("Could not initialize SDL = %s\n", SDL_GetError());
        return -1;
    }
    // Disable this events
    SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
    SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

    // at the moment we just want to see the video, no hidding, no rezising 
    flags |= SDL_WINDOW_BORDERLESS;
    flags |= SDL_WINDOW_ALWAYS_ON_TOP;

    // The first thing we need to show the video on the screen is a window
    // this windows will contain the video 
    // to show that video it need the following:
    //      - A Renderer, is basically the engine of the window that will take the information of the video
    //        that we will get from the ffpmeg library in form of frames 
    //      - A Texture, a texture is a struct to represent the pixel information of the image, the Renderer 
    //        processes the frames and saves the information in a texture 
    //      - A rect is an area were we will show the texture generated by the renderer. aka here is were the image
    //        will be displayed inside the window
    this->screen.window = SDL_CreateWindow("CMS Video Player",
            SDL_WINDOWPOS_UNDEFINED, 
            SDL_WINDOWPOS_UNDEFINED,
            this->width,
            this->height,
            flags
            );
    // we will not be scaling at this point, but its good to have this here in case that we do it later 
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    
    if(this->screen.window) {
        // If we have a window, try to create a Renderer for that window using our GPU
        this->screen.renderer = SDL_CreateRenderer(this->screen.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if(!this->screen.renderer) {
            // if we could not get a Renderer with GPU support, try the default 
            printf("Failed to initialized hardware accelerated renderer: %s\n", SDL_GetError());
            this->screen.renderer = SDL_CreateRenderer(this->screen.window, -1, 0);
        }
        if(this->screen.renderer) {
            if (!SDL_GetRendererInfo(this->screen.renderer, &this->screen.renderer_info)){
                printf("Initilized %s render\n", this->screen.renderer_info.name);
            }
        }
    }
    if (!this->screen.window || !this->screen.renderer || !this->screen.renderer_info.num_texture_formats) {
        cout << "Failed to create window or renderer " << SDL_GetError() << endl;
        return -1;
    }

    // At this point we have the Window and the renderer, video_open will take care of the texture and rectangule 
    ret = VideoPlayer::video_open();
    if (ret < 0) {
        printf("Error setting window size and position\n");
        return -1;
    }

    return 0;
}

int VideoPlayer::video_open() {

    SDL_SetWindowTitle(this->screen.window, this->file_name.c_str());
    SDL_SetWindowSize(this->screen.window, this->width, this->height);
    SDL_SetWindowPosition(this->screen.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(this->screen.window);
    // Create the texture 
    this->screen.texture = SDL_CreateTexture(this->screen.renderer,
            SDL_PIXELFORMAT_IYUV,
            SDL_TEXTUREACCESS_STREAMING,
            this->width,
            this->height);
    // A rect struct just have this 4 variables, set defaults 
    this->screen.rec.x = 0;
    this->screen.rec.y = 0;
    this->screen.rec.w = this->width;
    this->screen.rec.h = this->height;
    
    return 0;
}

int VideoPlayer::refresh_screen()
{
    // update texture 
    int ret;
    AVFrame *frame = NULL;
    this->queue_frame_picture.get(frame, 1);

    ret = SDL_UpdateYUVTexture(this->screen.texture, 
            NULL,
            frame->data[0], frame->linesize[0],
            frame->data[1], frame->linesize[1],
            frame->data[2], frame->linesize[2]
            );
    SDL_RenderClear(this->screen.renderer);
    SDL_RenderCopy(this->screen.renderer, this->screen.texture, NULL, &this->screen.rec);
    SDL_RenderPresent(this->screen.renderer);

    return 0;

}
