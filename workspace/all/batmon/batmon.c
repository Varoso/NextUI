
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <defines.h>
#include <api.h>

/*
 * Battery status monitor.
 *
 * Keeps the current battery percentage available through /tmp/percBat.
 * Historical battery tracking and SQLite dependencies have been removed.
 */

static volatile sig_atomic_t quit_requested = 0;

static void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
    {
        quit_requested = 1;
    }
}

static void register_handlers(void)
{
    struct sigaction sa = {0};

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

static void cleanup(void)
{
    remove("/tmp/percBat");
}

int main(void)
{
    int is_charging = 0;
    int battery_percentage = 0;
    int old_percentage = -1;

    FILE *fp = NULL;

    atexit(cleanup);
    register_handlers();

    while (!quit_requested)
    {
        PLAT_getBatteryStatusFine(&is_charging, &battery_percentage);

        /*
         * Publish the initial percentage and subsequent changes.
         */
        if (battery_percentage != old_percentage)
        {
            LOG_debug(
                "Battery status updated: charging = %d, percentage = %d\n",
                is_charging,
                battery_percentage
            );

            old_percentage = battery_percentage;

            if ((fp = fopen("/tmp/percBat", "w+")) != NULL)
            {
                fprintf(fp, "%d", battery_percentage);
                fflush(fp);
                fsync(fileno(fp));
                fclose(fp);
            }
        }

        /*
         * SIGSTOP pauses the process at operating-system level.
         * SIGCONT resumes it; neither needs a custom handler.
         */
        sleep(1);
    }

    LOG_debug("Battery monitor stopping\n");

    return EXIT_SUCCESS;
}
