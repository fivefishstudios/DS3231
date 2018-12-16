void SetDateTime(int year = 1970, int mon = 1, int day = 1, int hour = 0, int min = 0, int sec = 0) {
    struct     tm Clock;
    Clock.tm_year = year - 1900;
    Clock.tm_mon  = mon - 1;      // note: subtract 1 
    Clock.tm_mday = day;
    Clock.tm_hour = hour;
    Clock.tm_min  = min;
    Clock.tm_sec  = sec;
    time_t epoch = mktime(&Clock);
    if (epoch == (time_t) -1) {
        error("Error in clock setting\n");
        // Stop here
    }
    set_time(epoch);
}

