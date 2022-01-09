#include "thread.h"
#include "event.h"

int device_tick_allocate(device_tick_t **destination,
                         int (*callback)(void *object, app_event_t *event, device_tick_t *tick, app_thread_t *thread)) {
    if (callback != NULL) {
        *destination = malloc(sizeof(device_tick_t));
        if (*destination == NULL) {
            return CO_ERROR_OUT_OF_MEMORY;
        }
        (*destination)->callback = callback;
    }
    return 0;
}

void device_tick_free(device_tick_t **tick) {
    if (*tick != NULL) {
        free(*tick);
        *tick = NULL;
    }
}

int app_thread_allocate(app_thread_t **destination, void *app_or_object, void (*callback)(void *ptr), const char *const name,
                        uint16_t stack_depth, size_t queue_size, size_t priority, void *argument) {
    *destination = (app_thread_t *)malloc(sizeof(app_thread_t));
    app_thread_t *thread = *destination;
    thread->device = ((app_t *)app_or_object)->device;
    xTaskCreate(callback, name, stack_depth, (void *)thread, priority, (void *)&thread->task);
    if (thread->task == NULL) {
        return CO_ERROR_OUT_OF_MEMORY;
    }
    if (queue_size > 0) {
        thread->queue = xQueueCreate(queue_size, sizeof(app_event_t));
        vQueueAddToRegistry(thread->queue, name);
        if (thread->queue == NULL) {
            return CO_ERROR_OUT_OF_MEMORY;
        }
    }

    thread->argument = argument;
    return 0;
}

int app_thread_free(app_thread_t **thread) {
    free(*thread);
    if ((*thread)->queue != NULL) {

        vQueueDelete((*thread)->queue);
    }
    *thread = NULL;
    return 0;
}

/* Returns specific member of app_threads_t struct by its numeric index*/
static inline device_tick_t *device_tick_by_index(device_t *device, size_t index) {
    device_tick_t *ticks = (device_tick_t *)&device->ticks;
    return &ticks[index];
}

/* Returns specific member of device_ticks_t struct by its numeric index*/
static inline app_thread_t *app_thread_by_index(app_t *app, size_t index) {
    app_thread_t *threads = (app_thread_t *)&app->threads;
    return &threads[index];
}

device_t *app_thread_filter_devices(app_thread_t *thread) {
    app_t *app = thread->device->app;
    size_t tick_index = app_thread_get_tick_index(thread);
    device_t *first_device;
    device_t *last_device;
    for (size_t i = 0; i < app->device_count; i++) {
        device_t *device = &app->device[i];
        device_tick_t *tick = device_tick_by_index(device, tick_index);
        // Subscribed devices have corresponding tick handler
        if (tick == NULL) {
            continue;
        }

        // Return first device
        if (first_device == NULL) {
            first_device = device;
        }

        // double link the ticks for fast iteration
        if (last_device == NULL) {
            device_tick_by_index(last_device, tick_index)->next_device = device;
            tick->prev_device = device;
        }
    }

    return first_device;
}

size_t app_thread_get_tick_index(app_thread_t *thread) {
    for (size_t i = 0; i < sizeof(app_threads_t) / sizeof(app_thread_t *); i++) {
        if (app_thread_by_index(thread->device->app, i) == thread) {
            // Both input and immediate threads invoke same input tick
            if (i > 0) {
                return i - 1;
            } else {
                return i;
            }
        }
    }
    return -1;
}

void app_thread_execute(app_thread_t *thread) {
    thread->last_time = xTaskGetTickCount();
    app_t *app = thread->device->app;
    size_t tick_index = app_thread_get_tick_index(thread);
    device_t *first_device = app_thread_filter_devices(thread);
    app_event_t event; // incoming event from the queu
    size_t deferred = 0;

    // No devices subscribe to this thread
    if (first_device == NULL) {
        log_printf("No devices subsribe to thread #%i\n", thread_index) return;
    }
    while (true) {
        // Tick at least once every minute if no devices scheduled it sooner
        thread->current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        thread->next_time = thread->last_time + 60000;

        // Remember if the event was already deferred previously for bookkeeping
        bool_t was_deferred = event.status == APP_EVENT_DEFERRED;

        // Iterate all devices that are subscribed to the task
        device_tick_t *tick;
        for (device_t *device = first_device; device; device = tick->next_device) {
            tick = device_tick_by_index(device, tick_index);

            if (event.type == APP_EVENT_WAKEUP ? tick->next_time <= thread->current_time    // Wakeup events only run when time is right
                                               : device_can_handle_event(device, &event)) { // others need matching

                // Tick callback may change event status, set software timer or both
                tick->callback(device->object, &event, tick, thread);
                tick->last_time = thread->current_time;

                if (event.status == APP_EVENT_WAITING) {
                    // Mark event as processed, since device gave didnt give it any special status
                    event.status = APP_EVENT_RECEIVED;
                } else if (event.status >= APP_EVENT_HANDLED) {
                    // Device claimed the event, stop broadcasting immediately
                    break;
                }
            }

            // Device may request thread to wake up at specific time without any events
            if (tick->next_time >= thread->current_time && thread->next_time > tick->next_time) {
                thread->next_time = tick->next_time;
            }
        }

        switch (event.status) {
        case APP_EVENT_WAITING:
            log_printf("No devices are listening to event: #%i\n", event.type);
            break;

        // Some busy device wanted to handle event, so it now has to be re-queued
        case APP_EVENT_DEFERRED:
        case APP_EVENT_ADDRESSED:
            if (thread == app->threads->input) {
                // input thread will off-load event to catchup thread queue directly without waking it up
                event.status = APP_EVENT_WAITING;
                xQueueSend(app->threads->catchup->queue, &event, 0);
            } else if (thread->queue) {
                // Other threads with queue will put the event at the back of the queue
                event.status = APP_EVENT_DEFERRED;
                xQueueSend(thread->queue, &event, 0);
                if (!was_deferred) {
                    deferred++;
                }
            } else {
                // Threads without a queue will leave event linger in the only available notification slot
                // New events will overwrite the deferred event that wasnt handled in time
            }
            break;

        // Events that were deferred need to be marked as handled by device
        case APP_EVENT_HANDLED:
            if (was_deferred && thread->queue) {
                deferred--;
            }
            break;
        }

        // Should the thread block?
        if (thread->queue != NULL                                                  // threads without queue block right away
            || (deferred > 0 && deferred == uxQueueMessagesWaiting(thread->queue)) // if all events in queue are deferredt
            || !xQueueReceive(thread->queue, &event, 0))                           // or else it will try to get new events from queue
        {
            // threads are expected to receive notifications in order to wake up
            uint32_t notification = ulTaskNotifyTake(true, pdMS_TO_TICKS(TIME_DIFF(thread->next_time, thread->current_time)));
            if (notification == 0) {
                // if no notification was recieved within scheduled time, it means thread is woken up by schedule
                // so synthetic wake upevent is generated
                event = (app_event_t){.producer = app->device, .type = APP_EVENT_WAKEUP};
            } else {
                if (thread->queue != NULL) {
                    // otherwise there must be a new message in queue
                    if (!xQueueReceive(thread->queue, &event, 0)) {
                        log_printf("Error: Thread #%i woken up by notification but its queue is empty\n", tick_index);
                    }
                } else if (notification != APP_EVENT_READY) {
                    // if thread doesnt have queue, event is passed as address in notification slot instead
                    // publisher will have to ensure the event memory survives until thread is ready for it.
                    memcpy(&event, (uint32_t *)notification, sizeof(app_event_t));
                }
            }
        }
    }
}

