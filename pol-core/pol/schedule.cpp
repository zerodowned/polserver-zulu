/*
History
=======


Notes
=======

*/

#include "../clib/stl_inc.h"
#include <time.h>

#include "../clib/passert.h"
#include "../clib/tracebuf.h"

#include "dtrace.h"
#include "polclock.h"
#include "polsig.h"
#include "profile.h"
#include "schedule.h"



class SchComparer : public less<ScheduledTask*>
{
public:
    bool operator()(const ScheduledTask* x, const ScheduledTask* y) const;
};

bool SchComparer::operator()(const ScheduledTask* x, const ScheduledTask* y) const
{
    if (x->next_run_clock_ == y->next_run_clock_)
        return x > y;
    else
        return x->next_run_clock_ > y->next_run_clock_;
}

typedef priority_queue< ScheduledTask*, vector<ScheduledTask*>, SchComparer > TASK_QUEUE;

static TASK_QUEUE task_queue;
bool TaskScheduler::dirty_ = false;

static void add_task( ScheduledTask *task )
{
    TaskScheduler::mark_dirty();
	task_queue.push( task );
}

ScheduledTask::ScheduledTask( polclock_t next_run_clock ) :
    cancelled( false ),
    next_run_clock_( next_run_clock ),
    last_run_clock_( 0 )
{
}

ScheduledTask::~ScheduledTask()
{
}

void ScheduledTask::cancel()
{
    cancelled = true;
}

inline bool ScheduledTask::ready(polclock_t now_clock)
{
	return (now_clock >= next_run_clock_);
}
inline bool ScheduledTask::late(polclock_t now_clock)
{
	return (now_clock > next_run_clock_);
}
inline polticks_t ScheduledTask::ticks_late( polclock_t now_clock )
{
    return (now_clock - next_run_clock_);
}

polclock_t ScheduledTask::clocksleft( polclock_t now_clock )
{
    return next_run_clock_ - now_clock;
}

PeriodicTask::PeriodicTask( void (*i_f)(void), int initial_wait_seconds, const char* name ) :
	ScheduledTask( 0 ),
    n_initial_clocks( initial_wait_seconds * POLCLOCKS_PER_SEC ),
    n_clocks( initial_wait_seconds * POLCLOCKS_PER_SEC ),
	f(i_f),
    name_(name)
{
}

PeriodicTask::PeriodicTask( void (*i_f)(void), int initial_wait_seconds, int periodic_seconds, const char* name ) :
	ScheduledTask( 0 ),
    n_initial_clocks( initial_wait_seconds * POLCLOCKS_PER_SEC ),
    n_clocks( periodic_seconds * POLCLOCKS_PER_SEC ),
	f(i_f),
    name_(name)
{
}


void PeriodicTask::start()
{
    next_run_clock_ = polclock() + n_initial_clocks;
    last_run_clock_ = 0;
    add_task( this );
}

/*
int PeriodicTask::ready(time_t now)
{
	return (now >= next_run);
}
*/
void PeriodicTask::execute(polclock_t now)
{
	last_run_clock_ = now;
    next_run_clock_ += n_clocks;

/*
    cout << name_ << ": last=" << last_run_clock_ 
         << ", next=" << next_run_clock_ 
         << ", n_clocks=" << n_clocks
         << ", now=" << now
         << endl;
*/
    
    (*f)();
}

OneShotTask::OneShotTask( OneShotTask** handle, polclock_t run_when_clock ) :
    ScheduledTask( run_when_clock),
    handle(handle)
{
    if (handle != NULL)
    {
        passert( *handle == NULL );
        *handle = this;
    }
    
    add_task( this );
}

OneShotTask::~OneShotTask()
{
    if (handle != NULL) 
        *handle = NULL;
}

void OneShotTask::cancel()
{
    ScheduledTask::cancel();
    if (handle != NULL)
        *handle = NULL;
    handle = NULL;
}
void OneShotTask::execute(polclock_t now_clock)
{
	if (!cancelled)
    {
        cancel();
        on_run();
    }
}

/*void check_scheduled_tasks( void )
{
	unsigned idx;
	unsigned count;
	time_t now;

	if (p_task_list)
	{
		now = poltime(NULL);
		count = p_task_list->count();
		for( idx = 0; idx < count; idx++ )
		{
			ScheduledTask *task = (*p_task_list)[idx];
            if (task->ready(now))
            {
				task->execute(now);
            }

            if (task->cancelled)
            {
                p_task_list->remove( idx );
                delete task;
            }
		}
	}
}
*/
void check_scheduled_tasks( polclock_t* clocksleft, bool* pactivity )
{
    THREAD_CHECKPOINT( tasks, 101 );
    TaskScheduler::cleanse();

	polclock_t now_clock = polclock();
    TRACEBUF_ADDELEM( "check_scheduled_tasks now_clock", now_clock );
    bool activity = false;
    passert( !task_queue.empty() );
    THREAD_CHECKPOINT( tasks, 102 );
    for(;;)
    {
        THREAD_CHECKPOINT( tasks, 103 );
        ScheduledTask* task = task_queue.top();
        TRACEBUF_ADDELEM( "check_scheduled_tasks toptask->nextrun", task->next_run_clock() );
        THREAD_CHECKPOINT( tasks, 104 );
        if (!task->ready(now_clock))
        {
            *clocksleft = task->clocksleft(now_clock);
            *pactivity = activity;
            TRACEBUF_ADDELEM( "check_scheduled_tasks clocksleft", *clocksleft );
            return;
        }
        
        THREAD_CHECKPOINT( tasks, 105 );
        if (!task->late(now_clock))
        {
            INC_PROFILEVAR( tasks_ontime );
        }
        else
        {
            INC_PROFILEVAR( tasks_late );
            INC_PROFILEVAR_BY( tasks_late_ticks, task->ticks_late(now_clock) );
        }

        THREAD_CHECKPOINT( tasks, 106 );
        task_queue.pop();

        THREAD_CHECKPOINT( tasks, 107 );
        task->execute(now_clock);
        THREAD_CHECKPOINT( tasks, 108 );
        
        activity = true;
        
        if (task->cancelled)
        {
            THREAD_CHECKPOINT( tasks, 109 );
            delete task;
        }
        else
        {
            THREAD_CHECKPOINT( tasks, 110 );
            task_queue.push( task );
        }
        THREAD_CHECKPOINT( tasks, 111 );
    }
}

polclock_t calc_scheduler_clocksleft( polclock_t now )
{
    ScheduledTask* task = task_queue.top();
    if (!task->ready(now))
    {
        dtrace(20) << "Task " << task << ": " << task->clocksleft(now) << " clocks left" << endl;
        return task->clocksleft(now);
    }
    else
    {
        return 0; // it's ready now!
    }
}

