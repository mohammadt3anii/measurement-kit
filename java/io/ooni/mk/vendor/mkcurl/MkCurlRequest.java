package io.ooni.mk.vendor.mkcurl;

public class MkCurlRequest {
  long handle = 0;

  public MkCurlRequest() throws MkCurlException {
    this.handle = FFI.RequestNew();
    if (this.handle == 0) {
      throw new MkCurlException("cannot allocate FFI request");
    }
  }

  public void setCABundlePath(String path) {
    FFI.RequestSetCABundlePath(this.handle, path);
  }

  public void enableHTTP2() {
    FFI.RequestEnableHTTP2(this.handle);
  }

  public void setMethodPOST() {
    FFI.RequestSetMethodPOST(this.handle);
  }

  public void setURL(String url) {
    FFI.RequestSetURL(this.handle, url);
  }

  public void addHeader(String header) {
    FFI.RequestAddHeader(this.handle, header);
  }

  public void setBody(byte[] body) {
    FFI.RequestSetBody(this.handle, body);
  }

  public void setTimeout(long timeout) {
    FFI.RequestSetTimeout(this.handle, timeout);
  }

  public void setProxyURL(String url) {
    FFI.RequestSetProxyURL(this.handle, url);
  }

  public void enableFollowRedirect() {
    FFI.RequestEnableFollowRedirect(this.handle);
  }

  public MkCurlResponse perform() throws MkCurlException {
    long rh = FFI.Perform(this.handle);
    if (rh == 0) {
      throw new MkCurlException("FFI.Perform() returned a null handle");
    }
    return new MkCurlResponse(rh);
  }

  @Override public synchronized void finalize() {
    FFI.RequestDelete(this.handle);
  }
}
