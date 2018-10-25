package io.ooni.mk.vendor.mkcurl;

class MkCurlResponse {
  long handle = 0;

  protected MkCurlResponse(long handle) throws MkCurlException {
    if (handle == 0) {
      throw new MkCurlException("You passed MkCurlResponse a null handle");
    }
    this.handle = handle;
  }

  public long getError() {
    return FFI.ResponseGetError(this.handle);
  }

  public String getRedirectURL() throws MkCurlException {
    String s = FFI.ResponseGetRedirectURL(this.handle);
    if (s == null) {
      throw new MkCurlException("FFI returned a null redirect URL");
    }
    return s;
  }

  public byte[] getBody() throws MkCurlException {
    byte[] b = FFI.ResponseGetBody(this.handle);
    if (b == null) {
      throw new MkCurlException("FFI returned a null response body");
    }
    return b;
  }

  public double getBytesSent() {
    return FFI.ResponseGetBytesSent(this.handle);
  }

  public double getBytesRecv() {
    return FFI.ResponseGetBytesRecv(this.handle);
  }

  public byte[] getLogs() throws MkCurlException {
    byte[] b = FFI.ResponseGetLogs(this.handle);
    if (b == null) {
      throw new MkCurlException("FFI returned a null logs byte array");
    }
    return b;
  }

  public String getRequestHeaders() throws MkCurlException {
    String s = FFI.ResponseGetRequestHeaders(this.handle);
    if (s == null) {
      throw new MkCurlException("FFI returned a null request headers");
    }
    return s;
  }

  public byte[] getResponseHeaders() throws MkCurlException {
    byte[] b = FFI.ResponseGetResponseHeaders(this.handle);
    if (b == null) {
      throw new MkCurlException("FFI returned a null response headers");
    }
    return b;
  }

  public String getCertificateChain() throws MkCurlException {
    String s = FFI.ResponseGetCertificateChain(this.handle);
    if (s == null) {
      throw new MkCurlException("FFI returned a null certificate chain");
    }
    return s;
  }

  @Override public synchronized void finalize() {
    FFI.ResponseDelete(this.handle);
  }
}
