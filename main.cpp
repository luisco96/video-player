#include "VideoPlayer.hpp"
#include "Queues.hpp"

using namespace std;

int reading_thread(void* args);

int main(int argc, char *argv[])
{
    string file = "billie-eilish-everything-i-wanted.mp4";
    VideoPlayer vid;

    vid.open_file(file);
    vid.read_tid = SDL_CreateThread(reading_thread, "Reading Thread", &vid);    
    vid.video_tid = SDL_CreateThread(vid.decode_video_pkt, "Video Thread", &vid);

    vid.screen_init();

    for(;;) {
        vid.refresh_screen();
        SDL_Delay(30);
    }
    return 0;        
}

int reading_thread(void* args)
{
    VideoPlayer *vid = (VideoPlayer *)args;
    
    for(;;){
        vid->read_video_packet();
    }
}
