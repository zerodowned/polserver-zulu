/*
History
=======
2006/10/28 Shinigami: GCC 4.1.1 fix - missing declaration of friend functions

Notes
=======

*/


#ifndef __SCHEDULE_H
#define __SCHEDULE_H

#include <time.h>
#include "polclock.h"

class TaskScheduler
{
public:
    static bool is_dirty() { return dirty_; }
    static void cleanse() { dirty_ = false; }
    static void mark_dirty() { dirty_ = true; }
private:
    static bool dirty_;
};

class ScheduledTask
{
public:
    explicit ScheduledTask( polclock_t next_run_clock );
    virtual ~ScheduledTask();
	
    bool ready(polclock_t now);
    bool late(polclock_t now);
    polticks_t ticks_late( polclock_t now );
    
    polticks_t ticks_left( polclock_t now ) const;
    polclock_t clocksleft( polclock_t now );
	polclock_t next_run_clock() const;
    virtual void execute(polclock_t now) = 0;
    
    virtual void cancel( void );
protected:
    bool cancelled;
    polclock_t next_run_clock_;
    polclock_t last_run_clock_;
    friend class SchComparer;
    friend void check_scheduled_tasks( polclock_t* clocksleft, bool* pactivity );
    friend void check_scheduled_tasks2( void );
    friend polclock_t calc_scheduler_clocksleft( polclock_t now );
};

inline polclock_t ScheduledTask::next_run_clock() const
{
    return next_run_clock_;
}

void check_scheduled_tasks( polclock_t* clocksleft, bool* pactivity );
void check_scheduled_tasks2( void );
polclock_t calc_scheduler_clocksleft( polclock_t now );

class PeriodicTask : public ScheduledTask
{
public:
	PeriodicTask( void (*f)(void), int n_secs, const char* name );
	PeriodicTask( void (*f)(void), int initial_wait_seconds, int periodic_seconds, const char* name );

    virtual void execute(polclock_t now);
    void start();

private:
    polclock_t n_initial_clocks;
	polclock_t n_clocks;
	void (*f)(void);
    const char* name_;
};

class OneShotTask : public ScheduledTask
{
public:
    OneShotTask( OneShotTask** handle, polclock_t run_when );
    virtual void cancel( void );

protected:
        // oneshots can't be deleted, only cancelled.
    virtual ~OneShotTask();
    virtual void execute(polclock_t now);

    virtual void on_run() = 0;
private:
    OneShotTask** handle;
};


template <class T>
class OneShotTaskInst : public OneShotTask
{
public:
    OneShotTaskInst( OneShotTask** handle, polclock_t run_when, void (*f)(T data), T data ) :
        OneShotTask( handle, run_when ), data_(data), f_(f) {}
    
    virtual void on_run();
private:
    T data_;
    void (*f_)(T data);
};

template<class T>
void OneShotTaskInst<T>::on_run()
{
    (*f_)(data_);
}


#endif
