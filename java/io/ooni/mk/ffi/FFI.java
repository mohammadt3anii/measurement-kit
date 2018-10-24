// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

package io.ooni.mk.ffi;

class FFI {
    protected final static native long NettestStart(String settings);
    protected final static native boolean TaskIsDone(long task);
    protected final static native long TaskWaitForNextEvent(long task);
    protected final static native void TaskInterrupt(long task);
    protected final static native void TaskDestroy(long task);
    protected final static native String EventSerialize(long event);
    protected final static native void EventDestroy(long event);
}
