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
