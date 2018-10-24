// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

package io.ooni.mk.ffi;

public class Task {
    public static Task startNettest(String conf) {
        long task = FFI.NettestStart(conf);
        return (task != 0) ? new Task(task) : null;
    }

    public boolean isDone() {
        return FFI.TaskIsDone(handle);
    }

    public Event waitForNextEvent() {
        long event = FFI.TaskWaitForNextEvent(handle);
        return (event != 0) ? new Event(event) : null;
    }

    public void interrupt() {
        FFI.TaskInterrupt(handle);
    }

    @Override
    public synchronized void finalize() {
        FFI.TaskDestroy(handle);
        handle = 0;
    }

    Task(long handle) {
        this.handle = handle;
    }

    long handle = 0;
}
