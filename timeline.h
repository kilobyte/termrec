struct tty_event
{
    struct timeval t;
    int len;
    char *data;
    vt100 *snapshot;
    struct tty_event *next;
};

void timeline_lock();
void timeline_unlock();

void timeline_init();
void timeline_clear();
void replay_pause();
void replay_resume();
void replay_seek();
int replay_play(struct timeval *delay);
extern struct timeval tr;
int defsx, defsy;
