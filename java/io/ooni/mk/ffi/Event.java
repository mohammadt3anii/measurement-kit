// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

package io.ooni.mk.ffi;

public class Event {
    public String serialize() {
        return FFI.EventSerialize(handle);
    }

    @Override
    public synchronized void finalize() {
        FFI.EventDestroy(handle);
        handle = 0;
    }

    protected Event(long handle) {
        this.handle = handle;
    }

    long handle = 0;
}