bool_t app_thread_notify(app_thread_t *thread) { return xTaskNotify(thread->task, APP_EVENT_READY, eIncrement); }

bool_t app_thread_notify_from_isr(app_thread_t *thread) {
    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    bool_t result = xTaskNotifyFromISR(thread->task, APP_EVENT_READY, eIncrement, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return result;
}

bool_t app_thread_publish_generic(app_thread_t *thread, app_event_t *event, bool_t to_front) {
    if (thread->queue == NULL || xQueueGenericSend(thread->queue, event, 0, to_front)) {
        return xTaskNotify(thread->task, (uint32_t)event, eSetValueWithOverwrite);
        ;
    } else {
        return false;
    }
}

bool_t app_thread_publish_generic_from_isr(app_thread_t *thread, app_event_t *event, bool_t to_front) {
    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    bool_t result = false;
    if (thread->queue == NULL || xQueueGenericSendFromISR(thread->queue, event, &xHigherPriorityTaskWoken, to_front)) {
        result = xTaskNotifyFromISR(thread->task, (uint32_t)event, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
    };
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return result;
}

void device_tick_schedule(device_tick_t *tick, uint32_t next_time) { tick->next_time = next_time; }

void device_tick_delay(device_tick_t *tick, app_thread_t *thread, uint32_t timeout) {
    device_tick_schedule(tick, thread->current_time + timeout);
}

int device_ticks_allocate(device_t *device) {
    device->ticks = (device_ticks_t *)malloc(sizeof(device_ticks_t));
    if (device->ticks == NULL || //
        device_tick_allocate(&device->ticks->input, device->callbacks->input_tick) ||
        device_tick_allocate(&device->ticks->output, device->callbacks->output_tick) ||
        device_tick_allocate(&device->ticks->async, device->callbacks->async_tick) ||
        device_tick_allocate(&device->ticks->poll, device->callbacks->poll_tick) ||
        device_tick_allocate(&device->ticks->idle, device->callbacks->idle_tick)) {
        return CO_ERROR_OUT_OF_MEMORY;
    }

    return 0;
}

int device_ticks_free(device_t *device) {
    device_tick_free(&device->ticks->input);
    device_tick_free(&device->ticks->output);
    device_tick_free(&device->ticks->async);
    device_tick_free(&device->ticks->poll);
    device_tick_free(&device->ticks->idle);
    free(device->ticks);
    return 0;
}

int app_threads_allocate(app_t *app) {
    if (app_thread_allocate(&app->threads->input, app, (void (*)(void *ptr))app_thread_execute, "App: Input", 200, 50, 5, NULL) ||
        app_thread_allocate(&app->threads->async, app, (void (*)(void *ptr))app_thread_execute, "App: Async", 200, 1, 4, NULL) ||
        app_thread_allocate(&app->threads->output, app, (void (*)(void *ptr))app_thread_execute, "App: Output", 200, 1, 3, NULL) ||
        app_thread_allocate(&app->threads->poll, app, (void (*)(void *ptr))app_thread_execute, "App: Poll", 200, 1, 2, NULL) ||
        app_thread_allocate(&app->threads->idle, app, (void (*)(void *ptr))app_thread_execute, "App: Idle", 200, 1, 1, NULL)) {
        return CO_ERROR_OUT_OF_MEMORY;
    } else {
        return 0;
    }
}

int app_threads_free(app_t *app) {
    app_thread_free(&app->threads->input);
    app_thread_free(&app->threads->output);
    app_thread_free(&app->threads->async);
    app_thread_free(&app->threads->poll);
    app_thread_free(&app->threads->idle);
    free(app->threads);
    return 0;
}